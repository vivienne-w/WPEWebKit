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

#ifndef wpe_tvcontrol_backend_h
#define wpe_tvcontrol_backend_h

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __GBM__
#define __GBM__
#endif

#include <stdbool.h>
#include <stdint.h>

typedef enum { Added, Removed } tuner_changed_operation;
typedef enum { Cleared, Scanned, Completed, Stopped } scanning_state;
typedef enum { ParentalControlOff, ParentalControlOn } parental_control_state;
typedef enum { ParentalLockOff, ParentalLockOn } parental_lock_state;

typedef enum { TVControlFailed, TVControlSuccess, TVControlNotImplemented } tvcontrol_return;

struct wpe_tvcontrol_string {
    char* data;
    uint64_t length;
};

struct wpe_tvcontrol_string_vector {
    struct wpe_tvcontrol_string* strings;
    uint64_t length;
};

typedef enum {TunerChanged, SourceChanged, ChannelChanged, ScanningChanged, ParentalControlChanged, ParentalLockChanged} tvcontrol_events;


typedef enum { DvbT, DvbT2, DvbC, DvbC2, DvbS, DvbS2, DvbH, DvbSh, Atsc, AtscMH, IsdbT, IsdbTb, IsdbS, IsdbC, _1seg, Dtmb, Cmmb, TDmb, SDmb, Undifined } SourceType;
struct wpe_tvcontrol_src_types_vector {
    SourceType* types;
    uint64_t length;
};

typedef enum { Tv, Radio, Data } ChannelType;
struct wpe_tvcontrol_channel {
    uint64_t networkId;
    uint64_t transportSId;
    uint64_t serviceId;
    char* name;
    uint64_t number;
    ChannelType type;
};

struct wpe_get_programs_options {
    uint64_t startTime;
    uint64_t endTime;
};

struct wpe_tvcontrol_program {
    uint64_t    eventId;
    char*       title;
    uint64_t    startTime;
    uint64_t    duration;
    uint64_t    shortDescription;
    uint64_t    longDescription;
    uint64_t    rating;
    uint64_t    seriesId;
};

struct wpe_tvcontrol_event {
    tvcontrol_events eventID;
    tuner_changed_operation operation;
    scanning_state state;
    parental_control_state parentalControl;
    parental_lock_state parentalLock;
    struct wpe_tvcontrol_string tuner_id;
    struct wpe_tvcontrol_channel* channel_info;
};

struct wpe_tvcontrol_backend_manager_event_client {
    void (*handle_tuner_event)(void*, struct wpe_tvcontrol_event*);
    void (*handle_source_changed_event)(void*, struct wpe_tvcontrol_event*);
    void (*handle_channel_changed_event)(void*, struct wpe_tvcontrol_event*);
    void (*handle_scanning_state_changed_event)(void*, struct wpe_tvcontrol_event*);
    void (*handle_parental_control_changed_event)(void*, struct wpe_tvcontrol_event*);
    void (*handle_parental_lock_changed_event)(void*, struct wpe_tvcontrol_event*);
};

struct wpe_tvcontrol_channel_vector {
    struct wpe_tvcontrol_channel* channels;
    uint64_t length;
};

struct wpe_tvcontrol_program_vector {
    struct wpe_tvcontrol_program* programs;
    uint64_t                      length;
};

struct wpe_tvcontrol_backend_interface {
    void*               (*create)(struct  wpe_tvcontrol_backend*);
    void                (*destroy)(void*);
    tvcontrol_return    (*get_tuner_list)(void*, struct wpe_tvcontrol_string_vector*);
    void                (*get_supported_source_types_list)(void*, const char*, struct wpe_tvcontrol_src_types_vector*);
    tvcontrol_return    (*get_source_list)(void*, const char*, struct wpe_tvcontrol_src_types_vector*);
    void                (*get_signal_strength)(void*, const char*, double*);
    tvcontrol_return    (*start_scanning)(void*, const char*, SourceType, bool);
    tvcontrol_return    (*stop_scanning)(void*, const char*);
    tvcontrol_return    (*set_current_channel)(void*, const char*, SourceType, uint64_t);
    tvcontrol_return    (*set_current_source)(void*, const char*, SourceType);
    tvcontrol_return    (*get_channel_list)(void*, const char*, SourceType, struct wpe_tvcontrol_channel_vector**);
    tvcontrol_return    (*get_program_list)(void*, const char*, uint64_t, struct wpe_get_programs_options*, struct wpe_tvcontrol_program_vector**);
    tvcontrol_return    (*get_current_program)(void*, const char*, uint64_t, struct wpe_tvcontrol_program**);
    void                (*is_parental_controlled)(void*, bool*);
    tvcontrol_return    (*set_parental_control)(void*, const char*, bool*);
    tvcontrol_return    (*set_parental_control_pin)(void*i, const char*, const char*);
    tvcontrol_return    (*set_parental_lock)(void*, const char*, uint64_t, const char*, bool*);
    void                (*is_parental_locked)(void*, const char*, uint64_t, bool*);
};

