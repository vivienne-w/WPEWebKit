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

#ifndef SOURCE_BACKEND_H_
#define SOURCE_BACKEND_H_

#include "ChannelBackend.h"
#include "TVConfig.h"
#include "event-queue.h"

#include <condition_variable>
#include <fstream>
#include <libdvbapi/dvbdemux.h>
#include <libdvbapi/dvbfe.h>
#include <libucsi/atsc/extended_channel_name_descriptor.h>
#include <libucsi/atsc/section.h>
#include <libucsi/atsc/types.h>
#include <libucsi/dvb/section.h>
#include <linux/dvb/dmx.h>
#include <mutex>
#include <poll.h>
#include <signal.h>
#include <thread>
#include <unistd.h>

#define TV_DEBUG 1

struct TunerData {
    std::string tunerId;
    uint64_t modulation;
    std::vector<long> frequency;
};

namespace BCMRPi {
class TvControlBackend;

class SourceBackend {
public:
    SourceBackend(EventQueue<wpe_tvcontrol_event*>*, SourceType, TunerData*);
    ~SourceBackend();

    tvcontrol_return startScanning(bool);
    tvcontrol_return stopScanning();
    tvcontrol_return setCurrentChannel(uint64_t);
    SourceType srcType() { return m_sType; }
    tvcontrol_return getChannels(wpe_tvcontrol_channel_vector**);

    wpe_tvcontrol_channel_vector m_channelVector;

private:
    void parseAtscExtendedChannelNameDescriptor(char**, const unsigned char*);
    void startPlayBack(int, uint64_t, int, int, int);
    void stopPlayBack();
    ChannelBackend* getChannelByLCN(uint64_t);
    bool tuneToFrequency(int, uint64_t, struct dvbfe_handle*);
    uint32_t getBits(const uint8_t*, int, int);
    void atscScan(int, uint64_t);
    void mpegScan(int, std::map<int, int>&);
    void dvbScan();
    bool processTVCT(int, int);
    bool processPAT(int, int, struct pollfd*, std::map<int, int>&);
    void processPMT(int, std::map<int, int>&);
    int createSectionFilter(uint16_t, uint8_t);
    void clearChannelList();
    void clearChannelVector();
    void scanningThread();
    void setCurrentChannelThread();

    std::map<uint64_t, ChannelBackend*> m_channelList;
    EventQueue<wpe_tvcontrol_event*>* m_eventQueue;

    SourceType m_sType;
    TunerData* m_tunerData;
    std::thread m_scanningThread;
    std::thread m_setCurrentChannelThread;
    int m_adapter;
    int m_demux;

    int m_scanIndex;
    bool m_isRescanned;
    bool m_isScanStopped;
    bool m_isRunning;
    bool m_isScanInProgress;
    std::mutex m_scanMutex;
    std::condition_variable_any m_scanCondition;
    pid_t m_pid;

    uint64_t m_channelNo;
    std::mutex m_channelChangeMutex;
    std::condition_variable_any m_channelChangeCondition;
};

} // namespace BCMRPi
#endif // SOURCE_BACKEND_H_
