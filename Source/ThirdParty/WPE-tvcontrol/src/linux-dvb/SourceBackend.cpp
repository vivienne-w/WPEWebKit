/*
 * Copyright (C) 2017 TATA ELXSI
 * Copyright (C) 2017 Metrological
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "SourceBackend.h"
#include "tv-log.h"

namespace LinuxDVB {

#define TVControlPushEvent(eventId, tunerId, evtState /*optional*/, channelInfo /*optional*/, parentalLockState /*optional*/, emergencyAlerted /*optional*/) \
{                                                                                                                                  \
    struct wpe_tvcontrol_event* event = reinterpret_cast<struct wpe_tvcontrol_event*>(malloc(sizeof(struct wpe_tvcontrol_event))); \
    event->eventID = eventId;                                                                                                      \
    event->tuner_id.data = strndup(tunerId, TUNER_ID_LEN);                                                                         \
    TvLogInfo("Tuner ID =  %s \n", event->tuner_id.data);                                                                          \
    event->tuner_id.length = strlen(tunerId);                                                                                      \
    event->state = evtState;                                                                                                       \
    event->parentalLock = parentalLockState;                                                                                       \
    event->emergencyAlert = emergencyAlerted;                                                                                      \
    event->channel_info = channelInfo;                                                                                             \
    TvLogTrace();                                                                                                                  \
    fflush(stdout);                                                                                                                \
    m_eventQueue->pushEvent(event);                                                                                                \
}

SourceBackend::SourceBackend(EventQueue<wpe_tvcontrol_event*>* eventQueue, SourceType type, TunerData* tunerData)
    : m_sType(type)
    , m_tunerData(tunerData)
    , m_isRescanned(false)
    , m_isScanStopped(false)
    , m_isScanInProgress(false)
    , m_eventQueue(eventQueue)
    , m_scanIndex(0)
    , m_isRunning(true)
{
    m_adapter = stoi(m_tunerData->tunerId.substr(0, m_tunerData->tunerId.find(":")));
    m_demux = stoi(m_tunerData->tunerId.substr(m_tunerData->tunerId.find(":") + 1));
    m_siThread = std::thread(&SourceBackend::siThread, this);
    m_scanningThread = std::thread(&SourceBackend::scanningThread, this);
    m_setCurrentChannelThread = std::thread(&SourceBackend::setCurrentChannelThread, this);
    m_channelVector.length = 0;
    m_frequency = 0;
}

SourceBackend::~SourceBackend()
{
    m_isRunning = false;
    m_channelChangeCondition.notify_all();
    m_scanCondition.notify_all();
    m_scanningThread.join();
    m_siThread.join();
    m_setCurrentChannelThread.join();
    clearChannelVector();
    clearChannelList();
}

void SourceBackend::clearChannelVector()
{
    if (m_channelVector.length) {
        for (int i; i < m_channelVector.length; i++) {
            if (m_channelVector.channels[i].name)
                free(m_channelVector.channels[i].name);
        }
        delete[] m_channelVector.channels;
        m_channelVector.length = 0;
    }
}

void SourceBackend::clearChannelList()
{
    if (!m_channelList.empty())
        m_channelList.clear();
}

tvcontrol_return SourceBackend::startScanning(bool isRescanned)
{
    tvcontrol_return ret = TVControlFailed;

    if (m_isRunning && !m_isScanInProgress) {
        ret = TVControlSuccess;
        m_scanMutex.lock();
        m_isRescanned = isRescanned;
        m_scanCondition.notify_all();
        m_scanMutex.unlock();
    }
    return ret;
}

void SourceBackend::scanningThread()
{
    while (m_isRunning) {
        m_scanMutex.lock();
        m_isScanInProgress = false;
        m_scanCondition.wait(m_scanMutex);
        m_isScanInProgress = true;
        m_scanMutex.unlock();

        if (!m_isRunning)
            break;

        uint64_t modulation = m_tunerData->modulation;
        int length = (m_tunerData->frequency).size();
        struct dvbfe_handle* feHandle = openFE(m_tunerData->tunerId);
        if (m_isRescanned) {
            clearChannelList();
            TVControlPushEvent(ScanningChanged, m_tunerData->tunerId.c_str(), Cleared, nullptr, (parental_lock_state)0, nullptr);
            m_scanIndex = 0;
        }
        if (feHandle) {
            int i;
            length = 1;
            for (i = m_scanIndex; i < length; i++) {
                int frequency = 575000000/* m_tunerData->frequency[i]*/;
                if (tuneToFrequency(frequency, modulation, feHandle)) {
                    switch (m_sType) {
                    case Atsc:
                    case AtscMH:
                        atscScan(frequency, modulation);
                        break;
                    case DvbT:
                    case DvbT2:
                    case DvbC:
                    case DvbC2:
                    case DvbS:
                    case DvbS2:
                    case DvbH:
                    case DvbSh:
                        dvbScan();
                        break;
                    default:
                        TvLogInfo("Type Not supported!!");
                        break;
                    }
                }
                if (m_isScanStopped) {
                    TVControlPushEvent(ScanningChanged, m_tunerData->tunerId.c_str(), Stopped, nullptr, (parental_lock_state)0, nullptr);
                    m_isScanStopped = false;
                    m_scanIndex = i + 1;
                    break;
                }
            }
            if (i == length) {
                TVControlPushEvent(ScanningChanged, m_tunerData->tunerId.c_str(), Completed, nullptr, (parental_lock_state)0, nullptr);
                m_scanIndex = 0;
            }
            dvbfe_close(feHandle);
        } else
            TVControlPushEvent(ScanningChanged, m_tunerData->tunerId.c_str(), Stopped, nullptr, (parental_lock_state)0, nullptr);
    }
}

