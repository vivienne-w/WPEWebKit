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

#ifdef TVCONTROL_BACKEND_LINUX_DVB
#include "TunerBackend.h"
#include <libudev.h>
#endif

#include "tv-log.h"

#define TUNER_ID_LEN 3
#define TV_DEBUG 1
#define PARENTAL_LOCK_FILE "/root/TVParentLock.txt"

#define TVControlPushEvent(eventId, tunerId, evtOperation /*optional*/, pcState /*optional*/)                                              \
    \
{                                                                                                                               \
    struct wpe_tvcontrol_event* event = reinterpret_cast<struct wpe_tvcontrol_event*>(malloc(sizeof(struct wpe_tvcontrol_event))); \
    event->eventID = eventId;                                                                                                      \
    event->tuner_id.data = strndup(tunerId, TUNER_ID_LEN);                                                                         \
    event->tuner_id.length = strlen(tunerId);                                                                                      \
    TvLogInfo("Tuner id platform  =  %s \n", event->tuner_id.data);                                                                \
    event->operation = evtOperation;                                                                                               \
    event->parentalControl = pcState;                                                                                              \
    m_eventQueue.pushEvent(event);                                                                                                 \
    \
}

namespace BCMRPi {

struct TvControlBackend {
public:
    EventQueue<wpe_tvcontrol_event*> m_eventQueue;

    TvControlBackend(struct wpe_tvcontrol_backend*);
    virtual ~TvControlBackend();
    tvcontrol_return getTuners(struct wpe_tvcontrol_string_vector*);
    void getSupportedSourceTypesList(const char*, struct wpe_tvcontrol_src_types_vector*);
    tvcontrol_return getSourceList(const char*, struct wpe_tvcontrol_src_types_vector*);
    void getSignalStrength(const char*, double* signalStrength);
    tvcontrol_return startScanning(const char*, SourceType, bool isRescanned);
    tvcontrol_return stopScanning(const char*);
    tvcontrol_return setCurrentChannel(const char*, SourceType, uint64_t);
    tvcontrol_return setCurrentSource(const char*, SourceType);
    tvcontrol_return getChannels(const char*, SourceType, struct wpe_tvcontrol_channel_vector**);
    void updateTunerList(const char*, tuner_changed_operation);
    tvcontrol_return getPrograms(const char*, uint64_t, struct wpe_get_programs_options*, struct wpe_tvcontrol_program_vector**);
    tvcontrol_return getCurrentProgram(const char*, uint64_t, struct wpe_tvcontrol_program**);
    void isParentalControlled(bool*);
    tvcontrol_return setParentalControl(const char*, bool*);
    tvcontrol_return setParentalControlPin(const char*, const char*);
    tvcontrol_return setParentalLock(const char*, uint64_t, const char*, bool*);
    void isParentalLocked(const char*, uint64_t, bool*);

private:
    struct wpe_tvcontrol_backend* m_backend;
    uint64_t m_tunerCount;
    wpe_tvcontrol_string* m_strPtr;
    std::thread m_eventThread;
    std::thread m_tunerThread;
    bool m_isRunning;
    bool m_isEventProcessing;
    bool m_isParentalControlled;
    std::string m_parentalControlPin;

    void eventProcessor();
    void tunerChangedListener();
    void initializeTuners();
    void createTunerId(int, int, std::string&);

