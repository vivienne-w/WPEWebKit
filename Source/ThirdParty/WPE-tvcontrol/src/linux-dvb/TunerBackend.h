
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

#ifndef TUNER_BACKEND_H_
#define TUNER_BACKEND_H_

#include "SourceBackend.h"

#include <math.h>
#include <sstream>

#define DVB_ADAPTER_SCAN 6
#define DVB_FE_SCAN 6
#define DVB_MAX_TUNER 6
#define FE_STATUS_PARAMS (DVBFE_INFO_LOCKSTATUS | DVBFE_INFO_FEPARAMS | DVBFE_INFO_SIGNAL_STRENGTH | DVBFE_INFO_BER | DVBFE_INFO_SNR | DVBFE_INFO_UNCORRECTED_BLOCKS)
#define CONFIGFILE "TVConfig.txt"
typedef std::map<std::string, std::string> ConfigInfo;

namespace LinuxDVB {
class TvTunerBackend {
public:
    TvTunerBackend(EventQueue<wpe_tvcontrol_event*>*, int, std::unique_ptr<TunerData>);
    virtual ~TvTunerBackend();

    SourceType getSrcType() { return m_sType; };
    void setSrcType(SourceType sType) { m_sType = sType; };
    void getSignalStrength(double*);
    tvcontrol_return startScanning(bool);
    tvcontrol_return stopScanning();
    tvcontrol_return setCurrentChannel(SourceType, uint64_t);
    tvcontrol_return setCurrentSource(SourceType);
    tvcontrol_return getChannels(SourceType, struct wpe_tvcontrol_channel_vector**);
    tvcontrol_return getSupportedSrcTypeList(wpe_tvcontrol_src_types_vector*);
    tvcontrol_return getPrograms(uint64_t, struct wpe_get_programs_options*, struct wpe_tvcontrol_program_vector**);
    tvcontrol_return getCurrentProgram(uint64_t, struct wpe_tvcontrol_program**);
    void isParentalLocked(uint64_t, bool*);
    tvcontrol_return setParentalLock(uint64_t, bool*);

    std::unique_ptr<struct TunerData> m_tunerData;
    std::vector<std::unique_ptr<SourceBackend>> m_sourceList; // List of source objects

private:
    ChannelList m_channel;
    SourceType m_sType;
    SourceType* m_srcTypeListPtr;
    uint64_t m_supportedSysCount;
    wpe_tvcontrol_src_types_vector m_srcList; // List of src type
    ConfigInfo m_configValues;
    EventQueue<wpe_tvcontrol_event*>* m_eventQueue;

    int baseOffset(int, int);
    int freqStep(int, int);
    int getSupportedSourcesTypeList(wpe_tvcontrol_src_types_vector*);
    void getAvailableSrcList(wpe_tvcontrol_src_types_vector*);
    void initializeSourceList();
    void getSources();
    void getSource(SourceType, SourceBackend**);
    void getSourceType(SourceType, fe_delivery_system*);
    void configureTuner(std::string&);
    void setModulation(std::string&);
    void configureTuner(int);
    void getConfiguration();
    void getCapabilities();
    void populateFreq();
    void clearSourceList();
};

} // namespace LinuxDVB

#endif // TUNER_BACKEND_H_