void SourceBackend::processPMT(int pmtFd, std::map<int, int>& streamInfo)
{
    int size;
    uint8_t siBuf[4096];

    // read the section
    if ((size = read(pmtFd, siBuf, sizeof(siBuf))) < 0)
        return;

    // parse section
    struct section* section = section_codec(siBuf, size);
    if (!section)
        return;

    // parse section_ext
    struct section_ext* sectionExt = section_ext_decode(section, 0);
    if (!sectionExt)
        return;

    // parse PMT
    struct mpeg_pmt_section* pmt = mpeg_pmt_section_codec(sectionExt);
    if (!pmt)
        return;

    struct mpeg_pmt_stream* curStream;
    mpeg_pmt_section_streams_for_each(pmt, curStream)
    {
        TvLogInfo("stream_type : %x pid : %x \n", curStream->stream_type, curStream->pid);
        streamInfo[curStream->pid] = curStream->stream_type;
    }
}

tvcontrol_return SourceBackend::stopScanning()
{
    m_isScanStopped = true;
    return TVControlSuccess;
}

ChannelBackend* SourceBackend::getChannelByLCN(uint64_t channelNo)
{
    for (auto& channel : m_channelList) {
        if (channelNo == channel.second->getLCN())
            return channel.second.get();
    }
    return nullptr;
}

void SourceBackend::startPlayBack(int frequency, uint64_t modulation, int pmtPid, int videoPid, int audioPid)
{
    char command[1024];
    snprintf(command, 1024, "PLAY:%d:%d:%d:%d", frequency, pmtPid, videoPid, audioPid);
    TvLogInfo("Command : %s\n", command);
    std::ofstream tvConfig;
    tvConfig.open("/etc/TVTune.txt", std::ios::trunc);
    tvConfig << command;
    tvConfig.close();
}

void SourceBackend::stopPlayBack()
{
    char command[1024];
    snprintf(command, 1024, "STOP");
    TvLogInfo("Command : %s\n", command);
    std::ofstream myfile;
    myfile.open("/etc/TVTune.txt", std::ios::trunc);
    myfile << command;
    myfile.close();
    sleep(1);
}

tvcontrol_return SourceBackend::getChannels(wpe_tvcontrol_channel_vector** channelVector)
{
    /*Populate channel list */
    tvcontrol_return ret = TVControlFailed;
    TvLogTrace();
    if (!m_channelList.empty()) {
        TvLogTrace();
        if (!m_channelVector.length
            || (m_channelVector.length != m_channelList.size())) {
            clearChannelVector();
            m_channelVector.length = m_channelList.size();
            m_channelVector.channels = new wpe_tvcontrol_channel[m_channelList.size()];
            int i = 0;
            for (auto& channel : m_channelList) {
                m_channelVector.channels[i].networkId = channel.second->getNetworkId();
                m_channelVector.channels[i].transportSId = channel.second->getTransportStreamId();
                m_channelVector.channels[i].serviceId = channel.second->getServiceId();
                m_channelVector.channels[i].number = channel.second->getLCN();
                if (!(channel.second->getName()).empty())
                    m_channelVector.channels[i].name = strdup((channel.second->getName()).c_str());
                i++;
            }
        }
        *channelVector = &m_channelVector;
        ret = TVControlSuccess;
    } else {
        channelVector = nullptr;
        TvLogInfo("Channel list is empty .Scanning is incomplete \n");
    }
    return ret;
}

tvcontrol_return SourceBackend::setCurrentChannel(uint64_t channelNo)
{
    TvLogInfo("\nSet Channel invoked \n");
    tvcontrol_return ret = TVControlFailed;
    if (m_channelNo != channelNo) {
        m_currentChannel = getChannelByLCN(channelNo);
        bool lockTrack;
        m_currentChannel->isParentalLocked(&lockTrack);
        if (!lockTrack) {
            m_channelChangeMutex.lock();
            m_channelNo = channelNo;
            m_channelChangeCondition.notify_all();
            m_channelChangeMutex.unlock();
            ret = TVControlSuccess;
        }
    }
    return ret;
}