    void handleTunerChangedEvent(struct wpe_tvcontrol_event*);
    void handleSourceChangedEvent(struct wpe_tvcontrol_event*);
    void handleChannelChangedEvent(struct wpe_tvcontrol_event*);
    void handleParentalControlChangedEvent(struct wpe_tvcontrol_event*);
    void handleScanningStateChangedEvent(struct wpe_tvcontrol_event*);
    void handleParentalLockChangedEvent(struct wpe_tvcontrol_event*);

#ifdef TVCONTROL_BACKEND_LINUX_DVB
    Country m_country;
    std::vector<std::unique_ptr<LinuxDVB::TvTunerBackend>> m_tunerList;
    void getTunner(const char*, LinuxDVB::TvTunerBackend**);
#endif
};

TvControlBackend::TvControlBackend(struct wpe_tvcontrol_backend* backend)
    : m_backend(backend)
    , m_strPtr(nullptr)
    , m_tunerCount(0)
    , m_isParentalControlled(false)
{
    TvLogTrace();
    m_isEventProcessing = true;
    m_isRunning = true;

    initializeTuners();
    TvLogTrace();
    m_eventThread = std::thread(&TvControlBackend::eventProcessor, this);
    m_tunerThread = std::thread(&TvControlBackend::tunerChangedListener, this);

    char passKey[10];
    std::ifstream infile;
    infile.open(PARENTAL_LOCK_FILE);
    TvLogTrace();
    if (infile) {
        infile >> passKey;
        m_parentalControlPin = passKey; // Set the pin as the one previously saved by the user.
        infile.close();
    } else
        m_parentalControlPin = "Metro123#"; // Set the pin to it's default value if the key file is now found.
}

TvControlBackend::~TvControlBackend()
{
    TvLogTrace();
    /*Clear tuner id list*/
    for (uint64_t i = 0; i < m_tunerCount; i++) {
        free(m_strPtr[i].data);
        m_strPtr[i].data = nullptr;
    }
    free(m_strPtr);
    m_strPtr = nullptr;
    m_isEventProcessing = false;
    m_isRunning = false;
    m_tunerThread.join();
    m_eventThread.join();

#ifdef TVCONTROL_BACKEND_LINUX_DVB
    /*Clear private tuner  list*/
    m_isRunning = false;
    m_tunerList.clear();
#endif
}

void TvControlBackend::eventProcessor()
{
    TvLogTrace();
    while (m_isEventProcessing) {
        wpe_tvcontrol_event* event = m_eventQueue.getEvents();
        if (!event)
            continue;
        switch (event->eventID) {
        case TunerChanged:
            handleTunerChangedEvent(event);
            break;
        case SourceChanged:
            handleSourceChangedEvent(event);
            break;
        case ChannelChanged:
            handleChannelChangedEvent(event);
            break;
        case ScanningChanged:
            handleScanningStateChangedEvent(event);
            break;
        case ParentalControlChanged:
            handleParentalControlChangedEvent(event);
            break;
        case ParentalLockChanged:
            handleParentalLockChangedEvent(event);
            break;
        default:
            TvLogInfo("Unknown Event\n");
        }
        free(event);
    }
    TvLogTrace();
}

void TvControlBackend::initializeTuners()
{
    TvLogTrace();

#ifdef TVCONTROL_BACKEND_LINUX_DVB
    int feOpenMode = 0;

    m_tunerCount = 0;
    for (uint64_t i = 0; i < DVB_ADAPTER_SCAN; i++) {
        for (uint64_t j = 0; j < DVB_ADAPTER_SCAN; j++) {

            struct dvbfe_handle* feHandleTmp = dvbfe_open(i, j, feOpenMode);
            if (!feHandleTmp)
                continue;

            std::unique_ptr<TunerData> tunerData = std::make_unique<TunerData>();

            std::string tunerIdStr;
            createTunerId(i, j, tunerIdStr);
#ifdef TV_DEBUG
            TvLogInfo("Tuner identified as  %s \n Adapter: %d Frontend: %d \n ", feHandleTmp->name, i, j);
            TvLogInfo("Tuner id %s \n", tunerIdStr.c_str());
#endif
            tunerData->tunerId.assign(tunerIdStr);
            dvbfe_close(feHandleTmp);
            std::unique_ptr<LinuxDVB::TvTunerBackend> tInfo = std::make_unique<LinuxDVB::TvTunerBackend>(&m_eventQueue, m_tunerCount, std::move(tunerData));
            /*Update the  private tuner list*/
            m_tunerList.push_back(std::move(tInfo));
            m_tunerCount++;
            TvLogTrace();
        }
    }
#endif
    TvLogTrace();
}

tvcontrol_return TvControlBackend::getChannels(const char* tunerId, SourceType type, struct wpe_tvcontrol_channel_vector** channelVector)
{
    tvcontrol_return ret = TVControlFailed;
    TvLogTrace();
#ifdef TVCONTROL_BACKEND_LINUX_DVB
    LinuxDVB::TvTunerBackend* tuner;
    getTunner(tunerId, &tuner);
    ret = tuner->getChannels(type, channelVector);
#endif
    TvLogTrace();
    return ret;
}

tvcontrol_return TvControlBackend::getPrograms(const char* tunerId, uint64_t serviceId, struct wpe_get_programs_options* programsOptions, struct wpe_tvc
{
    tvcontrol_return ret = TVControlFailed;
    TvLogTrace();
#ifdef TVCONTROL_BACKEND_LINUX_DVB
    LinuxDVB::TvTunerBackend* tuner;
    getTunner(tunerId, &tuner);
    ret = tuner->getPrograms(serviceId, programsOptions, programVector);
#endif
    TvLogTrace();
    fflush(stdout);
    return ret;
}

tvcontrol_return TvControlBackend::getCurrentProgram(const char* tunerId, uint64_t serviceId, struct wpe_tvcontrol_program** program)
{
    tvcontrol_return ret = TVControlFailed;
    TvLogTrace();
#ifdef TVCONTROL_BACKEND_LINUX_DVB
    LinuxDVB::TvTunerBackend* tuner;
    getTunner(tunerId, &tuner);
    ret = tuner->getCurrentProgram(serviceId, program);
#endif
    TvLogTrace();
    return ret;
}

void TvControlBackend::createTunerId(int i, int j, std::string& tunerId)
{
    TvLogTrace();
    tunerId.assign(std::to_string(i));
    tunerId.append(":"); // delimiter
    tunerId.append(std::to_string(j));
    TvLogInfo("Tuner id %s \n", tunerId.c_str());
}

void TvControlBackend::handleTunerChangedEvent(struct wpe_tvcontrol_event* event)
{
    wpe_tvcontrol_backend_dispatch_tuner_event(m_backend, event);
    if (event->tuner_id.data)
        free(event->tuner_id.data);
}

void TvControlBackend::handleSourceChangedEvent(struct wpe_tvcontrol_event* event)
{
    wpe_tvcontrol_backend_dispatch_source_event(m_backend, event);
    if (event->tuner_id.data)
        free(event->tuner_id.data);
}

void TvControlBackend::handleChannelChangedEvent(struct wpe_tvcontrol_event* event)
{
    wpe_tvcontrol_backend_dispatch_channel_event(m_backend, event);
    if (event->tuner_id.data)
        free(event->tuner_id.data);
}

void TvControlBackend::handleScanningStateChangedEvent(struct wpe_tvcontrol_event* event)
{
    wpe_tvcontrol_backend_dispatch_scanning_state_event(m_backend, event);

    if (event->tuner_id.data)
        free(event->tuner_id.data); // free channel info
    if (event->channel_info) {
        if (event->channel_info->name)
            free(event->channel_info->name);
        if (event->channel_info)
            free(event->channel_info);
    }
}

void TvControlBackend::handleParentalControlChangedEvent(struct wpe_tvcontrol_event* event)
{
    wpe_tvcontrol_backend_dispatch_parental_control_event(m_backend, event);
    if (event->tuner_id.data)
        free(event->tuner_id.data);
}

void TvControlBackend::handleParentalLockChangedEvent(struct wpe_tvcontrol_event* event)
{
    wpe_tvcontrol_backend_dispatch_parental_lock_event(m_backend, event);
    if (event->tuner_id.data)
        free(event->tuner_id.data);
}

tvcontrol_return TvControlBackend::getTuners(struct wpe_tvcontrol_string_vector* outTunerList)
{
    TvLogTrace();
    tvcontrol_return ret = TVControlFailed;
    if (!m_strPtr && m_tunerCount) {
        /*Create an array of  tuner id struct */
        ret = TVControlSuccess;
        m_strPtr = (wpe_tvcontrol_string*)new wpe_tvcontrol_string[m_tunerCount];
#ifdef TVCONTROL_BACKEND_LINUX_DVB
        /*Iterate  private tuner list and update the created array  */
        int i = 0;
        for (auto& element : m_tunerList) {
            TvLogTrace();
            m_strPtr[i].data = strdup(element->m_tunerData->tunerId.c_str());
            TvLogInfo("\n ID = %s \n", element->m_tunerData->tunerId.c_str());
            m_strPtr[i].length = element->m_tunerData->tunerId.length();
            i++;
        }
#endif
    }
#ifdef TV_DEBUG
    TvLogInfo("Number of  tuners mtc = ");
    TvLogInfo("%" PRIu64 "\n", m_tunerCount);
#endif
    /* Update number of tuners and tuner id */
    outTunerList->length = m_tunerCount;
    outTunerList->strings = m_strPtr;

#ifdef TV_DEBUG
    TvLogTrace();
    TvLogInfo("Number of tuners = ");
    TvLogInfo("%" PRIu64 "\n", outTunerList->length);
    for (int i = 0; i < outTunerList->length; i++)
        TvLogInfo("%d th tuner id  = %s \n ", (i + 1), outTunerList->strings[i].data);
#endif
    return ret;
}

void TvControlBackend::getSupportedSourceTypesList(const char* tunerId,
    struct wpe_tvcontrol_src_types_vector* outSourceTypesList)
{
    TvLogTrace();
#ifdef TVCONTROL_BACKEND_LINUX_DVB
    /* Get the tuner instance*/
    LinuxDVB::TvTunerBackend* curTuner;
    getTunner(tunerId, &curTuner);
    /* Invoke get supported type list of the particular tuner instance and get the data*/
    curTuner->getSupportedSrcTypeList(outSourceTypesList);
#endif
    TvLogTrace();
}

#ifdef TVCONTROL_BACKEND_LINUX_DVB
void TvControlBackend::getTunner(const char* tunerId, LinuxDVB::TvTunerBackend** tuner)
{

    TvLogTrace();
    /*Iterate  private tuner list and get the particular tuner info  */
    for (auto& element : m_tunerList) {
#ifdef TV_DEBUG
        TvLogInfo("Id of this tuner %s \n", element->m_tunerData->tunerId.c_str());
        TvLogInfo("Id of required tuner %s \n", tunerId);
#endif
        if (!strncmp(element->m_tunerData->tunerId.c_str(), tunerId, 3)) {
            *tuner = element.get();
            TvLogInfo("Got Tuner");
        }
    }
}
#endif

tvcontrol_return TvControlBackend::getSourceList(const char* tunerId, struct wpe_tvcontrol_src_types_vector* outSourceList)
{
    tvcontrol_return ret = TVControlFailed;
    TvLogTrace();
/* Get the tuner instance*/
#ifdef TVCONTROL_BACKEND_LINUX_DVB
    LinuxDVB::TvTunerBackend* curTuner;
    getTunner(tunerId, &curTuner);
    /* Invoke get available source type list of the particular tuner and get the data */
    ret = curTuner->getSupportedSrcTypeList(outSourceList);
#endif
    TvLogTrace();
    return ret;
}

void TvControlBackend::getSignalStrength(const char* tunerId, double* signalStrength)
{
    TvLogTrace();
#ifdef TVCONTROL_BACKEND_LINUX_DVB
    LinuxDVB::TvTunerBackend* tuner;
    getTunner(tunerId, &tuner);
    tuner->getSignalStrength(signalStrength);
#endif
    TvLogTrace();
}

tvcontrol_return TvControlBackend::startScanning(const char* tunerId, SourceType type, bool isRescanned)
{
    tvcontrol_return ret = TVControlFailed;
    TvLogTrace();
#ifdef TVCONTROL_BACKEND_LINUX_DVB
    LinuxDVB::TvTunerBackend* tuner;
    getTunner(tunerId, &tuner);
    tuner->setSrcType(type);
    ret = tuner->startScanning(isRescanned);
#endif
    TvLogTrace();
    return ret;
}

tvcontrol_return TvControlBackend::stopScanning(const char* tunerId)
{
    tvcontrol_return ret = TVControlFailed;
    TvLogTrace();
#ifdef TVCONTROL_BACKEND_LINUX_DVB
    LinuxDVB::TvTunerBackend* tuner;
    getTunner(tunerId, &tuner);
    ret = tuner->stopScanning();
#endif
    TvLogTrace();
    return ret;
}

tvcontrol_return TvControlBackend::setCurrentChannel(const char* tunerId, SourceType type, uint64_t channelNumber)
{
    tvcontrol_return ret = TVControlFailed;
    TvLogTrace();
#ifdef TVCONTROL_BACKEND_LINUX_DVB
    LinuxDVB::TvTunerBackend* tuner;
    getTunner(tunerId, &tuner);
    ret = tuner->setCurrentChannel(type, channelNumber);
#endif
    TvLogTrace();
    return ret;
}

tvcontrol_return TvControlBackend::setCurrentSource(const char* tunerId, SourceType sType)
{
    tvcontrol_return ret = TVControlFailed;
    TvLogTrace();
#ifdef TVCONTROL_BACKEND_LINUX_DVB
    LinuxDVB::TvTunerBackend* tuner;
    getTunner(tunerId, &tuner);
    ret = tuner->setCurrentSource(sType);

    TVControlPushEvent(SourceChanged, tunerId, (tuner_changed_operation)0 /*Not relevant*/, (parental_control_state)0 /*Not Relevant*/);
    TvLogTrace();

#endif
    TvLogTrace();
    return ret;
}

/* TunerChangedListener waits for the addition/removal of Tuner and triggers the event*/
void TvControlBackend::tunerChangedListener()
{
#ifdef TVCONTROL_BACKEND_LINUX_DVB
    struct udev* udev;

    /* Create the udev object */
    udev = udev_new();
    if (!udev) {
        TvLogInfo("Can't create udev\n");
        exit(1);
    }

    /* Create a list of the devices in the 'dvb' subsystem.(Initially found Tuners) */
    struct udev_enumerate* enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "dvb");
    udev_enumerate_scan_devices(enumerate);
    struct udev_list_entry* devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry* devListEntry;
    udev_list_entry_foreach(devListEntry, devices)
    {
        const char* path = udev_list_entry_get_name(devListEntry);
        struct udev_device* dev = udev_device_new_from_syspath(udev, path);

        TvLogInfo("Device Node Path: %s\n", udev_device_get_devnode(dev));

        dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
        if (!dev) {
            TvLogInfo("Unable to find parent usb device.");
            exit(1);
        }
    }
    struct udev_monitor* mon = udev_monitor_new_from_netlink(udev, "udev");
    udev_monitor_filter_add_match_subsystem_devtype(mon, "dvb", nullptr);
    udev_monitor_enable_receiving(mon);
    int fd = udev_monitor_get_fd(mon);
    // Loop and wait for new Tuners.
    while (m_isRunning) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        int ret = select(fd + 1, &fds, nullptr, nullptr, &tv);

        if (ret > 0 && FD_ISSET(fd, &fds)) {
            TvLogInfo("\nselect() says there should be data\n");
            struct udev_device* dev = udev_monitor_receive_device(mon);
            if (dev) {
                TvLogTrace();
                std::string fpath = udev_device_get_devnode(dev);
                if (fpath.find("frontend") != std::string::npos) {
                    int i, j;
                    std::string tunerId;
                    sscanf(udev_device_get_devnode(dev), "/%*[a-z-A-Z]/%*[a-z-A-Z]/%*[a-z-A-Z]%d/%*[a-z-A-Z]%d", &i, &j);
                    createTunerId(i, j, tunerId);
                    TvLogTrace();

                    // The data from udev is in string format. So.converting the operation into the appropriate data type.
                    tuner_changed_operation operation;
                    if (!strcmp("add", udev_device_get_action(dev)))
                        operation = Added;
                    else if (!strcmp("remove", udev_device_get_action(dev)))
                        operation = Removed;
                    updateTunerList(tunerId.c_str(), operation);
                    TVControlPushEvent(TunerChanged, tunerId.c_str(), operation, (parental_control_state)0 /*Not Relevant*/);
                    TvLogTrace();
                }
                TvLogInfo("***Tuner***\n");
                TvLogInfo("   Node: %s\n", udev_device_get_devnode(dev));
                TvLogInfo("   Action: %s\n", udev_device_get_action(dev));
                udev_device_unref(dev);
            } else
                TvLogInfo("No Device from receive_device(). An error occured.\n");
        }
        usleep(250 * 1000);
    }
    udev_unref(udev);
#endif
}

