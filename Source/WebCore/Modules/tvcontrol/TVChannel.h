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

#include "ActiveDOMObject.h"
#include "EventNames.h"
#include "EventTarget.h"
#include "JSDOMPromise.h"
#include "JSTVProgram.h"
#include "PlatformTVChannel.h"
#include "ScriptExecutionContext.h"
#include "TVParentalLockChangedEvent.h"
#include "TVProgram.h"

namespace WebCore {

class TVSource;

class TVChannel final : public RefCounted<TVChannel>, public ContextDestructionObserver, public EventTargetWithInlineData {
public:
    struct GetProgramsOptions {
        unsigned long long startTime;
        unsigned long long endTime;
    };

    static Ref<TVChannel> create(ScriptExecutionContext*, RefPtr<PlatformTVChannel>, TVSource*);
    ~TVChannel() = default;

    enum class Type {
        Tv,
        Radio,
        Data
    };

    typedef DOMPromise<TVProgramVector> TVProgramPromise;
    typedef DOMPromise<TVProgram> TVPromise;

    void getPrograms(const GetProgramsOptions&, TVProgramPromise&&);
    void getCurrentProgram(TVPromise&&);

    const String networkId() const { return m_platformTVChannel->networkId(); }
    const String transportStreamId() const { return m_platformTVChannel->transportStreamId(); }
    const String serviceId() const { return m_platformTVChannel->serviceId(); }
    TVSource* source() const { return m_parentTVSource; }
    Type type() const { return ((Type)m_platformTVChannel->type()); }
    const String name() const { return m_platformTVChannel->name(); }
    const String number() const { return m_platformTVChannel->number(); }
    bool isEmergency() const { return m_platformTVChannel->isEmergency(); }
    bool isFree() const { return true; }
    bool isParentalLocked() const { return m_platformTVChannel->isParentalLocked(); }

    using RefCounted<TVChannel>::ref;
    using RefCounted<TVChannel>::deref;

    typedef DOMPromise<std::nullptr_t> TVsetParentalLock;
    void setParentalLock(const String&, bool, TVsetParentalLock&&);

    void dispatchParentalLockChangedEvent(uint16_t);
private:
    explicit TVChannel(ScriptExecutionContext*, RefPtr<PlatformTVChannel>, TVSource*);

    RefPtr<PlatformTVChannel> m_platformTVChannel;
    TVSource* m_parentTVSource;

    Vector<RefPtr<TVProgram>> m_programList;
    RefPtr<TVProgram> m_currentProgram;

    void refEventTarget() override { ref(); }
    void derefEventTarget() override { deref(); }

    void contextDestroyed() final;
    EventTargetInterface eventTargetInterface() const override { return TVChannelEventTargetInterfaceType; }
    ScriptExecutionContext* scriptExecutionContext() const override;
};

typedef Vector<RefPtr<TVChannel> > TVChannelVector;

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)