void SourceBackend::setCurrentChannelThread()
{
    while (m_isRunning) {
        m_channelChangeMutex.lock();
        m_channelChangeCondition.wait(m_channelChangeMutex);
        m_channelChangeMutex.unlock();
        if (!m_isRunning)
            break;
        TvLogInfo("\nTune to Channel:: %" PRIu64 "\n", m_channelNo);
        ChannelBackend* channel = getChannelByLCN(m_channelNo);
        if (channel) {
            printf("UNCLOCKED");
            stopPlayBack();
            struct dvbfe_handle* feHandle = openFE(m_tunerData->tunerId);
            if (feHandle) {
                int freq = channel->getFrequency();
                unsigned modulation = m_tunerData->modulation;
                if (tuneToFrequency(freq, modulation, feHandle)) {
                    std::map<int, int> streamInfo;
                    int programNumber = channel->getProgramNumber();
                    mpegScan(programNumber, streamInfo);
                    dvbfe_close(feHandle);
                    if (!streamInfo.empty()) {
                        int videoPid = 0;
                        int audioPid = 0;
                        for (auto& info : streamInfo) {
                            TvLogInfo("Stream Type: %d Pid : %d\n", info.second, info.first);
                            if ((!videoPid) && info.second == 0x2)
                                videoPid = info.first;
                            if ((!audioPid) && info.second == 0x81)
                                audioPid = info.first;
                        }
                        TvLogInfo("%s:%s:%d \n", __FILE__, __func__, __LINE__);

                        int pmtPid = streamInfo[0];
                        startPlayBack(freq, modulation, pmtPid, videoPid, audioPid);

                        TVControlPushEvent(ChannelChanged, m_tunerData->tunerId.c_str(), (scanning_state)0 /*irrelevant*/, nullptr, (parental_lock_state)0, nullptr);
                        TvLogInfo("%s:%s:%d \n", __FILE__, __func__, __LINE__);
                    }
                } else {
                    dvbfe_close(feHandle);
                    TvLogInfo("Cannot tune to channel \n");
                }
            }
        }
    } // While
    TvLogTrace();
}

void SourceBackend::siThread()
{
    int flag = 0;
    while (m_isRunning) {
        m_channelChangeMutex.lock();
        m_channelChangeCondition.wait(m_channelChangeMutex);
        m_channelChangeMutex.unlock();
        if (!m_isRunning)
            break;
        ChannelBackend* channel = getChannelByLCN(m_channelNo);
        int i;
        if (m_frequency != channel->getFrequency()) {
            m_frequency = channel->getFrequency();
            if (flag)
                stopEITThread();
            startEITThread();
            flag = 1;
        } else
            continue;
    }
}

void SourceBackend::stopEITThread() {
    m_isEITRunning = false;
    m_EITThread.join();
}

void SourceBackend::startEITThread()
{
    m_isEITRunning = true;
    if (processMGT())
        fprintf(stderr, "%s(): error calling parse_mgt()\n", __FUNCTION__);

    TvLogInfo("receiving EIT \n");
    m_EITThread = std::thread(&SourceBackend::EITThread, this);
}

void SourceBackend::EITThread()
{
    while (m_isEITRunning) {
        int i;
        for (i = 0; i < 4; i++) {
            if (processEIT(i, m_eitPid[i]))
                fprintf(stderr, "%s(): error calling parse_eit()\n", __FUNCTION__);
        }
    }
}

void SourceBackend::dvbScan()
{
}

bool SourceBackend::tuneToFrequency(int frequency, uint64_t modulation, struct dvbfe_handle* feHandle)
{
    struct dvbfe_info feInfo;
    dvbfe_get_info(feHandle, DVBFE_INFO_FEPARAMS, &feInfo, DVBFE_INFO_QUERYTYPE_IMMEDIATE, 0);

    switch (m_sType) {
    case Atsc:
    case AtscMH:
        if (DVBFE_TYPE_ATSC != feInfo.type) {
            fprintf(stderr, "%s(): only ATSC frontend supported currently\n", __FUNCTION__);
            return false;
        }
        switch (modulation) {
        case DVBFE_ATSC_MOD_VSB_8:
            feInfo.feparams.frequency = frequency;
            feInfo.feparams.inversion = DVBFE_INVERSION_AUTO;
            feInfo.feparams.u.atsc.modulation = DVBFE_ATSC_MOD_VSB_8;
            break;
        default:
            TvLogInfo("Modulation not supported!!!\n");
            return false;
        }
        break;
    default:
        TvLogInfo("Only Atsc supported!!\n");
        return false;
    }
    fprintf(stdout, "tuning to %d Hz, please wait...\n", frequency);

    if (dvbfe_set(feHandle, &feInfo.feparams, 3 * 1000)) {
        fprintf(stderr, "%s(): cannot lock to %d Hz in 3 seconds\n", __FUNCTION__, frequency);
        return false;
    }
    fprintf(stdout, "tuner locked.\n");
    return true;
}