void TvControlBackend::updateTunerList(const char* tunerId, tuner_changed_operation operation)
{
#ifdef TVCONTROL_BACKEND_LINUX_DVB
    if (operation == Added) {
        std::unique_ptr<TunerData> tunerData = std::make_unique<TunerData>();
        tunerData->tunerId.assign(tunerId);
        std::unique_ptr<LinuxDVB::TvTunerBackend> tInfo = std::make_unique<LinuxDVB::TvTunerBackend>(&m_eventQueue, m_tunerCount, std::move(tunerData));
        m_tunerList.push_back(std::move(tInfo));
        TvLogTrace();
        TvLogInfo("Found and Added the Tuner:-)");
        m_tunerCount += 1;
    } else if (operation == Removed) {
        int position = 0;
        /*Iterate  private tuner list and get the particular tuner info  */
        for (auto& element : m_tunerList) {
            TvLogInfo("Id of this tuner %s \n", element->m_tunerData->tunerId.c_str());
            TvLogInfo("Id of required tuner %s \n", tunerId);

            if (!strncmp(element->m_tunerData->tunerId.c_str(), tunerId, element->m_tunerData->tunerId.length())) {
                m_tunerList.erase(m_tunerList.begin() + position);
                TvLogInfo("Found and deleted the Tuner:-))");
                break;
            }
            position++;
        }
    }
#endif
}

