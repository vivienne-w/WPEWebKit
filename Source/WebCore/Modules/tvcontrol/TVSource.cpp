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
#include "TVSource.h"

#if ENABLE(TV_CONTROL)

#include "ExceptionCode.h"

namespace WebCore {

Ref<TVSource> TVSource::create(ScriptExecutionContext* context, RefPtr<PlatformTVSource> platformTVSource, TVTuner* parentTVTuner)
{
    return adoptRef(*new TVSource(context, platformTVSource, parentTVTuner));
}

TVSource::TVSource(ScriptExecutionContext* context, RefPtr<PlatformTVSource> platformTVSource, TVTuner* parentTVTuner)
    : ContextDestructionObserver(context)
    , m_platformTVSource(platformTVSource)
    , m_parentTVTuner(parentTVTuner)
    , m_currentChannel(nullptr)
    , m_scanState(ScanningNotInitialised)
    , m_isScanning(false)
{
}

TVSource::~TVSource()
{
    if (m_channelList.size())
        m_channelList.clear();
}

void TVSource::getChannels(TVChannelPromise&& promise)
{

    if (ScanningStarted == m_scanState) {
        promise.reject(INVALID_STATE_ERR, "Invalid state: scanning in progress");
        return;
    }

    if (m_channelList.size()) {
        promise.resolve(m_channelList);
        return;
    }
    if (m_platformTVSource) {
        Vector<RefPtr<PlatformTVChannel>> channelVector;
        if (!m_platformTVSource->getChannels(channelVector)) {
            promise.reject(nullptr);
            return;
        }
        if (channelVector.size()) {
            for (auto& channel : channelVector)
                m_channelList.append(TVChannel::create(scriptExecutionContext(), channel, this));
            promise.resolve(m_channelList);
            return;
        }
    }
    promise.reject(nullptr);
}

void TVSource::setCurrentChannel(const String& channelNumber, TVPromise&& promise)
{
    if (ScanningStarted == m_scanState) {
        promise.reject(INVALID_STATE_ERR, "Invalid state: scanning in progress");
        return;
    }
    if (m_platformTVSource) {
        for (auto& channel : m_channelList) {
            if (equalIgnoringASCIICase(channel->number(), channelNumber) == 1) {
                m_currentChannel = channel;
                if (m_platformTVSource->setCurrentChannel(channelNumber)) {
                    promise.resolve(nullptr);
                    return;
                }
                if (channel->isParentalLocked()) {
                    promise.reject(INVALID_ACCESS_ERR, "Cannot Access a parental locked channel");
                    return;
                }
            }
        }
    }
    promise.reject(nullptr);
}

void TVSource::startScanning(const StartScanningOptions& scanningOptions, TVPromise&& promise)
{
    if (ScanningStarted == m_scanState) {
        promise.reject(nullptr);
        return;
    }
    if (m_platformTVSource) {
        m_scanState = ScanningStarted;
        m_isScanning = true;
        if (m_platformTVSource->startScanning(scanningOptions.isRescanned)) {
            m_isScanning = false;
            promise.resolve(nullptr);
            return;
        }
        m_scanState = ScanningCompleted;
    }
    promise.reject(nullptr);
}

void TVSource::stopScanning(TVPromise&& promise)
{
    if (m_platformTVSource) {
        if (ScanningCompleted != m_scanState) {
            if (m_platformTVSource->stopScanning()) {
                promise.resolve(nullptr);
                return;
            }
        }
    }
    promise.reject(nullptr);
}

void TVSource::dispatchChannelChangedEvent()
{
    scriptExecutionContext()->postTask([=](ScriptExecutionContext&) {
        dispatchEvent(TVCurrentChannelChangedEvent::create(eventNames().currentchannelchangedEvent, currentChannel()));
    });
}

void TVSource::dispatchEITBroadcastedEvent(Vector<RefPtr<PlatformTVProgram>> platformTVPrograms)
{
    Vector<RefPtr<TVProgram>> protector;
    if (platformTVPrograms.size()) {
        for (auto& program : platformTVPrograms) {
            for (auto& channel : m_channelList) {
                if (equalIgnoringASCIICase(program->serviceId(), channel->serviceId())) {
                    protector.append(TVProgram::create(program, channel.get()));
                    break;
                }
            }
        }
    }
    scriptExecutionContext()->postTask([=](ScriptExecutionContext&) {
        dispatchEvent(TVEITBroadcastedEvent::create(eventNames().eitbroadcastedEvent, protector));
    });
}

void TVSource::dispatchScanningStateChangedEvent(RefPtr<PlatformTVChannel> platformTVChannel, uint16_t state)
{
    RefPtr<TVChannel> protector = nullptr;
    switch (static_cast<TVScanningStateChangedEvent::State>(state)) {
    case TVScanningStateChangedEvent::State::Completed:
    case TVScanningStateChangedEvent::State::Stopped:
        m_scanState = ScanningCompleted;
        break;
    default:
        break;
    }
    if (m_platformTVSource && platformTVChannel) {
        protector = TVChannel::create(scriptExecutionContext(), platformTVChannel, this);
        m_channelList.append(protector);
    }
    scriptExecutionContext()->postTask([=](ScriptExecutionContext&) {
        dispatchEvent(TVScanningStateChangedEvent::create(eventNames().scanningstatechangedEvent,
            (TVScanningStateChangedEvent::State)state, protector));
    });
}

void TVSource::dispatchEmergencyAlertedEvent(String type, String severity, String description, String channelNo, String url, Vector<String> regionList)
{
    RefPtr<TVChannel> protector = nullptr;
    for (auto& channel : m_channelList) {
        if (equalIgnoringASCIICase(channel->number(), channelNo) == 1) {
            protector = channel;
            break;
        }
    }
    scriptExecutionContext()->postTask([=](ScriptExecutionContext&) {
        dispatchEvent(TVEmergencyAlertedEvent::create(eventNames().emergencyalertedEvent, type, severity, description, protector, url, regionList));
    });
}

ScriptExecutionContext* TVSource::scriptExecutionContext() const
{
    return ContextDestructionObserver::scriptExecutionContext();
}

void TVSource::contextDestroyed()
{
    ContextDestructionObserver::contextDestroyed();
}

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)
