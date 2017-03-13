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

#include "ChannelBackend.h"

#include "tv-log.h"

#define TV_DEBUG 1

namespace LinuxDVB {

ChannelBackend::~ChannelBackend()
{
    clearProgramVector();
    clearProgramList();
}

void ChannelBackend::clearProgramVector()
{
    if (m_programVector.length) {
        for (int i; i < m_programVector.length; i++) {
            if (m_programVector.programs[i].title)
                free(m_programVector.programs[i].title);
        }
        delete[] m_programVector.programs;
        m_programVector.length = 0;
    }
}

void ChannelBackend::clearProgramList()
{
    if (!m_programList.empty())
        m_programList.clear();
}

void ChannelBackend::appendPrograms(ProgramBackend* program)
{
    m_programList.push_back(program);
}

tvcontrol_return ChannelBackend::getPrograms(struct wpe_get_programs_options* programsOptions, wpe_tvcontrol_program_vector** programVector)
{
    tvcontrol_return ret = TVControlFailed;
    if (!m_programList.empty()) {
        if (m_programVector.length) {
            delete[] m_programVector.programs;
            m_programVector.length = 0;
        }
        int i = 0;
        TvLogInfo("programsOptions->startTime = %llu:programsOptions->endTime = %llu ",
            programsOptions->startTime, programsOptions->endTime);
        unsigned long long startTime, endTime;
        for (auto& program : m_programList) {
            startTime = (unsigned long long)program->getStartTime();
            endTime = (unsigned long long)(program->getStartTime() + program->getDuration());
            if ((unsigned long long)startTime >= (unsigned long long)programsOptions->startTime) {
                TvLogInfo("startTime = %llu:endTime = %llu ", startTime, endTime);
                if ((unsigned long long)endTime <= (unsigned long long)programsOptions->endTime) {
                    m_programVector.programs = reinterpret_cast<wpe_tvcontrol_program*>(realloc(m_programVector.programs,
                        (i+1) * sizeof(struct wpe_tvcontrol_program)));
                    m_programVector.programs[i].eventId = program->getEventId();
                    m_programVector.programs[i].startTime = program->getStartTime();
                    m_programVector.programs[i].duration = program->getDuration();
                    m_programVector.programs[i].title =  strdup(program->getTitle().c_str());
                    i++;
                }
            }
        }
        m_programVector.length = i;
        *programVector = &m_programVector;
        ret = TVControlSuccess;
    } else
        programVector = nullptr;
    return ret;
}

tvcontrol_return ChannelBackend::getCurrentProgram(wpe_tvcontrol_program** currentProgram)
{
    time_t cTime;
    time(&cTime);
    tvcontrol_return ret = TVControlFailed;
    struct tm currTime;
    localtime_r(&cTime, &currTime);
    TvLogInfo("current time = %d:%d", currTime.tm_hour, currTime.tm_min);
    for (auto& program : m_programList) {
        struct tm startTime;
        time_t progStartTime = program->getStartTime();
        localtime_r(&progStartTime, &startTime);
        TvLogInfo("%s:%s:%d:startTime = %d:%d\n", __FILE__, __func__, __LINE__, startTime.tm_hour, startTime.tm_min);
        if (startTime.tm_hour == currTime.tm_hour) {
            if (currTime.tm_min >= startTime.tm_min && currTime.tm_min < (startTime.tm_min + ((program->getDuration()) / 60))) {
                m_program = reinterpret_cast<wpe_tvcontrol_program*>(malloc(sizeof(struct wpe_tvcontrol_program)));
                m_program->eventId = program->getEventId();
                m_program->startTime = program->getStartTime();
                m_program->duration = program->getDuration();
                m_program->title =  strdup(program->getTitle().c_str());
                break;
            }
        }
    }
    *currentProgram = m_program;
    ret = TVControlSuccess;
    return ret;
}

void ChannelBackend::isParentalLocked(bool* isLocked)
{
    TvLogTrace();
    printf("isLocked m_state%d", m_isParentalLocked);
    *isLocked = m_isParentalLocked;
    printf("isLocked%d m_state%d", *isLocked, m_isParentalLocked);
}

tvcontrol_return ChannelBackend::setParentalLock(bool* isLocked, bool* lockChanged)
{
    if (*isLocked != m_isParentalLocked) {
        m_isParentalLocked = *isLocked;
        *lockChanged = true;
        return TVControlSuccess;
    }
    return TVControlFailed;
}

} // namespace LinuxDVB
