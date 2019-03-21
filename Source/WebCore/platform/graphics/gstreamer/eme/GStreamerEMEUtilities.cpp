/* GStreamer EME Utilities class
 *
 * Copyright (C) 2017 Metrological
 * Copyright (C) 2017 Igalia S.L
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "GStreamerEMEUtilities.h"

#include <wtf/MD5.h>
#include <wtf/text/Base64.h>

#if ENABLE(ENCRYPTED_MEDIA) && USE(GSTREAMER)

namespace WebCore {

bool isClearKeyKeySystem(const String& keySystem)
{
    return equalIgnoringASCIICase(keySystem, WEBCORE_GSTREAMER_EME_UTILITIES_CLEARKEY_KEY_SYSTEM);
}

#if USE(OPENCDM)
bool isPlayReadyKeySystem(const String& keySystem)
{
    return equalIgnoringASCIICase(keySystem, WEBCORE_GSTREAMER_EME_UTILITIES_PLAYREADY_KEY_SYSTEM)
        || equalIgnoringASCIICase(keySystem, WEBCORE_GSTREAMER_EME_UTILITIES_PLAYREADY_KEY_SYSTEM_YOUTUBE);
}

bool isWidevineKeySystem(const String& keySystem)
{
    return equalIgnoringASCIICase(keySystem, WEBCORE_GSTREAMER_EME_UTILITIES_WIDEVINE_KEY_SYSTEM);
}
#endif

const char* keySystemToUuid(const String& keySystem)
{
    if (isClearKeyKeySystem(keySystem))
        return WEBCORE_GSTREAMER_EME_UTILITIES_CLEARKEY_UUID;
#if USE(OPENCDM)
    if (isPlayReadyKeySystem(keySystem))
        return WEBCORE_GSTREAMER_EME_UTILITIES_PLAYREADY_UUID;

    if (isWidevineKeySystem(keySystem))
        return WEBCORE_GSTREAMER_EME_UTILITIES_WIDEVINE_UUID;
#endif
    ASSERT_NOT_REACHED();
    return { };
}


const char* uuidToKeySystem(const String& uuid)
{
    if (uuid == WEBCORE_GSTREAMER_EME_UTILITIES_CLEARKEY_UUID)
        return WEBCORE_GSTREAMER_EME_UTILITIES_CLEARKEY_KEY_SYSTEM;

#if USE(OPENCDM)
    if (uuid == WEBCORE_GSTREAMER_EME_UTILITIES_PLAYREADY_UUID)
        return WEBCORE_GSTREAMER_EME_UTILITIES_PLAYREADY_KEY_SYSTEM;

    if (uuid == WEBCORE_GSTREAMER_EME_UTILITIES_WIDEVINE_UUID)
        return WEBCORE_GSTREAMER_EME_UTILITIES_WIDEVINE_KEY_SYSTEM;
#endif

    ASSERT_NOT_REACHED();
    return nullptr;
}


// FIXME: This should go onto the InitData class.
#if (!defined(GST_DISABLE_GST_DEBUG))
String initDataMD5(const InitData& initData)
{
    WTF::MD5 md5;
    md5.addBytes(initData.data(), initData.size());

    WTF::MD5::Digest digest;
    md5.checksum(digest);

    return WTF::base64URLEncode(&digest[0], WTF::MD5::hashSize);
}
#endif

}

#endif
