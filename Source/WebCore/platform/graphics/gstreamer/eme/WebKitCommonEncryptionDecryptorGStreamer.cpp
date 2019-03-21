/* GStreamer ClearKey common encryption decryptor
 *
 * Copyright (C) 2013 YouView TV Ltd. <alex.ashley@youview.com>
 * Copyright (C) 2016 Metrological
 * Copyright (C) 2016 Igalia S.L
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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */

#include "config.h"
#include "WebKitCommonEncryptionDecryptorGStreamer.h"

#if ENABLE(ENCRYPTED_MEDIA) && USE(GSTREAMER)

#include "CDMInstance.h"
#include "GStreamerCommon.h"
#include "GStreamerEMEUtilities.h"
#include <wtf/Condition.h>
#include <wtf/PrintStream.h>
#include <wtf/RunLoop.h>

using WebCore::CDMInstance;
using WebCore::GstMappedBuffer;

#define WEBKIT_MEDIA_CENC_DECRYPT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WEBKIT_TYPE_MEDIA_CENC_DECRYPT, WebKitMediaCommonEncryptionDecryptPrivate))
struct _WebKitMediaCommonEncryptionDecryptPrivate {
    GRefPtr<GstEvent> protectionEvent;
    RefPtr<CDMInstance> cdmInstance;
    WTF::HashMap<String, WebCore::InitData> initDatas;
    Vector<GRefPtr<GstEvent>> protectionEvents;
    uint32_t currentEvent { 0 };
    bool isFlushing { false };
    bool keyReceived { false };
    bool waitingForKey { false };
    Lock mutex;
    Condition condition;
};

static GstStateChangeReturn changeState(GstElement*, GstStateChange transition);
static void finalize(GObject*);
static GstCaps* transformCaps(GstBaseTransform*, GstPadDirection, GstCaps*, GstCaps*);
static GstFlowReturn transformInPlace(GstBaseTransform*, GstBuffer*);
static gboolean sinkEventHandler(GstBaseTransform*, GstEvent*);
static gboolean queryHandler(GstBaseTransform*, GstPadDirection, GstQuery*);
static bool isCDMInstanceAvailable(WebKitMediaCommonEncryptionDecrypt*);
static void setContext(GstElement*, GstContext*);
static void processProtectionEvents(WebKitMediaCommonEncryptionDecrypt* self);

GST_DEBUG_CATEGORY_STATIC(webkit_media_common_encryption_decrypt_debug_category);
#define GST_CAT_DEFAULT webkit_media_common_encryption_decrypt_debug_category

#define webkit_media_common_encryption_decrypt_parent_class parent_class
G_DEFINE_TYPE(WebKitMediaCommonEncryptionDecrypt, webkit_media_common_encryption_decrypt, GST_TYPE_BASE_TRANSFORM);

static void webkit_media_common_encryption_decrypt_class_init(WebKitMediaCommonEncryptionDecryptClass* klass)
{
    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);
    gobjectClass->finalize = finalize;

    GST_DEBUG_CATEGORY_INIT(webkit_media_common_encryption_decrypt_debug_category,
        "webkitcenc", 0, "Common Encryption base class");

    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    elementClass->change_state = GST_DEBUG_FUNCPTR(changeState);
    elementClass->set_context = GST_DEBUG_FUNCPTR(setContext);

    GstBaseTransformClass* baseTransformClass = GST_BASE_TRANSFORM_CLASS(klass);
    baseTransformClass->transform_ip = GST_DEBUG_FUNCPTR(transformInPlace);
    baseTransformClass->transform_caps = GST_DEBUG_FUNCPTR(transformCaps);
    baseTransformClass->transform_ip_on_passthrough = FALSE;
    baseTransformClass->sink_event = GST_DEBUG_FUNCPTR(sinkEventHandler);
    baseTransformClass->query = GST_DEBUG_FUNCPTR(queryHandler);

    klass->handleInitData = [](WebKitMediaCommonEncryptionDecrypt*, const WebCore::InitData&) { return true; };
    klass->attemptToDecryptWithLocalInstance = [](WebKitMediaCommonEncryptionDecrypt*, const WebCore::InitData&) { return false; };

    g_type_class_add_private(klass, sizeof(WebKitMediaCommonEncryptionDecryptPrivate));
}

