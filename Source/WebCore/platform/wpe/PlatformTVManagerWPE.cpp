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
#include "PlatformTVManager.h"

#if ENABLE(TV_CONTROL)

#include "PlatformTVControl.h"
#include <wpe/tvcontrol-backend.h>
#include <wtf/MainThread.h>

#define emergencyParam(paramName)event->eventParams.emergencyAlert->paramName
#define paramValueorNull(param) emergencyParam(param) ? String(emergencyParam(param), strlen(emergencyParam(param))) : String()
#define uintToString(param) emergencyParam(channelNo) ? (std::to_string(emergencyParam(channelNo))).c_str() : String()

namespace WebCore {

PlatformTVManager::PlatformTVManager(PlatformTVManagerClient* client)
    : m_isParentalControlled(false)
    , m_tunerListIsInitialized(false)
    , m_platformTVManagerClient(client)
{
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    m_tvBackend = new PlatformTVControlBackend();
    m_tvBackend->m_backend = wpe_tvcontrol_backend_create();

    static struct wpe_tvcontrol_backend_manager_event_client s_eventClient = {
        // handle_tuner_event
        [](void* data, wpe_tvcontrol_event* event)
        {
            String tunerId(event->tunerId.data, event->tunerId.length);
            printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
            uint16_t operation = event->eventParams.operation;
            PlatformTVManager* tvManager = reinterpret_cast<PlatformTVManager*>(data);
            callOnMainThread([tvManager, tunerId, operation] {
                printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
                tvManager->m_platformTVManagerClient->didTunerOperationChanged(tunerId, operation);
            });
        },
        // handle current source changed event
        [](void* data, wpe_tvcontrol_event* event)
        {
            String tunerId(event->tunerId.data, event->tunerId.length);
            printf("\n%s:%s:%d Tuner ID Hello = %s\n", __FILE__, __func__, __LINE__, event->tunerId.data);
            PlatformTVManager* tvManager = reinterpret_cast<PlatformTVManager*>(data);
            callOnMainThread([tvManager, tunerId] {
                printf("\n%s:%s:%d Tuner ID  = %s\n", __FILE__, __func__, __LINE__, tunerId.utf8().data());
                tvManager->m_platformTVManagerClient->didCurrentSourceChanged(tunerId);
            });
        },
        // handle current channel changed event
        [](void* data, wpe_tvcontrol_event* event)
        {
            String tunerId(event->tunerId.data, event->tunerId.length);
            PlatformTVManager* tvManager = reinterpret_cast<PlatformTVManager*>(data);
            RefPtr<PlatformTVChannel> protector = nullptr;

            printf("\n%s:%s:%d Tuner ID  = %s\n", __FILE__, __func__, __LINE__, event->tunerId.data);
            callOnMainThread([tvManager, tunerId] {
                printf("\n%s:%s:%d Tuner ID  = %s\n", __FILE__, __func__, __LINE__, tunerId.utf8().data());
                tvManager->m_platformTVManagerClient->didCurrentChannelChanged(tunerId);
            });
        },
        // handle eit broadcasted event
        [](void* data, wpe_tvcontrol_event* event)
        {
            String tunerId(event->tunerId.data, event->tunerId.length);
            PlatformTVManager* tvManager = reinterpret_cast<PlatformTVManager*>(data);
            Vector<RefPtr<PlatformTVProgram>> programs;

            if (event->eventParams.programsInfo) {
                for (uint64_t i = 0; i < event->eventParams.programsInfo->length; i++) {
                    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
                    fflush(stdout);
                    tvManager->m_tvBackend->m_program = &event->eventParams.programsInfo->programs[i];
                    programs.append(PlatformTVProgram::create(tvManager->m_tvBackend, tunerId));
                }
            }
            printf("\n%s:%s:%d Tuner ID  = %s\n", __FILE__, __func__, __LINE__, event->tunerId.data);
            callOnMainThread([tvManager, tunerId, programs] {
                printf("\n%s:%s:%d Tuner ID  = %s\n", __FILE__, __func__, __LINE__, tunerId.utf8().data());
                tvManager->m_platformTVManagerClient->didEITBroadcasted(tunerId, programs);
            });
        },
        // handle scan state  changed event
        [](void* data, wpe_tvcontrol_event* event)
        {

            uint16_t state = (uint16_t)event->eventParams.state;
            String tunerId(event->tunerId.data, event->tunerId.length);
            PlatformTVManager* tvManager = reinterpret_cast<PlatformTVManager*>(data);
            RefPtr<PlatformTVChannel> protector = nullptr;

            if (event->channelInfo) {
                tvManager->m_tvBackend->m_channel = event->channelInfo;
                protector = PlatformTVChannel::create(tvManager->m_tvBackend, tunerId);
            }
            printf("\n%s:%s:%d Tuner ID  = %s\n", __FILE__, __func__, __LINE__, event->tunerId.data);
            callOnMainThread([tvManager, tunerId, state, protector] {
                printf("\n%s:%s:%d Tuner ID  = %s\n", __FILE__, __func__, __LINE__, tunerId.utf8().data());
                tvManager->m_platformTVManagerClient->didScanningStateChanged(tunerId, protector, state);
            });
        },
        // handle parental control changed event
        [](void* data, wpe_tvcontrol_event* event)
        {
            printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
            uint16_t state = (uint16_t)event->eventParams.parentalControl;
            PlatformTVManager* tvManager = reinterpret_cast<PlatformTVManager*>(data);
            callOnMainThread([tvManager, state ] {
                printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
                tvManager->m_platformTVManagerClient->didParentalControlChanged(state);
            });
        },
        // handle parental lock changed event
        [](void* data, wpe_tvcontrol_event* event)
        {
            printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
            String tunerId(event->tunerId.data, event->tunerId.length);
            uint16_t state = (uint16_t)event->eventParams.parentalLock;
            PlatformTVManager* tvManager = reinterpret_cast<PlatformTVManager*>(data);
            callOnMainThread([tvManager, tunerId, state ] {
                printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
                tvManager->m_platformTVManagerClient->didParentalLockChanged(tunerId, state);
            });
        },
        // handle emergency alerted event
        [](void* data, wpe_tvcontrol_event* event)
        {
            printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
            String tunerId(event->tunerId.data, event->tunerId.length);
            PlatformTVManager* tvManager = reinterpret_cast<PlatformTVManager*>(data);
            // If the attributes are null, convert them to null WTF string.
            String emergencyType = paramValueorNull(type);
            String emergencySeverity = paramValueorNull(severityLevel);
            String description = paramValueorNull(description);
            String url = paramValueorNull(url);
            String channelNo = uintToString(channelNo);
            Vector<String> regionVector;
            if (emergencyParam(region->length)) {
                for (uint64_t i = 0; i < emergencyParam(region->length); i++)
                    regionVector.append(String(emergencyParam(region->regions[i]), strlen(emergencyParam(region->regions[i]))));
            }
            callOnMainThread([tvManager, tunerId, emergencyType, emergencySeverity, description, channelNo, url, regionVector] {
                tvManager->m_platformTVManagerClient->didEmergencyAlerted(tunerId, emergencyType, emergencySeverity, description, channelNo, url, regionVector);
            });
        },
    };
    wpe_tvcontrol_backend_set_manager_event_client(m_tvBackend->m_backend, &s_eventClient, this);
}

PlatformTVManager::~PlatformTVManager()
{
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    wpe_tvcontrol_backend_destroy(m_tvBackend->m_backend);
    delete m_tvBackend;
}

bool PlatformTVManager::getTuners(Vector<RefPtr<PlatformTVTuner>>& tunerVector)
{
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);

