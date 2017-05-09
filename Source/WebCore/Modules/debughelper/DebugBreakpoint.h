/*
 * DebugBreakpoint.h
 *
 *  Created on: Sep 23, 2014
 *      Author: enrique
 */

#ifndef DEBUGBREAKPOINT_H_
#define DEBUGBREAKPOINT_H_

#include "ScriptWrappable.h"
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore
{

class DebugBreakpoint : public RefCounted<DebugBreakpoint>, public ScriptWrappable
{
public:
    static RefPtr<DebugBreakpoint> create();

    DebugBreakpoint();
    virtual ~DebugBreakpoint();
    static void printMessage(const String& message);
    static void crash(const String& message);
};

} /* namespace WebCore */

#endif /* DEBUGBREAKPOINT_H_ */
