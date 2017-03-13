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
#include "PlatformTVChannel.h"

#if ENABLE(TV_CONTROL)

#include "PlatformTVControl.h"
#include <wpe/tvcontrol-backend.h>

namespace WebCore {

RefPtr<PlatformTVChannel> PlatformTVChannel::create(PlatformTVControlBackend* tvBackend, String tunerId)
{
    return adoptRef(new PlatformTVChannel(tvBackend, tunerId));
}

PlatformTVChannel::PlatformTVChannel(PlatformTVControlBackend* tvBackend, String tunerId)
    : m_tunerId(tunerId)
    , m_isParentalLocked(false)
    , m_tvBackend(tvBackend)
{
    m_networkId = (std::to_string(tvBackend->m_channel->networkId)).c_str();
    m_transportStreamId = (std::to_string(tvBackend->m_channel->transportSId)).c_str();
    m_serviceId = (std::to_string(tvBackend->m_channel->serviceId)).c_str();
    m_name = tvBackend->m_channel->name;
    m_number = (std::to_string(tvBackend->m_channel->number)).c_str();
    m_type = (PlatformTVChannel::Type)tvBackend->m_channel->type;
    m_isParentalLocked = false;
}

PlatformTVChannel::~PlatformTVChannel()
{
}

bool PlatformTVChannel::getPrograms(unsigned long long startTime, unsigned long long endTime, Vector<RefPtr<PlatformTVProgram>>& programVector)
{
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    struct wpe_tvcontrol_program_vector* programList;
    struct wpe_get_programs_options programsOptions;
    programsOptions.startTime = startTime;
    programsOptions.endTime = endTime;
    tvcontrol_return ret = TVControlFailed;
    std::string::size_type sz = 0;
    ret = wpe_tvcontrol_backend_get_program_list(m_tvBackend->m_backend, m_tunerId.utf8().data(), std::stoull(m_serviceId.utf8().data(), &sz, 0), &programsOptions, &programList);
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    fflush(stdout);
    if (ret == TVControlFailed || ret == TVControlNotImplemented)
        return false;

    if (programList) {
        if (programList->length) {
            for (uint64_t i = 0; i < programList->length; i++) {
                printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
                fflush(stdout);
                m_tvBackend->m_program = &programList->programs[i];
                programVector.append(PlatformTVProgram::create(m_tvBackend));
            }
            return true;
        }
    }
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    return false;
}

bool PlatformTVChannel::getCurrentProgram(RefPtr<PlatformTVProgram>& currentProgram)
{
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    struct wpe_tvcontrol_program* program = nullptr;
    tvcontrol_return ret = TVControlFailed;
    std::string::size_type sz = 0;
    ret = wpe_tvcontrol_backend_get_current_program(m_tvBackend->m_backend, m_tunerId.utf8().data(), std::stoull(m_serviceId.utf8().data(), &sz, 0), &program);
    fflush(stdout);
    if (ret == TVControlFailed || ret == TVControlNotImplemented)
        return false;

    if (program) {
        m_tvBackend->m_program = program;
        currentProgram = PlatformTVProgram::create(m_tvBackend);
        return true;
    }
    printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
    return false;
}

bool PlatformTVChannel::setParentalLock(const String& pin, bool isLocked)
{
    return wpe_tvcontrol_backend_set_parental_lock(m_tvBackend->m_backend, m_tunerId.utf8().data(), atoi(m_number.utf8().data()), pin.utf8().data(), &isLocked);
}

bool PlatformTVChannel::isParentalLocked()
{
    wpe_tvcontrol_backend_is_parental_locked(m_tvBackend->m_backend, m_tunerId.utf8().data(), atoi((m_number.utf8().data())), &m_isParentalLocked);
    return m_isParentalLocked;
}

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)