void SourceBackend::mpegScan(int programNumber, std::map<int, int>& streamInfo)
{
    int patFd = createSectionFilter(PID_PAT, TABLE_PAT);
    struct pollfd pollFd[2];
    pollFd[0].fd = patFd; // PAT
    pollFd[0].events = POLLIN | POLLPRI | POLLERR;
    pollFd[1].fd = 0; // PMT
    pollFd[1].events = 0;

    bool flag = true;

    while (flag) {
        int count = poll(pollFd, 2, 100);
        if (count < 0) {
            fprintf(stderr, "Poll error\n");
            break;
        }
        if (!count)
            continue;
        if (pollFd[0].revents & (POLLIN | POLLPRI)) {
            if (processPAT(pollFd[0].fd, programNumber, &pollFd[1], streamInfo))
                dvbdemux_stop(pollFd[0].fd);
            else
                flag = false;
        }
        if (pollFd[1].revents & (POLLIN | POLLPRI)) {
            processPMT(pollFd[1].fd, streamInfo);
            dvbdemux_stop(pollFd[1].fd);
            flag = false;
        }
    }
}

void SourceBackend::atscScan(int frequency, uint64_t modulation)
{
    switch (modulation) {
    case DVBFE_ATSC_MOD_VSB_8:
        if (!processTVCT(frequency))
            fprintf(stderr, "%s(): error calling parse_tvct()\n", __FUNCTION__);
        break;
    default:
        TvLogInfo("Modulation not supported!!\n");
        return;
    }
}
}

int SourceBackend::createSectionFilter(uint16_t pid, uint8_t tableId)
{
    int demuxFd = -1;

    // Open the demuxer
    if ((demuxFd = dvbdemux_open_demux(m_adapter, m_demux, 0)) < 0)
        return -1;

    // Create a section filter
    uint8_t filter[18];
    memset(filter, 0, sizeof(filter));
    uint8_t mask[18];
    memset(mask, 0, sizeof(mask));
    filter[0] = tableId;
    mask[0] = 0xFF;
    if (dvbdemux_set_section_filter(demuxFd, pid, filter, mask, 1, 1)) {
        close(demuxFd);
        return -1;
    }
    return demuxFd;
}

int SourceBackend::parseSections(int dmxfd, struct atsc_section_psip** psip)
{
    unsigned char sibuf[4096];
    int size;
    int ret;
    struct pollfd pollfd;
    struct section* section;
    struct section_ext* sectionExt;

    // poll for data
    pollfd.fd = dmxfd;
    pollfd.events = POLLIN | POLLERR |POLLPRI;
    if ((ret = poll(&pollfd, 1, 60000)) < 0) {
        fprintf(stderr, "%s(): error calling poll()\n", __FUNCTION__);
        return -1;
    }

    if (!ret)
        return 0;

    /* read it */
    if ((size = read(dmxfd, sibuf, sizeof(sibuf))) < 0) {
        fprintf(stderr, "%s(): error calling read()\n", __FUNCTION__);
        return -1;
    }

    /* parse section */
    section = section_codec(sibuf, size);
    if (!section) {
        fprintf(stderr, "%s(): error calling section_codec()\n", __FUNCTION__);
        return -1;
    }

    sectionExt = section_ext_decode(section, 0);
    if (!sectionExt) {
        fprintf(stderr, "%s(): error calling section_ext_decode()\n", __FUNCTION__);
        return -1;
    }

    *psip = atsc_section_psip_decode(sectionExt);
    if (!*psip) {
        fprintf(stderr, "%s(): error calling atsc_section_psip_decode()\n", __FUNCTION__);
        return -1;
    }
    return 1;
}

bool SourceBackend::processPAT(int patFd, int programNumber, struct pollfd* pollfd, std::map<int, int>& streamInfo)
{
    int size;
    uint8_t siBuf[4096];
    // Read the section
    if ((size = read(patFd, siBuf, sizeof(siBuf))) < 0)
        return false;

    // Parse section
    struct section* section = section_codec(siBuf, size);
    if (!section)
        return false;
    // Parse sectionExt
    struct section_ext* sectionExt = section_ext_decode(section, 0);
    if (!sectionExt)
        return false;
    // Parse PAT
    struct mpeg_pat_section* pat = mpeg_pat_section_codec(sectionExt);
    if (!pat)
        return false;

    // Try and find the requested program
    struct mpeg_pat_program* curProgram;
    mpeg_pat_section_programs_for_each(pat, curProgram)
    {
        TvLogInfo("Program Number:- %d PMT Pid:- %x\n", curProgram->program_number, curProgram->pid);
        if (curProgram->program_number == programNumber) {
            if (pollfd->fd != -1)
                close(pollfd->fd);
            if ((pollfd->fd = createSectionFilter(curProgram->pid, TABLE_PMT)) < 0)
                return false;
            pollfd->events = POLLIN | POLLPRI | POLLERR;
            streamInfo[0] = curProgram->pid;
            break;
        }
    }

    // Remember the PAT version
    // pat_version = sectionExt->version_number;
    return true;
}

