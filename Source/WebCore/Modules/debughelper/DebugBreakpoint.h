/*
 * DebugBreakpoint.h
 *
 *  Created on: Sep 23, 2014
 *      Author: enrique
 */

#ifndef DEBUGBREAKPOINT_H_
#define DEBUGBREAKPOINT_H_

#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore
{

class DebugBreakpoint : public RefCounted<DebugBreakpoint>
{
public:
    static void crash(const String& message);
    static void warn(const String& message);
    static void print(const String& message);
    static void printMessage(const String& message) { print(message); }
};

} /* namespace WebCore */

#endif /* DEBUGBREAKPOINT_H_ */
