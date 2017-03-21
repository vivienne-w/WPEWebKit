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

#include "Event.h"
#include "JSTVChannel.h"

namespace WebCore {

class TVEmergencyAlertedEvent : public Event {
public:

    static Ref<TVEmergencyAlertedEvent> create(const AtomicString&, String, String, String, RefPtr<TVChannel>, String, Vector<String>);
    ~TVEmergencyAlertedEvent();

    const String type() const { return m_type; }
    const String severityLevel() const { return m_severity; }
    const String description() const { return m_description; }
    RefPtr<TVChannel> channel() const { return m_channel; }
    const String url() const { return m_url; }

    const Vector<String>& getRegions() const { return m_regionList; }

    virtual EventInterface eventInterface() const { return TVEmergencyAlertedEventInterfaceType; }

private:
    TVEmergencyAlertedEvent(const AtomicString&, String, String, String, RefPtr<TVChannel>, String, Vector<String>);
    String m_type;
    String m_severity;
    String m_description;
    RefPtr<TVChannel> m_channel;
    String m_url;
    Vector<String> m_regionList;
};

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)
