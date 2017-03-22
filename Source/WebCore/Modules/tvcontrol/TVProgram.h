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
#include "PlatformTVProgram.h"
#include "ScriptExecutionContext.h"

namespace WebCore {

class TVChannel;

class TVProgram final : public RefCounted<TVProgram>, public EventTargetWithInlineData {
public:
    static Ref<TVProgram> create(RefPtr<PlatformTVProgram>, TVChannel*);
    ~TVProgram() = default;

    const String eventId() const { return m_platformTVProgram->eventId(); }
    TVChannel* channel() const { return m_parentTVChannel; }
    const String title() const { return m_platformTVProgram->title(); }
    unsigned long long startTime() const { return m_platformTVProgram->startTime(); }
    unsigned long long duration() const { return  m_platformTVProgram->duration(); }
    const String shortDescription() const { return m_platformTVProgram->shortDescription(); }
    const String longDescription() const { return m_platformTVProgram->longDescription(); }
    const String rating() const { return m_platformTVProgram->rating(); }
    const String seriesId() const { return m_platformTVProgram->seriesId(); }
    bool isFree() const { return true; }

    using RefCounted<TVProgram>::ref;
    using RefCounted<TVProgram>::deref;

private:
    explicit TVProgram(RefPtr<PlatformTVProgram>, TVChannel*);

    RefPtr<PlatformTVProgram> m_platformTVProgram;
    TVChannel* m_parentTVChannel;
    void refEventTarget() override { ref(); }
    void derefEventTarget() override { deref(); }

    EventTargetInterface eventTargetInterface() const override { return TVProgramEventTargetInterfaceType; }
    ScriptExecutionContext* scriptExecutionContext() const override { return nullptr; }
};

typedef Vector<RefPtr<TVProgram>> TVProgramVector;
typedef Vector<String> TVStringVector;

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)