static void webkit_media_common_encryption_decrypt_init(WebKitMediaCommonEncryptionDecrypt* self)
{
    WebKitMediaCommonEncryptionDecryptPrivate* priv = WEBKIT_MEDIA_CENC_DECRYPT_GET_PRIVATE(self);

    self->priv = priv;
    new (priv) WebKitMediaCommonEncryptionDecryptPrivate();

    GstBaseTransform* base = GST_BASE_TRANSFORM(self);
    gst_base_transform_set_in_place(base, TRUE);
    gst_base_transform_set_passthrough(base, FALSE);
    gst_base_transform_set_gap_aware(base, FALSE);
}

static void finalize(GObject* object)
{
    WebKitMediaCommonEncryptionDecrypt* self = WEBKIT_MEDIA_CENC_DECRYPT(object);
    WebKitMediaCommonEncryptionDecryptPrivate* priv = self->priv;

    priv->~WebKitMediaCommonEncryptionDecryptPrivate();
    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static GstCaps* transformCaps(GstBaseTransform* base, GstPadDirection direction, GstCaps* caps, GstCaps* filter)
{
    if (direction == GST_PAD_UNKNOWN)
        return nullptr;

    GST_DEBUG_OBJECT(base, "direction: %s, caps: %" GST_PTR_FORMAT " filter: %" GST_PTR_FORMAT, (direction == GST_PAD_SRC) ? "src" : "sink", caps, filter);

    GstCaps* transformedCaps = gst_caps_new_empty();
    WebKitMediaCommonEncryptionDecrypt* self = WEBKIT_MEDIA_CENC_DECRYPT(base);
    WebKitMediaCommonEncryptionDecryptClass* klass = WEBKIT_MEDIA_CENC_DECRYPT_GET_CLASS(self);

    unsigned size = gst_caps_get_size(caps);
    for (unsigned i = 0; i < size; ++i) {
        GstStructure* incomingStructure = gst_caps_get_structure(caps, i);
        GUniquePtr<GstStructure> outgoingStructure = nullptr;

        if (direction == GST_PAD_SINK) {
            if (!gst_structure_has_field(incomingStructure, "original-media-type"))
                continue;

            outgoingStructure = GUniquePtr<GstStructure>(gst_structure_copy(incomingStructure));
            gst_structure_set_name(outgoingStructure.get(), gst_structure_get_string(outgoingStructure.get(), "original-media-type"));

            // Filter out the DRM related fields from the down-stream caps.
            gst_structure_remove_fields(outgoingStructure.get(), "protection-system", "original-media-type", "encryption-algorithm", "encoding-scope", "cipher-mode", nullptr);
        } else {
            outgoingStructure = GUniquePtr<GstStructure>(gst_structure_copy(incomingStructure));
            // Filter out the video related fields from the up-stream caps,
            // because they are not relevant to the input caps of this element and
            // can cause caps negotiation failures with adaptive bitrate streams.
            gst_structure_remove_fields(outgoingStructure.get(), "base-profile", "codec_data", "height", "framerate", "level", "pixel-aspect-ratio", "profile", "rate", "width", nullptr);

            gst_structure_set(outgoingStructure.get(), "protection-system", G_TYPE_STRING, klass->protectionSystemId,
                "original-media-type", G_TYPE_STRING, gst_structure_get_name(incomingStructure), nullptr);

            // GST_PROTECTION_UNSPECIFIED_SYSTEM_ID was added in the GStreamer
            // developement git master which will ship as version 1.16.0.
            gst_structure_set_name(outgoingStructure.get(),
#if GST_CHECK_VERSION(1, 15, 0)
                !g_strcmp0(klass->protectionSystemId, GST_PROTECTION_UNSPECIFIED_SYSTEM_ID) ? "application/x-webm-enc" :
#endif
                "application/x-cenc");
        }

        bool duplicate = false;
        unsigned size = gst_caps_get_size(transformedCaps);

        for (unsigned index = 0; !duplicate && index < size; ++index) {
            GstStructure* structure = gst_caps_get_structure(transformedCaps, index);
            if (gst_structure_is_equal(structure, outgoingStructure.get()))
                duplicate = true;
        }

        if (!duplicate)
            gst_caps_append_structure(transformedCaps, outgoingStructure.release());
    }

    if (filter) {
        GstCaps* intersection;

        GST_DEBUG_OBJECT(base, "Using filter caps %" GST_PTR_FORMAT, filter);
        intersection = gst_caps_intersect_full(transformedCaps, filter, GST_CAPS_INTERSECT_FIRST);
        gst_caps_unref(transformedCaps);
        transformedCaps = intersection;
    }

    GST_DEBUG_OBJECT(base, "returning %" GST_PTR_FORMAT, transformedCaps);
    return transformedCaps;
}

static GstFlowReturn transformInPlace(GstBaseTransform* base, GstBuffer* buffer)
{
    WebKitMediaCommonEncryptionDecrypt* self = WEBKIT_MEDIA_CENC_DECRYPT(base);
    WebKitMediaCommonEncryptionDecryptPrivate* priv = WEBKIT_MEDIA_CENC_DECRYPT_GET_PRIVATE(self);
    
    LockHolder locker(priv->mutex);

    if (priv->isFlushing) {
        GST_DEBUG_OBJECT(self, "flushing");
        return GST_FLOW_FLUSHING;
    }

    GstProtectionMeta* protectionMeta = reinterpret_cast<GstProtectionMeta*>(gst_buffer_get_protection_meta(buffer));
    if (!protectionMeta) {
        GST_TRACE_OBJECT(self, "buffer %p has no protection meta, assuming it's not encrypted", buffer);
        return GST_FLOW_OK;
    }

    const GValue* streamEncryptionEventsList = gst_structure_get_value(protectionMeta->info, "stream-encryption-events");
    if (streamEncryptionEventsList && GST_VALUE_HOLDS_LIST(streamEncryptionEventsList)) {
        unsigned streamEncryptionEventsListSize = gst_value_list_get_size(streamEncryptionEventsList);
        for (unsigned i = 0; i < streamEncryptionEventsListSize; ++i)
            priv->protectionEvents.append(GRefPtr<GstEvent>(static_cast<GstEvent*>(g_value_get_boxed(gst_value_list_get_value(streamEncryptionEventsList, i)))));
        gst_structure_remove_field(protectionMeta->info, "stream-encryption-events");
        if (!gst_structure_n_fields(protectionMeta->info)) {
            GST_ERROR_OBJECT(self, "buffer %p did not have enough protection meta-data", buffer);
            return GST_FLOW_NOT_SUPPORTED;
        }
    }

    if (!priv->protectionEvents.isEmpty())
        processProtectionEvents(self);

    // The key might not have been received yet. Wait for it.
    if (!priv->keyReceived) {
        GST_DEBUG_OBJECT(self, "key not available yet, waiting for it");
        if (GST_STATE(GST_ELEMENT(self)) < GST_STATE_PAUSED || (GST_STATE_TARGET(GST_ELEMENT(self)) != GST_STATE_VOID_PENDING && GST_STATE_TARGET(GST_ELEMENT(self)) < GST_STATE_PAUSED)) {
            GST_ERROR_OBJECT(self, "can't process key requests in less than PAUSED state");
            return GST_FLOW_NOT_SUPPORTED;
        }
        // Send "drm-cdm-instance-needed" message to the player to resend the CDMInstance if available and inform we are waiting for key.
        priv->waitingForKey = true;
        gst_element_post_message(GST_ELEMENT(self), gst_message_new_element(GST_OBJECT(self), gst_structure_new_empty("drm-waiting-for-key")));

        if (!priv->condition.waitFor(priv->mutex, WEBCORE_GSTREAMER_EME_LICENSE_KEY_RESPONSE_TIMEOUT, [priv] { return priv->isFlushing || priv->keyReceived; })) {
            if (priv->isFlushing) {
                GST_DEBUG_OBJECT(self, "flushing");
                return GST_FLOW_FLUSHING;
            }
            GST_ERROR_OBJECT(self, "key not available");
            return GST_FLOW_NOT_SUPPORTED;
        }
        GST_DEBUG_OBJECT(self, "key received, continuing");
     
        priv->waitingForKey = false;
        gst_element_post_message(GST_ELEMENT(self), gst_message_new_element(GST_OBJECT(self), gst_structure_new_empty("drm-key-received")));
    }

    unsigned ivSize;
    if (!gst_structure_get_uint(protectionMeta->info, "iv_size", &ivSize)) {
        GST_ERROR_OBJECT(self, "Failed to get iv_size");
        gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));
        return GST_FLOW_NOT_SUPPORTED;
    }

    gboolean encrypted;
    if (!gst_structure_get_boolean(protectionMeta->info, "encrypted", &encrypted)) {
        GST_ERROR_OBJECT(self, "Failed to get encrypted flag");
        gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));
        return GST_FLOW_NOT_SUPPORTED;
    }

    if (!ivSize || !encrypted) {
        gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));
        return GST_FLOW_OK;
    }

    GST_DEBUG_OBJECT(base, "protection meta: %" GST_PTR_FORMAT, protectionMeta->info);

    unsigned subSampleCount;
    if (!gst_structure_get_uint(protectionMeta->info, "subsample_count", &subSampleCount)) {
        GST_ERROR_OBJECT(self, "Failed to get subsample_count");
        gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));
        return GST_FLOW_NOT_SUPPORTED;
    }

    const GValue* value;
    GstBuffer* subSamplesBuffer = nullptr;
    if (subSampleCount) {
        value = gst_structure_get_value(protectionMeta->info, "subsamples");
        if (!value) {
            GST_ERROR_OBJECT(self, "Failed to get subsamples");
            gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));
            return GST_FLOW_NOT_SUPPORTED;
        }
        subSamplesBuffer = gst_value_get_buffer(value);
    }

    value = gst_structure_get_value(protectionMeta->info, "kid");

    if (!value) {
        GST_ERROR_OBJECT(self, "Failed to get key id for buffer");
        gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));
        return GST_FLOW_NOT_SUPPORTED;
    }
    GstBuffer* keyIDBuffer = gst_value_get_buffer(value);

    value = gst_structure_get_value(protectionMeta->info, "iv");
    if (!value) {
        GST_ERROR_OBJECT(self, "Failed to get IV for sample");
        gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));
        return GST_FLOW_NOT_SUPPORTED;
    }

    GstBuffer* ivBuffer = gst_value_get_buffer(value);
    WebKitMediaCommonEncryptionDecryptClass* klass = WEBKIT_MEDIA_CENC_DECRYPT_GET_CLASS(self);

    GST_TRACE_OBJECT(self, "decrypting");
    if (!klass->decrypt(self, ivBuffer, keyIDBuffer, buffer, subSampleCount, subSamplesBuffer)) {
        GST_ERROR_OBJECT(self, "Decryption failed");
        gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));
        return GST_FLOW_NOT_SUPPORTED;
    }

    gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));
    return GST_FLOW_OK;
}

