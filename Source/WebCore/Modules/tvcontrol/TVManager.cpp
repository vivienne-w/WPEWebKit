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
#include "TVManager.h"

#if ENABLE(TV_CONTROL)

#include "Document.h"
#include "EventNames.h"
#include "Frame.h"
#include "Navigator.h"
#include "TVParentalControlChangedEvent.h"
#include "TVTunerChangedEvent.h"

namespace WebCore {

Ref<TVManager> TVManager::create(ScriptExecutionContext* context)
{
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    Ref<TVManager> tvManager(adoptRef(*new TVManager(context)));
    tvManager->suspendIfNeeded();
    return tvManager;
}

TVManager::TVManager(ScriptExecutionContext* context)
    : ActiveDOMObject(context)
    , m_platformTVManager(nullptr)
{
    if (!m_platformTVManager)
        m_platformTVManager = std::make_unique<PlatformTVManager>(this);
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

TVManager::~TVManager()
{
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    if (m_tunerList.size())
        m_tunerList.clear();
}

Document* TVManager::document() const
{
    return downcast<Document>(scriptExecutionContext());
}

void TVManager::didTunerOperationChanged(String tunerId, uint16_t event)
{
    int position;
    if ((TVTunerChangedEvent::Operation)event == TVTunerChangedEvent::Operation::Added) {
        m_tunerList.append(TVTuner::create(scriptExecutionContext(), PlatformTVTuner::create(tunerId.utf8().data(), m_platformTVManager->m_tvBackend)));
        printf("Found and Added the Tuner");
    } else { // Case when DVB Adapter is closed.
        position = 0;
        // Iterate  private tuner list and get the particular tuner info
        for (auto& tuner : m_tunerList) {
            printf("Id of this tuner %s\n", (tuner->id()).utf8().data());
            if (equalIgnoringASCIICase(tunerId, tuner->id())) {
                m_tunerList.remove(position);
                printf("Found and Deleted the Tuner");
                break;
            }
            position++;
        }
    }
    scriptExecutionContext()->postTask([=](ScriptExecutionContext&) {
        dispatchEvent(TVTunerChangedEvent::create(eventNames().tunerchangedEvent, tunerId, (TVTunerChangedEvent::Operation)event));
    });
}

void TVManager::didCurrentSourceChanged(String tunerId)
{
    printf("\n%s:%s:%d\n TUNER ID = %s", __FILE__, __func__, __LINE__, tunerId.utf8().data());
    for (auto& tuner : m_tunerList) {
        if (equalIgnoringASCIICase(tunerId, tuner->id())) {
            tuner->dispatchSourceChangedEvent();
            break;
        }
    }
}

void TVManager::didCurrentChannelChanged(String tunerId)
{
    printf("\n%s:%s:%d\n TUNER ID = %s", __FILE__, __func__, __LINE__, tunerId.utf8().data());
    for (auto& tuner : m_tunerList) {
        if (equalIgnoringASCIICase(tunerId, tuner->id())) {
            printf("\n%s:%s:%d\n TUNER ID = %s", __FILE__, __func__, __LINE__, tunerId.utf8().data());
            tuner->currentSource()->dispatchChannelChangedEvent();
            break;
        }
    }
}

void TVManager::didEITBroadcasted(String tunerId, Vector<RefPtr<PlatformTVProgram>> platformTVPrograms)
{
    printf("\n%s:%s:%d\n TUNER ID = %s", __FILE__, __func__, __LINE__, tunerId.utf8().data());
    for (auto& tuner : m_tunerList) {
        if (equalIgnoringASCIICase(tunerId, tuner->id())) {
            printf("\n%s:%s:%d\n TUNER ID = %s", __FILE__, __func__, __LINE__, tunerId.utf8().data());
            tuner->currentSource()->dispatchEITBroadcastedEvent(platformTVPrograms);
            break;
        }
    }
}

void TVManager::didScanningStateChanged(String tunerId, RefPtr<PlatformTVChannel> platformTVChannel, uint16_t state)
{
    for (auto& tuner : m_tunerList) {
        if (equalIgnoringASCIICase(tunerId, tuner->id())) {
            printf("Scanning state  dispatched from Mgr = ");
            printf("%" PRIu16 "\n", state);

            tuner->currentSource()->dispatchScanningStateChangedEvent(platformTVChannel, state);
            break;
        }
    }
}

void TVManager::didParentalControlChanged(uint16_t state)
{
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    scriptExecutionContext()->postTask([=](ScriptExecutionContext&) {
        dispatchEvent(TVParentalControlChangedEvent::create(eventNames().parentalcontrolchangedEvent, (TVParentalControlChangedEvent::State)state));
    });
}

void TVManager::didParentalLockChanged(String tunerId, uint16_t state)
{
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    for (auto& tuner : m_tunerList) {
        if (equalIgnoringASCIICase(tunerId, tuner->id())) {
            tuner->currentSource()->currentChannel()->dispatchParentalLockChangedEvent(state);
            break;
        }
    }
}

void TVManager::didEmergencyAlerted(String tunerId, String type, String severity, String description, String channelNo, String url, Vector<String> regionList)
{
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    for (auto& tuner : m_tunerList) {
        if (equalIgnoringASCIICase(tunerId, tuner->id())) {
            tuner->currentSource()->dispatchEmergencyAlertedEvent(type, severity, description, channelNo, url, regionList);
            break;
        }
    }
}

void TVManager::getTuners(TVTunerPromise&& promise)
{
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    if (m_tunerList.size()) {
        promise.resolve(m_tunerList);
        return;
    }
    if (!m_platformTVManager)
        m_platformTVManager = std::make_unique<PlatformTVManager>(this);

    if (m_platformTVManager) {
        Vector<RefPtr<PlatformTVTuner>> platformTunerList;
        if (!m_platformTVManager->getTuners(platformTunerList)) {
            promise.reject(nullptr);
            return;
        }
        if (platformTunerList.size()) {
            printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
            for (auto& tuner : platformTunerList) {
                printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
                m_tunerList.append(TVTuner::create(document(), tuner));
            }
            platformTunerList.clear();
        }
        if (m_tunerList.size()) {
            promise.resolve(m_tunerList);
            return;
        }
    }
    printf("\n%s:%s:%d\n", __FILE__, __func__, __LINE__);
    promise.reject(nullptr);
}

void TVManager::setParentalControl(const String& pin, bool isLocked, TVsetParentalControl&& promise)
{
    if (m_platformTVManager->setParentalControl(pin, isLocked))
        promise.resolve(nullptr);
    else
        promise.reject(SECURITY_ERR, "Unmatched Pins!!!");
    return;
}

void TVManager::setParentalControlPin(const String& oldPin, const String& newPin, TVsetParentalControl&& promise)
{
    if (m_platformTVManager->setParentalControlPin(oldPin, newPin))
        promise.resolve(nullptr);
    else
        promise.reject(SECURITY_ERR, "Unmatched Pins!!!....Unable to Change Pin");
    return;
}

ScriptExecutionContext* TVManager::scriptExecutionContext() const
{
    return ActiveDOMObject::scriptExecutionContext();
}

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)