bool SourceBackend::processTVCT(int frequency)
{
    int dmxfd;
    int numSections = -1;
    uint32_t sectionPattern = 0;
    struct atsc_section_psip* psip;
    const enum atsc_section_tag tag = stag_atsc_terrestrial_virtual_channel;
    do {
        dmxfd = createSectionFilter(PID_VCT, tag);
        int ret = parseSections(dmxfd, &psip);
        if (0 > ret) {
            fprintf(stderr, "%s(): error calling atsc_scan_table()\n", __FUNCTION__);
            return -1;
        }

        if (!ret) {
            TvLogInfo("timeout\n");
            return 0;
        }

        struct atsc_tvct_section* tvct;
        tvct = atsc_tvct_section_codec(psip);
        if (!tvct) {
            fprintf(stderr, "%s(): error decode table section\n", __FUNCTION__);
            return false;
        }

        if (-1 == numSections) {
            numSections = 1 + tvct->head.ext_head.last_section_number;
            if (32 < numSections) {
                fprintf(stderr
                    , "%s(): no support yet for "
                    "tables having more than 32 sections\n",
                    __FUNCTION__);
                return false;
            }
        } else {
            if (numSections != 1 + tvct->head.ext_head.last_section_number) {
                fprintf(stderr
                    , "%s(): last section number does not match\n",
                    __FUNCTION__);
                return false;
            }
        }
        if (sectionPattern & (1 << tvct->head.ext_head.section_number))
            continue;
        sectionPattern |= 1 << tvct->head.ext_head.section_number;

        int i, k;
        struct atsc_tvct_channel* ch;
        atsc_tvct_section_channels_for_each(tvct, ch, i)
        {
            /* initialize the currInfo structure */
            /* each EIT covers 3 hours */
            std::unique_ptr<ChannelBackend> currInfo = std::make_unique<ChannelBackend>();
            currInfo->setNumEits((12 / 3) + !!(12 % 3));
            if (currInfo->getEit(0)) {
                fprintf(stderr, "%s(): non-NULL pointer detected during initialization", __FUNCTION__);
                return -1;
            }

            currInfo->setEit(reinterpret_cast<atscEitInfo*>(calloc(currInfo->getNumEits(), sizeof(struct atscEitInfo))));
            if (!currInfo->getEit(0)) {
                fprintf(stderr, "%s(): error calling calloc()\n", __FUNCTION__);
                return -1;
            }

            char* serviceName = reinterpret_cast<char*>(malloc(sizeof(char) * 8));
            for (int k = 0; k < 7; k++) {
                serviceName[k] = getBits((const uint8_t*)ch->short_name,
                    k * 16, 16);
            }
            serviceName[7] = '\0';

            int majorNum = ch->major_channel_number;
            int minorNum = ch->minor_channel_number;
            int logicalChannelNumber = majorNum << 10 | minorNum;

            TvLogInfo("TSID : %d\n", ch->channel_TSID);
            TvLogInfo("ProgramNumber : %d\n", ch->program_number);
            TvLogInfo("LCN : %d\n", logicalChannelNumber);
            TvLogInfo("ServiceID : %d\n", ch->source_id);

            currInfo->setNumber(logicalChannelNumber);
            currInfo->setTransportStreamId(ch->channel_TSID);
            currInfo->setServiceId(ch->source_id);
            currInfo->setProgramNumber(ch->program_number);
            currInfo->setFrequency(frequency);
            TvLogInfo("Short Name : %s\n", serviceName);
            struct descriptor* desc;
            atsc_tvct_channel_descriptors_for_each(ch, desc)
            {
                TvLogInfo("Descriptor : %x\n", desc->tag);
                switch (desc->tag) {
                case dtag_atsc_extended_channel_name:
                    parseAtscExtendedChannelNameDescriptor(&serviceName, const_cast<const unsigned char*>(reinterpret_cast<unsigned char*>(desc)));
                    break;
                default:
                    break;
                }
            }
            std::string name(serviceName);
            TvLogInfo("Extended name : %s\n", name.c_str());
            currInfo->setName(name);
            if (m_channelList.empty() || (!m_channelList.count(logicalChannelNumber))) {
                m_channelList.insert(std::pair<uint64_t, std::unique_ptr<ChannelBackend>>(logicalChannelNumber, std::move(currInfo)));
                TvLogInfo("Sending channel Info\n");
                wpe_tvcontrol_channel* channelInfo = reinterpret_cast<struct wpe_tvcontrol_channel*>(malloc(sizeof(struct wpe_tvcontrol_channel)));
                channelInfo->networkId = 0;
                channelInfo->transportSId = ch->channel_TSID;
                channelInfo->serviceId = ch->source_id;
                channelInfo->name = strdup(name.c_str());
                channelInfo->number = logicalChannelNumber;
                TVControlPushEvent(ScanningChanged, m_tunerData->tunerId.c_str(), Scanned, channelInfo, (parental_lock_state)0, nullptr);
            }
        }
    } while (sectionPattern != (uint32_t)((1 << numSections) - 1));

    m_numChannels = m_channelList.size();
    return true;
}

