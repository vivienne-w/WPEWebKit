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

#ifndef CHANNEL_BACKEND_H_
#define CHANNEL_BACKEND_H_

#include "ProgramBackend.h"
#include "TVConfig.h"

#include <fstream>
#include <libdvbapi/dvbdemux.h>
#include <libucsi/atsc/section.h>
#include <libucsi/dvb/section.h>
#include <poll.h>
#include <string.h>
#include <thread>
#include <wpe/tvcontrol-backend.h>

#define TV_DEBUG 1

struct atscEitSectionInfo {
    uint8_t sectionNum;
    uint8_t numEvents;
    uint8_t numEtms;
    uint8_t numReceivedEtms;
    LinuxDVB::ProgramBackend** events;
};

struct atscEitInfo {
    int numEitSections;
    struct atscEitSectionInfo *section;
};

namespace LinuxDVB {

class ChannelBackend {

public:
    ChannelBackend() = default;
    ~ChannelBackend();

    tvcontrol_return getPrograms(struct wpe_get_programs_options*, wpe_tvcontrol_program_vector**);
    tvcontrol_return getCurrentProgram(wpe_tvcontrol_program**);
    struct wpe_tvcontrol_program_vector m_programVector;
    struct wpe_tvcontrol_program* m_program;
    void clearProgramVector();
    void clearProgramList();
    void appendPrograms(ProgramBackend*);

    void setNetworkId(uint64_t networkId) { m_networkId = networkId; }
    void setTransportStreamId(uint64_t transportStreamId) { m_transportStreamId = transportStreamId; }
    void setServiceId(uint64_t serviceId) { m_serviceId = serviceId; }
    void setName(std::string name) { m_name = name; }
    void setNumber(uint64_t number) { m_number = number; }
    void setProgramNumber(int programNumber) { m_programNumber = programNumber; }
    void setFrequency(int frequency) { m_frequency = frequency; }
    void setNumEits(uint8_t numEits) { m_numEits = numEits; }
    void setEit(struct atscEitInfo *Eit) { m_eit = Eit; }
    void setLastEvent(ProgramBackend* lastEvent) { lastEvent = m_lastEvent; }
    void setEventInfoIndex(int eventInfoIndex) { m_eventInfoIndex = eventInfoIndex; }

    uint64_t getLCN() const { return m_number; }
    int getFrequency() const { return m_frequency; }
    int getProgramNumber() const { return m_programNumber; }
    std::string getName() const { return m_name; }
    uint64_t getServiceId() const { return m_serviceId; }
    uint64_t getTransportStreamId() const { return m_transportStreamId; }
    uint64_t getNetworkId() const { return m_networkId; }
    uint8_t getNumEits() const { return m_numEits; }
    struct atscEitInfo* getEit(int index) { return (m_eit + index); }
    ProgramBackend* getLastEvent() { return m_lastEvent; }
    int getEventInfoIndex() { return m_eventInfoIndex; }
    void isParentalLocked(bool*);
    tvcontrol_return setParentalLock(bool*, bool*);
private:
    std::string m_name;
    uint64_t m_networkId;
    uint64_t m_serviceId;
    uint64_t m_transportStreamId;
    uint64_t m_number;
    int m_programNumber;
    int m_frequency;

    bool m_isEmergency;
    bool m_isFree;
    bool m_isParentalLocked;
    uint8_t m_numEits;
    int m_eventInfoIndex;
    ProgramBackend* m_lastEvent;
    struct atscEitInfo *m_eit;
    std::vector<ProgramBackend*> m_programList;
};

} // namespace LinuxDVB

#endif // CHANNEL_BACKEND_H_
