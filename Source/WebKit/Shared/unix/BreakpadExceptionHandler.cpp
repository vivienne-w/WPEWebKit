/*
 * If not stated otherwise in this file or this component's license file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if ENABLE(BREAKPAD)
#include "config.h"
#include "BreakpadExceptionHandler.h"
#include <client/linux/handler/exception_handler.h>
#include <signal.h>
#include <wtf/FileSystem.h>

namespace WebKit
{
// called by 'google_breakpad::ExceptionHandler' on every crash
static bool breakpadCallback(const google_breakpad::MinidumpDescriptor& descriptor, void* context, bool succeeded)
{
    (void) descriptor;
    (void) context;
    return succeeded;
}

void installExceptionHandler()
{
    static google_breakpad::ExceptionHandler* execptionHandler = NULL;
    static String breakpadMinidumpDir(getenv("BREAKPAD_MINIDUMP_DIR"));

#ifdef BREAKPAD_MINIDUMP_DIR
    if (breakpadMinidumpDir.isEmpty())
        breakpadMinidumpDir = String(BREAKPAD_MINIDUMP_DIR);
#endif

    if (breakpadMinidumpDir.isEmpty()) {
        WTFLogAlways("Breakpad dir is not set or empty, not installing handler");
        return;
    }

    if (!FileSystem::fileIsDirectory(breakpadMinidumpDir, FileSystem::ShouldFollowSymbolicLinks::No)) {
        WTFLogAlways("Breakpad dir \"%s\" doesn't exist, not installing handler", breakpadMinidumpDir.utf8().data());
        return;
    }

#ifdef SIGPIPE
    signal (SIGPIPE, SIG_IGN);
#endif

    execptionHandler = new google_breakpad::ExceptionHandler(google_breakpad::MinidumpDescriptor(breakpadMinidumpDir.utf8().data()), NULL, breakpadCallback, NULL, true, -1);
}
}
#endif