bool SourceBackend::processEIT(int index, uint16_t pid)
{
    int numSections;
    uint8_t currChannelIndex;
    uint8_t sectionNum;
    uint32_t sectionPattern;
    const enum atsc_section_tag tag = stag_atsc_event_information;
    struct atsc_eit_section* eit;
    ChannelBackend* currInfo;
    struct atscEitInfo* eitInfo;
    struct atscEitSectionInfo* section;
    uint16_t sourceId;
    uint32_t eitInstancePattern = 0;
    int i, k, ret;
    struct atsc_section_psip* psip;
    int dmxfd;
    fflush(stdout);
    while (eitInstancePattern != (uint32_t)((1 << m_numChannels) - 1)) {
        sourceId = 0xFFFF;
        sectionPattern = 0;
        numSections = -1;
        do {
            dmxfd = createSectionFilter(pid, tag);
            int ret = parseSections(dmxfd, &psip);
            if (0 > ret) {
                fprintf(stderr, "%s(): error calling atsc_scan_table()\n", __FUNCTION__);
                return -1;
            }
            if (!ret) {
                TvLogInfo("timeout\n");
                return 0;
            }
            struct atsc_eit_section* eit;
            eit = atsc_eit_section_codec(psip);
            if (!eit) {
                fprintf(stderr, "%s(): error decode table section\n", __FUNCTION__);
                return false;
            }

            if (0xFFFF == sourceId) {
                sourceId = atsc_eit_section_source_id(eit);
                k = 0;
                for (const auto& p : m_channelList) {
                    if (sourceId == p.second->getServiceId()) {
                        currInfo = p.second.get();
                        currChannelIndex = k;
                        if (!index)
                            currInfo->setLastEvent(nullptr);
                        break;
                    }
                    k++;
                }
                if (k == m_numChannels) {
                    fprintf(stderr, "%s(): cannot find source_id 0x%04X in the EIT\n", __FUNCTION__, sourceId);
                    return -1;
                }
            } else {
                if (sourceId != atsc_eit_section_source_id(eit))
                    continue;
            }
            if (eitInstancePattern & (1 << currChannelIndex)) {
                /* we have received this instance,
                 * so quit quick
                 */
                break;
            }
            if (-1 == numSections) {
                numSections = 1 +
                    eit->head.ext_head.last_section_number;
                if (32 < numSections) {
                    fprintf(stderr, "%s(): no support yet for tables having more than 32 sections\n", __FUNCTION__);
                    return -1;
                }
            } else {
                if (numSections != 1 + eit->head.ext_head.last_section_number) {
                    fprintf(stderr, "%s(): last section number does not match\n", __FUNCTION__);
                    return -1;
                }
            }
            if (sectionPattern & (1 << eit->head.ext_head.section_number))
                continue;
            sectionPattern |= 1 << eit->head.ext_head.section_number;

            fflush(stdout);
            eitInfo = currInfo->getEit(index);
            eitInfo->section = reinterpret_cast<atscEitSectionInfo*>(realloc(eitInfo->section,
                (eitInfo->numEitSections + 1) * sizeof(struct atscEitSectionInfo)));
            if (!eitInfo->section) {
                fprintf(stderr, "%s(): error calling realloc()\n", __FUNCTION__);
                return -1;
            }
            sectionNum = eit->head.ext_head.section_number;
            if (!eitInfo->numEitSections) {
                eitInfo->numEitSections = 1;
                section = eitInfo->section;
            } else {
                /* have to sort it into section order
                 * (temporal order)
                 */
                for (i = 0; i < eitInfo->numEitSections; i++) {
                    if (eitInfo->section[i].sectionNum > sectionNum)
                        break;
                }
                memmove(&eitInfo->section[i + 1],
                    &eitInfo->section[i],
                    (eitInfo->numEitSections - i) *
                    sizeof(struct atscEitSectionInfo));
                section = &eitInfo->section[i - 1];
                section = &eitInfo->section[i];
                eitInfo->numEitSections += 1;
            }
            section->sectionNum = sectionNum;
            section->numEvents = eit->num_events_in_section;
            section->numEtms = 0;
            section->numReceivedEtms = 0;
            if (!(section->events =  new ProgramBackend*[section->numEvents])) {
                fprintf(stderr, "%s(): error calling calloc()\n", __FUNCTION__);
                return -1;
            }
            if (processEvents(currInfo, eit, section)) {
                fprintf(stderr, "%s(): error calling parse_events()\n", __FUNCTION__);
                return -1;
            }
        } while (sectionPattern != (uint32_t)((1 << numSections) - 1));
        eitInstancePattern |= 1 << currChannelIndex;
    }
    for (const auto& p : m_channelList) {
        ChannelBackend* channel = p.second.get();
        struct atscEitInfo* ei = channel->getEit(index);
        struct atscEitSectionInfo* s;

        if (!ei->numEitSections) {
            channel->setLastEvent(nullptr);
            continue;
        }
        s = &ei->section[ei->numEitSections - 1];
        /* BUG: it's incorrect when last section has no event */
        if (!s->numEvents) {
            channel->setLastEvent(nullptr);
            continue;
        }
        channel->setLastEvent((s->events[s->numEvents - 1]));
    }
    return 0;
}