void TvControlBackend::isParentalControlled(bool* lockStatus)
{
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    fflush(stdout);
    *lockStatus = m_isParentalControlled ? true : false;
}

tvcontrol_return TvControlBackend::setParentalControl(const char* pin, bool* isLocked)
{
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    if (!strcmp(pin, m_parentalControlPin.c_str())) {
        if (*isLocked != m_isParentalControlled) {
            m_isParentalControlled = *isLocked;
            TVControlPushEvent(ParentalControlChanged, ""/*Not relevant*/, (tuner_changed_operation)0,  ((parental_control_state)((int)m_isParentalControlled)));
        }
        return TVControlSuccess;
    }
    return TVControlFailed;
}

tvcontrol_return TvControlBackend::setParentalControlPin(const char* oldPin, const char* newPin)
{
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    if (!strcmp(oldPin, m_parentalControlPin.c_str())) {
        m_parentalControlPin = newPin;
        std::ofstream outfile;
        outfile.open(PARENTAL_LOCK_FILE, std::ios::trunc);
        outfile << newPin;
        outfile.close();
        return TVControlSuccess;
    }
    return TVControlFailed;
}

tvcontrol_return TvControlBackend::setParentalLock(const char* tunerId, uint64_t channelNumber, const char* pin, bool* isParentalLocked)
{
    tvcontrol_return ret = TVControlFailed;
    if ((m_isParentalControlled) && (!strcmp(pin, m_parentalControlPin.c_str()))) {
        printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
        LinuxDVB::TvTunerBackend* tuner;
        getTunner(tunerId, &tuner);
        ret = tuner->setParentalLock(channelNumber, isParentalLocked);
    }
    return ret;
}

