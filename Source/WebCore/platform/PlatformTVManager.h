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

#include "PlatformTVTuner.h"

namespace WebCore {

class PlatformTVControlBackend;


class PlatformTVManagerClient {
public:
    virtual void didTunerOperationChanged(String, uint16_t) = 0;
    virtual void didCurrentSourceChanged(String) = 0;
    virtual void didCurrentChannelChanged(String) = 0;
    virtual void didScanningStateChanged(String, RefPtr<PlatformTVChannel>, uint16_t) = 0;
    virtual void didParentalControlChanged(uint16_t) = 0;
    virtual void didParentalLockChanged(String, uint16_t) = 0;
protected:
    virtual ~PlatformTVManagerClient() = default;
};


class WEBCORE_EXPORT PlatformTVManager {
public:
    WEBCORE_EXPORT explicit PlatformTVManager(PlatformTVManagerClient*);
    WEBCORE_EXPORT virtual ~PlatformTVManager();

    bool getTuners(Vector<RefPtr<PlatformTVTuner>>&);
    void updateTunerList(String, uint16_t);
    bool isParentalControlled();
    bool setParentalControl(const String&, bool);
    bool setParentalControlPin(const String&, const String&);
    PlatformTVControlBackend *m_tvBackend;
private:
    bool m_isParentalControlled;
    bool m_tunerListIsInitialized;
    PlatformTVManagerClient* m_platformTVManagerClient;
};

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)
