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

#pragma once

#if ENABLE(TV_CONTROL)

#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class PlatformTVControlBackend;
class PlatformTVChannel;

class PlatformTVProgram : public RefCounted<PlatformTVProgram> {
public:
    static RefPtr<PlatformTVProgram> create(PlatformTVControlBackend*, String);
    ~PlatformTVProgram();

    bool getAudioLanguages(Vector<String>&);
    bool getSubtitleLanguages(Vector<String>&);
    bool getGenres(Vector<String>&);

    const String eventId() const { return m_eventId; }
    const String title() const { return m_title; }
    const unsigned long long& startTime() const { return m_startTime; }
    const unsigned long long& duration() const { return m_duration; }
    const String shortDescription() const { return m_shortDescription; }
    const String longDescription() const { return m_longDescription; }
    const String rating() const { return m_rating; }
    const String seriesId() const { return m_seriesId; }
    const String serviceId() const { return m_serviceId; }

private:
    PlatformTVProgram(PlatformTVControlBackend*, String);

    String m_tunerId;
    String m_eventId;
    String m_title;
    unsigned long long m_startTime;
    unsigned long long m_duration;
    String m_shortDescription;
    String m_longDescription;
    String m_rating;
    String m_seriesId;
    String m_serviceId;
    PlatformTVControlBackend* m_tvBackend;
};

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)