void TvControlBackend::isParentalLocked(const char* tunerId, uint64_t channelNumber, bool* isParentalLocked)
{
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    LinuxDVB::TvTunerBackend* tuner;
    getTunner(tunerId, &tuner);
    tuner->isParentalLocked(channelNumber, isParentalLocked);
    return;
}

} // namespace BCMRPi

extern "C" {

__attribute__((visibility("default")))
struct wpe_tvcontrol_backend_interface bcm_rpi_tvcontrol_backend_interface = {
    // create
    [](struct wpe_tvcontrol_backend* backend) -> void*
    {
        TvLogTrace();
        return new BCMRPi::TvControlBackend(backend);
    },
    // destroy
    [](void* data)
    {
        TvLogTrace();
        auto* backend = static_cast<BCMRPi::TvControlBackend*>(data);
        delete backend;
    },
    // get_tuner_list
    [](void* data, struct wpe_tvcontrol_string_vector* out_tuner_list) -> tvcontrol_return
    {
        TvLogTrace();
        auto& backend = *static_cast<BCMRPi::TvControlBackend*>(data);
        return backend.getTuners(out_tuner_list);
    },
    // get_supported_source_types_list
    [](void* data, const char* tuner_id, struct wpe_tvcontrol_src_types_vector* out_source_types_list)
    {
        TvLogTrace();
        auto& backend = *static_cast<BCMRPi::TvControlBackend*>(data);
        backend.getSupportedSourceTypesList(tuner_id, out_source_types_list);
    },
    // get_source_list
    [](void* data, const char* tuner_id, struct wpe_tvcontrol_src_types_vector* out_source_list) -> tvcontrol_return
    {
        TvLogTrace();
        auto& backend = *static_cast<BCMRPi::TvControlBackend*>(data);
        return backend.getSourceList(tuner_id, out_source_list);
    },
    // get_signal_strength
    [](void* data, const char* tuner_id, double* out_signal_strength)
    {
        TvLogTrace();
        auto& backend = *static_cast<BCMRPi::TvControlBackend*>(data);
        backend.getSignalStrength(tuner_id, out_signal_strength);
    },
    // start_scanning
    [](void* data, const char* tuner_id, SourceType type, bool isRescanned) -> tvcontrol_return
    {
        TvLogTrace();
        auto& backend = *static_cast<BCMRPi::TvControlBackend*>(data);
        return backend.startScanning(tuner_id, type, isRescanned);
    },
    // stop_scanning
    [](void* data, const char* tuner_id) -> tvcontrol_return
    {
        TvLogTrace();
        auto& backend = *static_cast<BCMRPi::TvControlBackend*>(data);
        return backend.stopScanning(tuner_id);
    },
    // set_current_channel
    [](void* data, const char* tuner_id, SourceType type, uint64_t channel_number) -> tvcontrol_return
    {
        TvLogTrace();
        auto& backend = *static_cast<BCMRPi::TvControlBackend*>(data);
        return backend.setCurrentChannel(tuner_id, type, channel_number);
    },
    // set current source
    [](void* data, const char* tuner_id, SourceType type) -> tvcontrol_return
    {
        TvLogTrace();
        auto& backend = *static_cast<BCMRPi::TvControlBackend*>(data);
        return backend.setCurrentSource(tuner_id, type);
    },
    // get_channel_list
    [](void* data, const char* tuner_id, SourceType type, struct wpe_tvcontrol_channel_vector** out_channel_list) -> tvcontrol_return
    {
        tvcontrol_return ret = TVControlFailed;
        TvLogTrace();
        auto& backend = *static_cast<BCMRPi::TvControlBackend*>(data);
        return backend.getChannels(tuner_id, type, out_channel_list);
    },
    // get_program_list
    [](void* data, const char* tuner_id, uint64_t service_id, struct wpe_get_programs_options* programs_options, struct wpe_tvcontrol_program_vector** o
    {
        tvcontrol_return ret = TVControlFailed;
        TvLogTrace();
        fflush(stdout);
        auto& backend = *static_cast<BCMRPi::TvControlBackend*>(data);
        return backend.getPrograms(tuner_id, service_id, programs_options, out_program_list);
    },
    // get_current_program
    [](void* data, const char* tuner_id, uint64_t service_id, struct wpe_tvcontrol_program** program) -> tvcontrol_return
    {
        tvcontrol_return ret = TVControlFailed;
        TvLogTrace();
        fflush(stdout);
        auto& backend = *static_cast<BCMRPi::TvControlBackend*>(data);
        return backend.getCurrentProgram(tuner_id, service_id, program);
    },
    // is_parental_controlled
    [](void* data, bool* is_parental_controlled)
    {
        TvLogTrace();
        fflush(stdout);
        auto& backend = *static_cast<BCMRPi::TvControlBackend*>(data);
        backend.isParentalControlled(is_parental_controlled);
    },
    // set_parental_control
    [](void* data, const char* pin, bool* is_locked) -> tvcontrol_return
    {
        TvLogTrace();
        auto& backend = *static_cast<BCMRPi::TvControlBackend*>(data);
        return backend.setParentalControl(pin, is_locked);
    },
    // set_parental_control_pin
    [](void* data, const char* old_pin, const char* new_pin) -> tvcontrol_return
    {
        TvLogTrace();
        auto& backend = *static_cast<BCMRPi::TvControlBackend*>(data);
        return backend.setParentalControlPin(old_pin, new_pin);
    },
    // set_parental_lock
    [](void* data, const char* tuner_id, uint64_t channel_number, const char* pin, bool* is_locked) -> tvcontrol_return
    {
        TvLogTrace();
        auto& backend = *static_cast<BCMRPi::TvControlBackend*>(data);
        return backend.setParentalLock(tuner_id, channel_number, pin, is_locked);
    },
    // is_parental_locked
    [](void* data, const char* tuner_id, uint64_t channel_number, bool* is_parental_locked)
    {
        TvLogTrace();
        auto& backend = *static_cast<BCMRPi::TvControlBackend*>(data);
        backend.isParentalLocked(tuner_id, channel_number, is_parental_locked);
    },
};
}
