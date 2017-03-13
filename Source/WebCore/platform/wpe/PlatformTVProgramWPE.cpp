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
#include "PlatformTVProgram.h"

#if ENABLE(TV_CONTROL)

#include "PlatformTVControl.h"
#include <wpe/tvcontrol-backend.h>

namespace WebCore {

RefPtr<PlatformTVProgram> PlatformTVProgram::create(PlatformTVControlBackend* tvBackend)
{
    return adoptRef(new PlatformTVProgram(tvBackend));
}

PlatformTVProgram::PlatformTVProgram(PlatformTVControlBackend* tvBackend)
    : m_tvBackend(tvBackend)
{
    m_eventId = (std::to_string(tvBackend->m_program->eventId)).c_str();
    m_title = tvBackend->m_program->title;
    m_startTime = tvBackend->m_program->startTime;
    m_duration = tvBackend->m_program->duration;
    m_shortDescription = (std::to_string(tvBackend->m_program->shortDescription)).c_str();
    m_longDescription = (std::to_string(tvBackend->m_program->longDescription)).c_str();
    m_rating  = (std::to_string(tvBackend->m_program->rating)).c_str();
    m_seriesId  = (std::to_string(tvBackend->m_program->seriesId)).c_str();
}

PlatformTVProgram::~PlatformTVProgram()
{
}



} // namespace WebCore

#endif // ENABLE(TV_CONTROL)
