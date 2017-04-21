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

#include "config.h"
#include "TVChannel.h"

#if ENABLE(TV_CONTROL)

#include "ExceptionCode.h"

namespace WebCore {

Ref<TVChannel> TVChannel::create(ScriptExecutionContext* context, RefPtr<PlatformTVChannel> platformTVChannel, TVSource* parentTVSource)
{
    return adoptRef(*new TVChannel(context, platformTVChannel, parentTVSource));
}

TVChannel::TVChannel(ScriptExecutionContext* context, RefPtr<PlatformTVChannel> platformTVChannel, TVSource* parentTVSource)
    : ContextDestructionObserver(context)
    , m_platformTVChannel(platformTVChannel)
    , m_parentTVSource(parentTVSource)
{
}

void TVChannel::getPrograms(const GetProgramsOptions& programsOptions, TVProgramPromise&& promise)
{
    if (m_platformTVChannel) {
        Vector<RefPtr<PlatformTVProgram>> programVector;
        printf("programOptions.startTime = %llu programOptions.endTime = %llu \n", programsOptions.startTime, programsOptions.endTime);
        if (!m_platformTVChannel->getPrograms(programsOptions.startTime, programsOptions.endTime, programVector)) {
            promise.reject(nullptr);
            return;
        }
        if (programVector.size()) {
            printf("%s:%s:%d:got %d programsssss\n", __FILE__, __func__, __LINE__, programVector.size());
            for (auto& program : programVector) {
                printf("    %s:%s \n", program->eventId().utf8().data(), program->title().utf8().data());
                m_programList.append(TVProgram::create(program, this));
            }
            promise.resolve(m_programList);
            return;
        }
    }

    promise.reject(nullptr);
}

void TVChannel::getCurrentProgram(TVPromise&& promise)
{
    if (m_platformTVChannel) {
        RefPtr<PlatformTVProgram> currentProgram;
        if (!m_platformTVChannel->getCurrentProgram(currentProgram)) {
            promise.reject(nullptr);
            return;
        }
        if (currentProgram) {
            printf("eventId of current program= %s\n", currentProgram->eventId().utf8().data());
            m_currentProgram = TVProgram::create(currentProgram, this);
            promise.resolve(m_currentProgram);
            return;
        }
        printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
    }
    promise.reject(nullptr);
}

void TVChannel::setParentalLock(const String& pin, bool isLocked, TVsetParentalLock&& promise)
{
    if (m_platformTVChannel->setParentalLock(pin, isLocked))
        promise.resolve(nullptr);
    else
        promise.reject(SECURITY_ERR, "Unmatched Pins!!!..Channel lock not set");
}

void TVChannel::dispatchParentalLockChangedEvent(uint16_t state)
{
    scriptExecutionContext()->postTask([=](ScriptExecutionContext&) {
        dispatchEvent(TVParentalLockChangedEvent::create(eventNames().parentallockchangedEvent, (TVParentalLockChangedEvent::State)state));
    });
}

ScriptExecutionContext* TVChannel::scriptExecutionContext() const
{
    return ContextDestructionObserver::scriptExecutionContext();
}

void TVChannel::contextDestroyed()
{
    ContextDestructionObserver::contextDestroyed();
}

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)