    struct wpe_tvcontrol_string_vector tunerList;
    tunerList.length = 0;
    tvcontrol_return ret = TVControlFailed;
    ret = wpe_tvcontrol_backend_get_tuner_list(m_tvBackend->m_backend, &tunerList);
    if (ret == TVControlFailed || ret == TVControlNotImplemented)
        return false;

    if (tunerList.length) {
        for (uint64_t i = 0; i < tunerList.length; i++) {
            String tunerId(tunerList.strings[i].data, tunerList.strings[i].length);
            tunerVector.append(PlatformTVTuner::create(tunerId, m_tvBackend));
        }
    }
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    return true;
}

bool PlatformTVManager::setParentalControl(const String& pin, bool isLocked)
{
    return wpe_tvcontrol_backend_set_parental_control(m_tvBackend->m_backend, pin.utf8().data(), &isLocked);
}

bool PlatformTVManager::isParentalControlled()
{
    wpe_tvcontrol_backend_is_parental_controlled(m_tvBackend->m_backend, &m_isParentalControlled);
    return m_isParentalControlled;
}

bool PlatformTVManager::setParentalControlPin(const String& oldPin, const String& newPin)
{
    return wpe_tvcontrol_backend_set_parental_control_pin(m_tvBackend->m_backend, oldPin.utf8().data(), newPin.utf8().data());
}

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)