bool SourceBackend::processEvents(ChannelBackend *currInfo,
    struct atsc_eit_section* eit, struct atscEitSectionInfo* section)
{
    int i, j, k;
    struct atsc_eit_event* e;
    time_t startTime;
    atsc_eit_section_events_for_each(eit, e, i) {
        ProgramBackend* lastEvent = currInfo->getLastEvent();
        if ((!i) && lastEvent) {
            if (e->event_id == lastEvent->getEventId()) {
                section->events[i] = nullptr;
                // skip if it's the same event spanning
                // over sections

                continue;
            }
        }
        if (e->ETM_location && 3 != e->ETM_location) {
            /* FIXME assume 1 and 2 is interchangable as of now */
            section->numEtms++;
        }

        currInfo->setEventInfoIndex(currInfo->getEventInfoIndex() + 1);
        ProgramBackend* eInfo = new ProgramBackend();
        int titleLength = (e->title_length)+1;
        int bufPos = 0;
        char* titleAddr = reinterpret_cast<char*>(malloc(titleLength));
        struct atsc_text* eventTitle;
        struct atsc_text_string* str;
        eventTitle = atsc_eit_event_name_title_text(e);
        if (!eventTitle)
            continue;
        atsc_text_strings_for_each(eventTitle, str, j) {
            struct atsc_text_string_segment* seg;
            atsc_text_string_segments_for_each(str, seg, k) {
                if (0 > atsc_text_segment_decode(seg, (uint8_t **)&titleAddr, (size_t *)&titleLength, (size_t *)&bufPos)) {
                    fprintf(stderr, "%s(): error calling atsc_text_segment_decode()\n", __FUNCTION__);
                    return -1;
                }
            }
        }
        titleAddr[bufPos] = '\0';
        std::string title(titleAddr);
        eInfo->setTitle(title);
        TvLogInfo("title = %s\n", title.c_str());
        fflush(stdout);

        TvLogInfo("event_id : %d\n", e->event_id);
        fflush(stdout);
        startTime = atsctime_to_unixtime(e->start_time);
        eInfo->setDuration(e->length_in_seconds);
        eInfo->setEventId(e->event_id);
        eInfo->setStartTime(startTime);
        section->events[i] = eInfo;
        currInfo->appendPrograms(eInfo);
    }
    TvLogTrace();
    fflush(stdout);
    return 0;
}

