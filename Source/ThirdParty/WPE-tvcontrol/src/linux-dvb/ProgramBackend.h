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

#ifndef PROGRAM_BACKEND_H_
#define PROGRAM_BACKEND_H_

#include <string>
#include <wpe/tvcontrol-backend.h>

namespace LinuxDVB {

class ProgramBackend {

public:
    ProgramBackend() = default;
    ~ProgramBackend() = default;

    void setEventId(uint64_t eventId) { m_eventId = eventId; }
    void setStartTime(uint64_t startTime) { m_startTime = startTime; }
    void setDuration(uint64_t duration) { m_duration = duration; }
    void setShortDescription(uint64_t shortDescription) { m_shortDescription = shortDescription; }
    void setLongDescription(std::string longDescription) { m_longDescription = longDescription; }
    void setRating(uint64_t rating) { m_rating = rating; }
    void setTitle(std::string title) { m_title = title; }
    void setMsgLen(int msgLen) { m_msgLen = msgLen; }

    uint64_t getEventId() { return m_eventId; }
    uint64_t getStartTime() { return m_startTime; }
    uint64_t getDuration() { return m_duration; }
    std::string getTitle() { return m_title; }
    std::string getLongDescription() { return m_longDescription; }
    uint64_t* getDurationAddress() { return &m_duration; }
    int getMsgLen() { return m_msgLen; }

private:
    uint64_t m_duration;
    uint64_t m_eventId;
    std::string m_title;
    uint64_t m_startTime;
    uint64_t m_shortDescription;
    std::string m_longDescription;
    uint64_t m_rating;
    bool m_isFree;
    int m_msgLen;

};
} // namespace LinuxDVB

#endif // CHANNEL_BACKEND_H_
