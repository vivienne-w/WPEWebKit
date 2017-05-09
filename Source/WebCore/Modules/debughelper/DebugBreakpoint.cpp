/*
 * DebugBreakpoint.cpp
 *
 *  Created on: Sep 23, 2014
 *      Author: enrique
 */

#include "config.h"
#include "DebugBreakpoint.h"
#include <gst/gst.h>
#include <stdio.h>
#include <wtf/text/CString.h>

GST_DEBUG_CATEGORY_EXTERN(webkit_unaffiliated_debug);
#define GST_CAT_DEFAULT webkit_unaffiliated_debug

namespace WebCore
{

RefPtr<DebugBreakpoint> DebugBreakpoint::create()
{
    return adoptRef(new DebugBreakpoint());
}

DebugBreakpoint::DebugBreakpoint()
{
}

DebugBreakpoint::~DebugBreakpoint()
{
}

void DebugBreakpoint::printMessage(const String& message)
{
    fprintf(stderr, "%s:%d:%s: %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, message.utf8().data());
    GST_INFO("%s", message.utf8().data());
}

void DebugBreakpoint::crash(const String& message)
{
    if (message.isEmpty()) {
        fprintf(stderr, "%s:%d:%s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__);
        GST_ERROR("crashing");
    } else {
        fprintf(stderr, "%s:%d:%s: %s\n", __FILE__, __LINE__, __PRETTY_FUNCTION__, message.utf8().data());
        GST_ERROR("%s", message.utf8().data());
    }
#if PLATFORM(MAC)
    ASSERT_NOT_REACHED();
#else
    CRASH();
#endif
    return;
}

} /* namespace WebCore */
