/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebProcessMainUnix.h"

#include "ChildProcessMain.h"
#include "CommonVM.h"
#include "VM.h"
#include "WebProcess.h"
#include <WebCore/PlatformDisplayWPE.h>
#include <glib.h>
#include <iostream>
#include <libsoup/soup.h>
#include <WebCore/GCController.h>
#include <WebCore/MemoryCache.h>
#include <JavaScriptCore/JSLock.h>

using namespace JSC;
using namespace WebCore;

namespace WebKit {

static gboolean dumpMemoryStats(gpointer)
{
    WebCore::MemoryCache::singleton().dumpStats();
    GCController::singleton().garbageCollectNow();
    JSLockHolder lock(commonVM());
    printf ("MEMDBG: DUMPSTART\n");
    printf ("JavaScriptObjectsCount: %zu\n", commonVM().heap.objectCount());
    printf ("JavaScriptGlobalObjectsCount: %zu\n", commonVM().heap.globalObjectCount());
    printf ("JavaScriptProtectedObjectsCount: %zu\n", commonVM().heap.protectedObjectCount());
    printf ("JavaScriptProtectedGlobalObjectsCount: %zu\n", commonVM().heap.protectedGlobalObjectCount());

    printf ("JavaScriptPotectedObjectTypes\n");
    std::unique_ptr<TypeCountSet> protectedObjectTypeCounts(commonVM().heap.protectedObjectTypeCounts());
    TypeCountSet::const_iterator end = protectedObjectTypeCounts->end();
    for (TypeCountSet::const_iterator it = protectedObjectTypeCounts->begin(); it != end; ++it)
        printf ("\tPOT-%s: %d\n", it->key, it->value);

    printf ("JavaScriptObjectTypes\n");
    std::unique_ptr<TypeCountSet> objectTypeCounts(commonVM().heap.objectTypeCounts());
    end = objectTypeCounts->end();
    for (TypeCountSet::const_iterator it = objectTypeCounts->begin(); it != end; ++it)
        printf ("\tOT-%s: %d\n", it->key, it->value);

    size_t javaScriptHeapSize = commonVM().heap.size();
    printf ("JavaScriptHeapSize: %zu\n", javaScriptHeapSize);
    printf ("JavaScriptFreeSize: %zu\n", commonVM().heap.capacity() - javaScriptHeapSize);
    printf ("MEMDBG: DUMPEND\n");
    fflush(stdout);

    return G_SOURCE_CONTINUE;
}

class WebProcessMain final: public ChildProcessMainBase {
public:
    bool platformInitialize() override
    {
#if ENABLE(DEVELOPER_MODE)
        if (g_getenv("WEBKIT2_PAUSE_WEB_PROCESS_ON_LAUNCH"))
            WTF::sleep(30);
#endif

        g_timeout_add(10 * 1000, dumpMemoryStats, nullptr);

        return true;
    }

    bool parseCommandLine(int argc, char** argv) override
    {
        ASSERT(argc == 3);
        if (argc < 3)
            return false;

        if (!ChildProcessMainBase::parseCommandLine(argc, argv))
            return false;

        int wpeFd = atoi(argv[2]);
        RunLoop::main().dispatch(
            [wpeFd] {
                RELEASE_ASSERT(is<PlatformDisplayWPE>(PlatformDisplay::sharedDisplay()));
                downcast<PlatformDisplayWPE>(PlatformDisplay::sharedDisplay()).initialize(wpeFd);
            });
        return true;
    }
};

int WebProcessMainUnix(int argc, char** argv)
{
    return ChildProcessMain<WebProcess, WebProcessMain>(argc, argv);
}

} // namespace WebKit