struct wpe_tvcontrol_backend*
wpe_tvcontrol_backend_create();

void
wpe_tvcontrol_backend_destroy(struct wpe_tvcontrol_backend*);

void
wpe_tvcontrol_backend_set_manager_event_client(struct wpe_tvcontrol_backend*, struct wpe_tvcontrol_backend_manager_event_client*, void*);

void
wpe_tvcontrol_backend_dispatch_tuner_event(struct wpe_tvcontrol_backend*, struct wpe_tvcontrol_event*);

void
wpe_tvcontrol_backend_dispatch_source_event(struct wpe_tvcontrol_backend*, struct wpe_tvcontrol_event*);

void
wpe_tvcontrol_backend_dispatch_channel_event(struct wpe_tvcontrol_backend*, struct wpe_tvcontrol_event*);

void
wpe_tvcontrol_backend_dispatch_scanning_state_event(struct wpe_tvcontrol_backend*, struct wpe_tvcontrol_event*);

void
wpe_tvcontrol_backend_dispatch_parental_control_event(struct wpe_tvcontrol_backend*, struct wpe_tvcontrol_event*);

void
wpe_tvcontrol_backend_dispatch_parental_lock_event(struct wpe_tvcontrol_backend*, struct wpe_tvcontrol_event*);

tvcontrol_return
wpe_tvcontrol_backend_get_tuner_list(struct wpe_tvcontrol_backend* backend, struct wpe_tvcontrol_string_vector* out_tuner_list);

void
wpe_tvcontrol_backend_get_supported_source_types_list(struct wpe_tvcontrol_backend* backend, const char* tuner_id, struct wpe_tvcontrol_src_types_vector* out_source_types_list);

tvcontrol_return
wpe_tvcontrol_backend_get_source_list(struct wpe_tvcontrol_backend* backend, const char* tuner_id, struct wpe_tvcontrol_src_types_vector* out_source_list);

tvcontrol_return
wpe_tvcontrol_backend_set_current_source(struct wpe_tvcontrol_backend* backend, const char* tuner_id, SourceType sType);

void
wpe_tvcontrol_backend_get_signal_strength(struct wpe_tvcontrol_backend* backend, const char* tuner_id, double* out_signal_strength);

tvcontrol_return
wpe_tvcontrol_backend_start_scanning(struct wpe_tvcontrol_backend*, const char* tuner_id, SourceType type, bool isRescanned);

tvcontrol_return
wpe_tvcontrol_backend_stop_scanning(struct wpe_tvcontrol_backend*, const char* tuner_id);

tvcontrol_return
wpe_tvcontrol_backend_set_current_channel(struct wpe_tvcontrol_backend*, const char* tuner_id, SourceType type, uint64_t channel_number);

tvcontrol_return
wpe_tvcontrol_backend_get_channel_list(struct wpe_tvcontrol_backend*, const char* tuner_id, SourceType type, struct wpe_tvcontrol_channel_vector** out_channel_list);

tvcontrol_return
wpe_tvcontrol_backend_get_program_list(struct wpe_tvcontrol_backend*, const char* tuner_id, uint64_t service_id, struct wpe_get_programs_options*, struct wpe_tvcontrol_program_vector** out_program_list);

tvcontrol_return
wpe_tvcontrol_backend_get_current_program(struct wpe_tvcontrol_backend*, const char* tuner_id, uint64_t service_id, struct wpe_tvcontrol_program**);

void
wpe_tvcontrol_backend_is_parental_controlled(struct wpe_tvcontrol_backend*, bool* is_parental_controlled);

tvcontrol_return
wpe_tvcontrol_backend_set_parental_control(struct wpe_tvcontrol_backend*, const char* pin, bool* is_locked);

tvcontrol_return
wpe_tvcontrol_backend_set_parental_control_pin(struct wpe_tvcontrol_backend*, const char* old_pin, const char* new_pin);

tvcontrol_return
wpe_tvcontrol_backend_set_parental_lock(struct wpe_tvcontrol_backend*, const char* tuner_id, uint64_t channel_number, const char* pin, bool* is_locked);

void
wpe_tvcontrol_backend_is_parental_locked(struct wpe_tvcontrol_backend*, const char* tuner_id, uint64_t channel_number, bool* is_parental_locked);
#ifdef __cplusplus
}
#endif

#endif // wpe_tvcontrol_backend_h
