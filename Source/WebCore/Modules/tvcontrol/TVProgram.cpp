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
#include "TVProgram.h"

#if ENABLE(TV_CONTROL)

namespace WebCore {

Ref<TVProgram> TVProgram::create(RefPtr<PlatformTVProgram> platformTVProgram, TVChannel* parentTVChannel)
{
    return adoptRef(*new TVProgram(platformTVProgram, parentTVChannel));
}

TVProgram::TVProgram(RefPtr<PlatformTVProgram> platformTVProgram, TVChannel* parentTVChannel)
    : m_platformTVProgram(platformTVProgram)
    , m_parentTVChannel(parentTVChannel)
{
}

const Vector<String>& TVProgram::getAudioLanguages()
{
    if (m_platformTVProgram) {
        Vector<String> platformAudioLanguagesList;
        if (!m_platformTVProgram->getAudioLanguages(platformAudioLanguagesList))
            return m_audioLanguagesList;

        if (platformAudioLanguagesList.size()) {
            for (auto& language : platformAudioLanguagesList)
                m_audioLanguagesList.append(language);
        }
    }

    return m_audioLanguagesList;
}

const Vector<String>& TVProgram::getSubtitleLanguages()
{
    if (m_platformTVProgram) {
        Vector<String> platformSubtitleLanguagesList;
        if (!m_platformTVProgram->getSubtitleLanguages(platformSubtitleLanguagesList))
            return m_subtitleLanguagesList;

        if (platformSubtitleLanguagesList.size()) {
            for (auto& language : platformSubtitleLanguagesList)
                m_subtitleLanguagesList.append(language);
        }
    }

    return m_subtitleLanguagesList;
}

const Vector<String>& TVProgram::getGenres()
{
    if (m_platformTVProgram) {
        Vector<String> platformGenresList;
        if (!m_platformTVProgram->getGenres(platformGenresList))
            return m_genresList;

        if (platformGenresList.size()) {
            for (auto& genre : platformGenresList)
                m_genresList.append(genre);
        }
    }

    return m_genresList;
}

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)