static bool isCDMInstanceAvailable(WebKitMediaCommonEncryptionDecrypt* self)
{
    WebKitMediaCommonEncryptionDecryptPrivate* priv = WEBKIT_MEDIA_CENC_DECRYPT_GET_PRIVATE(self);

    ASSERT(priv->mutex.isLocked());

    if (!priv->cdmInstance) {
        GRefPtr<GstContext> context = adoptGRef(gst_element_get_context(GST_ELEMENT(self), "drm-cdm-instance"));
        // According to the GStreamer documentation, if we can't find the context, we should run a downstream query, then an upstream one and then send a bus
        // message. In this case that does not make a lot of sense since only the app (player) answers it, meaning that no query is going to solve it. A message
        // could be helpful but the player sets the context as soon as it gets the CDMInstance and if it does not have it, we have no way of asking for one as it is
        // something provided by crossplatform code. This means that we won't be able to answer the bus request in any way either. Summing up, neither queries nor bus
        // requests are useful here.
        if (context) {
            const GValue* value = gst_structure_get_value(gst_context_get_structure(context.get()), "cdm-instance");
            // Capture the CDMInstance into a separate variable to avoid missing a refcount.
            CDMInstance* instance = value ? reinterpret_cast<CDMInstance*>(g_value_get_pointer(value)) : nullptr;
            // ... And force a refcount bump using operator=.
            priv->cdmInstance = instance;
            if (priv->cdmInstance)
                GST_DEBUG_OBJECT(self, "received a new CDM instance %p", priv->cdmInstance.get());
            else
                GST_TRACE_OBJECT(self, "CDM instance was detached");
        }
    }

    GST_TRACE_OBJECT(self, "CDM instance available %s", boolForPrinting(priv->cdmInstance.get()));
    return priv->cdmInstance;
}