bool SourceBackend::processMGT()
{
    const enum atsc_section_tag tag = stag_atsc_master_guide;
    struct atsc_mgt_section* mgt;
    struct atsc_mgt_table* t;
    int i, j, ret;
    struct atsc_section_psip* psip;
    memset(m_eitPid, 0xFF, 128 * sizeof(uint16_t));
    memset(m_ettPid, 0xFF, 128 * sizeof(uint16_t));
    struct mgtTableName mgtTabNameArray[] = {
        {0x0000, "terrestrial VCT with current_next_indictor=1"},
        {0x0001, "terrestrial VCT with current_next_indictor=0"},
        {0x0002, "cable VCT with current_next_indictor=1"},
        {0x0003, "cable VCT with current_next_indictor=0"},
        {0x0004, "channel ETT"},
        {0x0005, "DCCSCT"},
        {0x00FF, "reserved for future ATSC use"},
        {0x017F, "EIT"},
        {0x01FF, "reserved for future ATSC use"},
        {0x027F, "event ETT"},
        {0x02FF, "reserved for future ATSC use"},
        {0x03FF, "RRT with rating region"},
        {0x0FFF, "user private"},
        {0x13FF, "reserved for future ATSC use"},
        {0x14FF, "DCCT with dcc_id"},
        {0xFFFF, "reserved for future ATSC use"}
    };
    int dmxfd;
    dmxfd = createSectionFilter(PID_VCT, tag);
    ret = parseSections(dmxfd, &psip);
    if (0 > ret) {
        fprintf(stderr, "%s(): error calling atsc_scan_table()\n", __FUNCTION__);
        return -1;
    }
    if (!ret) {
        TvLogInfo("timeout\n");
        return 0;
    }
    mgt = atsc_mgt_section_codec(psip);
    if (!mgt) {
        fprintf(stderr, "%s(): error decode table section\n", __FUNCTION__);
        return false;
    }
    atsc_mgt_section_tables_for_each(mgt, t, i) {
        struct mgtTableName table;

        for (j = 0; j < (int)(sizeof(mgtTabNameArray) / sizeof(struct mgtTableName)); j++) {
            if (t->table_type > mgtTabNameArray[j].range)
                continue;
            table = mgtTabNameArray[j];
            if (!j || mgtTabNameArray[j - 1].range + 1 == mgtTabNameArray[j].range)
                j = -1;
            else {
                j = t->table_type - mgtTabNameArray[j - 1].range - 1;
                if (0x017F == table.range)
                    m_eitPid[j] = t->table_type_PID;
                else if (0x027F == table.range)
                    m_ettPid[j] = t->table_type_PID;
            }
            break;
        }
        fprintf(stdout, "  %2d: type = 0x%04X, PID = 0x%04X, %s", i,
            t->table_type, t->table_type_PID, table.string);
        if (-1 != j)
            fprintf(stdout, " %d", j);
    }
    return 0;
}

void SourceBackend::parseAtscExtendedChannelNameDescriptor(char** serviceName, const unsigned char* buf)
{
    unsigned char* b = (unsigned char*)buf + 2;
    int numStr = b[0];
#define uncompressed_string 0x00

    b++;
    for (int i = 0; i < numStr; i++) {
        int numSeg = b[3];
        b += 4; /* skip lang code */
        for (int j = 0; j < numSeg; j++) {
            int compressionType = b[0], /* mode = b[1],*/ num_bytes = b[2];

            switch (compressionType) {
            case uncompressed_string:
                char* name;
                name = *serviceName;
                if (name)
                    free(name);
                name = reinterpret_cast<char*>(malloc(num_bytes * sizeof(char) + 1));
                memcpy(name, &b[3], num_bytes);
                name[num_bytes] = '\0';
                *serviceName = name;
                break;
            default:
                break;
            }
            b += 3 + num_bytes;
        }
    }
}

uint32_t SourceBackend::getBits(const uint8_t* buf, int startbit, int bitlen)
{
    const uint8_t* b = &buf[startbit / 8];
    startbit %= 8;

    int bitHigh = 8;
    uint32_t tmpLong = b[0];
    for (int i = 0; i < ((bitlen - 1) >> 3); i++) {
        tmpLong <<= 8;
        tmpLong |= b[i + 1];
        bitHigh += 8;
    }

    startbit = bitHigh - startbit - bitlen;
    tmpLong = tmpLong >> startbit;
    uint32_t mask = (1ULL << bitlen) - 1;
    return tmpLong & mask;
}

tvcontrol_return SourceBackend::getPrograms(uint64_t serviceId, struct wpe_get_programs_options* programsOptions, struct wpe_tvcontrol_program_vector** programVector)
{
    TvLogTrace();
    ChannelBackend* channel;
    for (const auto& p : m_channelList) {
        if (serviceId == p.second->getServiceId()) {
            channel = p.second.get();
            break;
        }
    }
    return channel->getPrograms(programsOptions, programVector);
}

tvcontrol_return SourceBackend::getCurrentProgram(uint64_t serviceId, struct wpe_tvcontrol_program** program)
{
    TvLogTrace();
    ChannelBackend* channel;
    for (const auto& p : m_channelList) {
        if (serviceId == p.second->getServiceId()) {
            channel = p.second.get();
            break;
        }
    }
    return channel->getCurrentProgram(program);
}

void SourceBackend::isParentalLocked(uint64_t channelNo, bool* isLocked)
{
    TvLogTrace();
    m_currentChannel->isParentalLocked(isLocked);
    TvLogTrace();
    return;
}

tvcontrol_return SourceBackend::setParentalLock(uint64_t channelNo, bool* isLocked)
{
    TvLogTrace();
    fflush(stdout);
    tvcontrol_return ret = TVControlFailed;
    bool lockChanged;
    lockChanged = false;
    ret = m_currentChannel->setParentalLock(isLocked, &lockChanged);
    if ((ret == TVControlSuccess) && (lockChanged)) {
        TVControlPushEvent(ParentalLockChanged, m_tunerData->tunerId.c_str(), Cleared, nullptr, ParentalLockOn, nullptr);
        return ret;
    }
}

} // namespace LinuxDVB
