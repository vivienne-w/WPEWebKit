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
#include "PlatformTVProgram.h"

#if ENABLE(TV_CONTROL)

#include "PlatformTVControl.h"
#include <wpe/tvcontrol-backend.h>

namespace WebCore {

RefPtr<PlatformTVProgram> PlatformTVProgram::create(PlatformTVControlBackend* tvBackend, String tunerId)
{
    return adoptRef(new PlatformTVProgram(tvBackend, tunerId));
}

PlatformTVProgram::PlatformTVProgram(PlatformTVControlBackend* tvBackend, String tunerId)
    : m_tunerId(tunerId)
    , m_tvBackend(tvBackend)
{
    m_eventId = (std::to_string(tvBackend->m_program->eventId)).c_str();
    m_title = tvBackend->m_program->title;
    m_startTime = tvBackend->m_program->startTime;
    m_duration = tvBackend->m_program->duration;
    m_shortDescription = (std::to_string(tvBackend->m_program->shortDescription)).c_str();
    m_longDescription = (std::to_string(tvBackend->m_program->longDescription)).c_str();
    m_rating  = tvBackend->m_program->rating;
    m_seriesId  = (std::to_string(tvBackend->m_program->seriesId)).c_str();
    m_serviceId = (std::to_string(tvBackend->m_program->serviceId)).c_str();
}

PlatformTVProgram::~PlatformTVProgram()
{
}

bool PlatformTVProgram::getAudioLanguages(Vector<String>& audioLanguagesVector)
{
    struct wpe_tvcontrol_string_vector audioLanguagesList;
    audioLanguagesList.length = 0;
    std::string::size_type sz = 0;
    wpe_tvcontrol_backend_get_audio_languages_list(m_tvBackend->m_backend, m_tunerId.utf8().data(), std::stoull(m_serviceId.utf8().data(), &sz, 0), std::stoull(m_eventId.utf8().data(), &sz, 0), &audioLanguagesList);
    if (audioLanguagesList.length) {
        for (uint64_t i = 0; i < audioLanguagesList.length; i++) {
            String language(audioLanguagesList.strings[i].data, audioLanguagesList.strings[i].length);
            audioLanguagesVector.append((String)language);
        }
        return true;
    }
    return false;
}


bool PlatformTVProgram::getSubtitleLanguages(Vector<String>& subtitleLanguagesVector)
{
    struct wpe_tvcontrol_string_vector subtitleLanguagesList;
    subtitleLanguagesList.length = 0;
    std::string::size_type sz = 0;
    wpe_tvcontrol_backend_get_subtitle_languages_list(m_tvBackend->m_backend, m_tunerId.utf8().data(), std::stoull(m_serviceId.utf8().data(), &sz, 0), std::stoull(m_eventId.utf8().data(), &sz, 0), &subtitleLanguagesList);
    if (subtitleLanguagesList.length) {
        for (uint64_t i = 0; i < subtitleLanguagesList.length; i++) {
            String language(subtitleLanguagesList.strings[i].data, subtitleLanguagesList.strings[i].length);
            subtitleLanguagesVector.append(language);
        }
        return true;
    }
    return false;
}
bool PlatformTVProgram::getGenres(Vector<String>& genresVector)
{
    struct wpe_tvcontrol_string_vector genresList;
    genresList.length = 0;
    std::string::size_type sz = 0;
    wpe_tvcontrol_backend_get_genres_list(m_tvBackend->m_backend, m_tunerId.utf8().data(), std::stoull(m_serviceId.utf8().data(), &sz, 0), std::stoull(m_eventId.utf8().data(), &sz, 0), &genresList);
    if (genresList.length) {
        for (uint64_t i = 0; i < genresList.length; i++) {
            String genre(genresList.strings[i].data, genresList.strings[i].length);
            genresVector.append(genre);
        }
        return true;
    }
    return false;
}

} // namespace WebCore

#endif // ENABLE(TV_CONTROL)