static void processProtectionEvents(WebKitMediaCommonEncryptionDecrypt* self)
{
    WebKitMediaCommonEncryptionDecryptPrivate* priv = WEBKIT_MEDIA_CENC_DECRYPT_GET_PRIVATE(self);
    WebKitMediaCommonEncryptionDecryptClass* klass = WEBKIT_MEDIA_CENC_DECRYPT_GET_CLASS(self);

    ASSERT(priv->mutex.isLocked());

    bool cdmInstanceAvailable = isCDMInstanceAvailable(self);

    WebCore::InitData concatenatedInitDatas;
    for (auto& event : priv->protectionEvents) {
        GstBuffer* buffer = nullptr;
        const char* eventKeySystemUUID = nullptr;
        gst_event_parse_protection(event.get(), &eventKeySystemUUID, &buffer, nullptr);

        GST_TRACE_OBJECT(self, "handling protection event %u for %s", GST_EVENT_SEQNUM(event.get()), eventKeySystemUUID);

        if (cdmInstanceAvailable && g_strcmp0(eventKeySystemUUID, WebCore::keySystemToUuid(priv->cdmInstance->keySystem()))) {
            GST_TRACE_OBJECT(self, "protection event for a different key system");
            continue;
        }

        if (priv->currentEvent == GST_EVENT_SEQNUM(event.get())) {
            GST_TRACE_OBJECT(self, "event %u already handled", priv->currentEvent);
            continue;
        }

        WebCore::InitData initData;
        if (cdmInstanceAvailable)
            initData = priv->initDatas.get(WebCore::keySystemToUuid(priv->cdmInstance->keySystem()));

        priv->currentEvent = GST_EVENT_SEQNUM(event.get());

        if (!initData.size() || gst_buffer_memcmp(buffer, 0, initData.data(), initData.size())) {
            auto mappedBuffer = GstMappedBuffer::create(buffer, GST_MAP_READ);
            if (!mappedBuffer) {
                GST_WARNING_OBJECT(self, "cannot map protection data");
                continue;
            }

            initData = WebCore::InitData(mappedBuffer->createSharedBuffer());
            GST_DEBUG_OBJECT(self, "init data of size %u", mappedBuffer->size());
            GST_TRACE_OBJECT(self, "init data MD5 %s", initDataMD5(initData).utf8().data());
            GST_MEMDUMP_OBJECT(self, "init data", mappedBuffer->data(), mappedBuffer->size());
            priv->initDatas.set(eventKeySystemUUID, initData);

            priv->keyReceived = cdmInstanceAvailable && !klass->handleInitData(self, initData);
            if (!priv->keyReceived) {
                if (cdmInstanceAvailable) {
                    GST_DEBUG_OBJECT(self, "posting event and considering key not received");
                    gst_element_post_message(GST_ELEMENT(self), gst_message_new_element(GST_OBJECT(self),
                        gst_structure_new("drm-initialization-data-encountered", "init-data", GST_TYPE_BUFFER, buffer, "key-system-uuid", G_TYPE_STRING, eventKeySystemUUID, nullptr)));
                    break;
                } else {
                    GST_TRACE_OBJECT(self, "concatenating init data and considering key not received");
                    priv->currentEvent = 0;
                    concatenatedInitDatas.append(WTFMove(initData));
                }
            } else {
                GST_DEBUG_OBJECT(self, "key is already usable");
                priv->condition.notifyOne();
                break;
            }
        } else {
            GST_DEBUG_OBJECT(self, "init data already present");
            break;
        }
    }

    priv->protectionEvents.clear();

    if (!cdmInstanceAvailable && !concatenatedInitDatas.isEmpty()) {
        GRefPtr<GstBuffer> buffer = adoptGRef(gst_buffer_new_allocate(nullptr, concatenatedInitDatas.size(), nullptr));
        auto mappedBuffer = GstMappedBuffer::create(buffer.get(), GST_MAP_WRITE);
        if (!mappedBuffer) {
            GST_WARNING_OBJECT(self, "cannot map writable init data");
            return;
        }
        memcpy(mappedBuffer->data(), concatenatedInitDatas.data(), concatenatedInitDatas.size());
        GST_DEBUG_OBJECT(self, "reporting concatenated init datas of size %u", concatenatedInitDatas.size());
        GST_TRACE_OBJECT(self, "init data MD5 %s", initDataMD5(concatenatedInitDatas).utf8().data());
        GST_MEMDUMP_OBJECT(self, "init data", concatenatedInitDatas.data(), concatenatedInitDatas.size());
        gst_element_post_message(GST_ELEMENT(self), gst_message_new_element(GST_OBJECT(self), gst_structure_new("drm-initialization-data-encountered", "init-data", GST_TYPE_BUFFER, buffer.get(), nullptr)));
    }
}

