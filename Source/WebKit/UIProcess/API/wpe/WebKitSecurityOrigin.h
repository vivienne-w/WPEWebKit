/*
 * Copyright (C) 2017 Igalia S.L.
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#if !defined(__WEBKIT_H_INSIDE__) && !defined(WEBKIT2_COMPILATION) && !defined(__WEBKIT_WEB_EXTENSION_H_INSIDE__)
#error "Only <wpe/webkit.h> can be included directly."
#endif

#ifndef WebKitSecurityOrigin_h
#define WebKitSecurityOrigin_h

#include <glib-object.h>
#include <wpe/WebKitDefines.h>

G_BEGIN_DECLS

#define WEBKIT_TYPE_SECURITY_ORIGIN (webkit_security_origin_get_type())

typedef struct _WebKitSecurityOrigin WebKitSecurityOrigin;

WEBKIT_API GType
webkit_security_origin_get_type     (void);

WEBKIT_API WebKitSecurityOrigin *
webkit_security_origin_new          (const gchar          *protocol,
                                     const gchar          *host,
                                     guint16               port);

WEBKIT_API WebKitSecurityOrigin *
webkit_security_origin_new_for_uri  (const gchar          *uri);

WEBKIT_API WebKitSecurityOrigin *
webkit_security_origin_ref          (WebKitSecurityOrigin *origin);

WEBKIT_API void
webkit_security_origin_unref        (WebKitSecurityOrigin *origin);

WEBKIT_API const gchar *
webkit_security_origin_get_protocol (WebKitSecurityOrigin *origin);

WEBKIT_API const gchar *
webkit_security_origin_get_host     (WebKitSecurityOrigin *origin);

WEBKIT_API guint16
webkit_security_origin_get_port     (WebKitSecurityOrigin *origin);

WEBKIT_API gboolean
webkit_security_origin_is_opaque    (WebKitSecurityOrigin *origin);

WEBKIT_API gchar *
webkit_security_origin_to_string    (WebKitSecurityOrigin *origin);

G_END_DECLS

#endif /* WebKitSecurityOrigin_h */