static gboolean sinkEventHandler(GstBaseTransform* trans, GstEvent* event)
{
    WebKitMediaCommonEncryptionDecrypt* self = WEBKIT_MEDIA_CENC_DECRYPT(trans);
    WebKitMediaCommonEncryptionDecryptPrivate* priv = WEBKIT_MEDIA_CENC_DECRYPT_GET_PRIVATE(self);
    WebKitMediaCommonEncryptionDecryptClass* klass = WEBKIT_MEDIA_CENC_DECRYPT_GET_CLASS(self);
    gboolean result = FALSE;

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_PROTECTION: {
        const char* systemId = nullptr;

        gst_event_parse_protection(event, &systemId, nullptr, nullptr);
        GST_TRACE_OBJECT(self, "received protection event %u for %s", GST_EVENT_SEQNUM(event), systemId);

        LockHolder locker(priv->mutex);
        priv->protectionEvents.append(event);
        result = TRUE;
        gst_event_unref(event);
        break;
    }
    case GST_EVENT_CUSTOM_DOWNSTREAM_OOB: {
        const GstStructure* structure = gst_event_get_structure(event);
        if (gst_structure_has_name(structure, "attempt-to-decrypt")) {
            gst_event_unref(event);
            result = TRUE;
            LockHolder locker(priv->mutex);
            RELEASE_ASSERT(isCDMInstanceAvailable(self));
            priv->keyReceived = klass->attemptToDecryptWithLocalInstance(self, priv->initDatas.get(WebCore::keySystemToUuid(priv->cdmInstance->keySystem())));
            GST_DEBUG_OBJECT(self, "attempted to decrypt with local instance %p, key received %s", priv->cdmInstance.get(), WTF::boolForPrinting(priv->keyReceived));
            if (priv->keyReceived)
                priv->condition.notifyOne();
        }
        break;
    }
    case GST_EVENT_FLUSH_START: {
        {
            LockHolder locker(priv->mutex);
            ASSERT(!priv->isFlushing);
            priv->isFlushing = true;
            GST_DEBUG_OBJECT(self, "flushing");
            priv->condition.notifyOne();
        }
        result = GST_BASE_TRANSFORM_CLASS(parent_class)->sink_event(trans, event);
        break;
    }
    case GST_EVENT_FLUSH_STOP: {
        {
            LockHolder locker(priv->mutex);
            ASSERT(priv->isFlushing);
            priv->isFlushing = false;
            GST_DEBUG_OBJECT(self, "flushing done");
        }
        result = GST_BASE_TRANSFORM_CLASS(parent_class)->sink_event(trans, event);
        break;
    }
    default:
        result = GST_BASE_TRANSFORM_CLASS(parent_class)->sink_event(trans, event);
        break;
    }

    return result;
}

static gboolean queryHandler(GstBaseTransform* trans, GstPadDirection direction, GstQuery* query)
{
    if (gst_structure_has_name(gst_query_get_structure(query), "any-decryptor-waiting-for-key")) {
        WebKitMediaCommonEncryptionDecryptPrivate* priv = WEBKIT_MEDIA_CENC_DECRYPT_GET_PRIVATE(trans);
        GST_TRACE_OBJECT(trans, "any-decryptor-waiting-for-key query, waitingforkey %s", boolForPrinting(priv->waitingForKey));
        return priv->waitingForKey;
    }
    return GST_BASE_TRANSFORM_CLASS(parent_class)->query(trans, direction, query);
}

static GstStateChangeReturn changeState(GstElement* element, GstStateChange transition)
{
    WebKitMediaCommonEncryptionDecrypt* self = WEBKIT_MEDIA_CENC_DECRYPT(element);
    WebKitMediaCommonEncryptionDecryptPrivate* priv = WEBKIT_MEDIA_CENC_DECRYPT_GET_PRIVATE(self);

    switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        GST_DEBUG_OBJECT(self, "PAUSED->READY");
        priv->condition.notifyOne();
        break;
    default:
        break;
    }

    GstStateChangeReturn result = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);

    // Add post-transition code here.

    return result;
}

static void setContext(GstElement* element, GstContext* context)
{
    WebKitMediaCommonEncryptionDecrypt* self = WEBKIT_MEDIA_CENC_DECRYPT(element);
    WebKitMediaCommonEncryptionDecryptPrivate* priv = WEBKIT_MEDIA_CENC_DECRYPT_GET_PRIVATE(self);

    if (gst_context_has_context_type(context, "drm-cdm-instance")) {
        const GValue* value = gst_structure_get_value(gst_context_get_structure(context), "cdm-instance");
        LockHolder locker(priv->mutex);
        priv->cdmInstance = value ? reinterpret_cast<CDMInstance*>(g_value_get_pointer(value)) : nullptr;
        GST_DEBUG_OBJECT(self, "received new CDMInstance %p", priv->cdmInstance.get());
        return;
    }

    GST_ELEMENT_CLASS(parent_class)->set_context(element, context);
}

RefPtr<CDMInstance> getCDMInstance(WebKitMediaCommonEncryptionDecrypt* self)
{
    WebKitMediaCommonEncryptionDecryptPrivate* priv = WEBKIT_MEDIA_CENC_DECRYPT_GET_PRIVATE(self);
    ASSERT(priv->mutex.isLocked());
    return isCDMInstanceAvailable(self) ? priv->cdmInstance : nullptr;
}

#endif // ENABLE(ENCRYPTED_MEDIA) && USE(GSTREAMER)
