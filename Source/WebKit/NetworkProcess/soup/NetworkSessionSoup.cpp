/*
 * Copyright (C) 2016 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "NetworkSessionSoup.h"

#include "NetworkProcess.h"
#include "NetworkSessionCreationParameters.h"
#include "WebCookieManager.h"
#include "WebSocketTaskSoup.h"
#include <WebCore/DeprecatedGlobalSettings.h>
#include <WebCore/NetworkStorageSession.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/SoupNetworkSession.h>
#include <libsoup/soup.h>

namespace WebKit {
using namespace WebCore;

NetworkSessionSoup::NetworkSessionSoup(NetworkProcess& networkProcess, NetworkSessionCreationParameters&& parameters)
    : NetworkSession(networkProcess, parameters)
    , m_networkSession(makeUnique<SoupNetworkSession>(m_sessionID))
{
    auto* storageSession = networkStorageSession();
    ASSERT(storageSession);

    if (!parameters.cookiePersistentStoragePath.isEmpty())
        setCookiePersistentStorage(parameters.cookiePersistentStoragePath, parameters.cookiePersistentStorageType);
    else
        m_networkSession->setCookieJar(storageSession->cookieStorage());

    storageSession->setCookieObserverHandler([this] {
        this->networkProcess().supplement<WebCookieManager>()->notifyCookiesDidChange(m_sessionID);
    });
}

NetworkSessionSoup::~NetworkSessionSoup()
{
    if (auto* storageSession = networkProcess().storageSession(m_sessionID))
        storageSession->setCookieObserverHandler(nullptr);
}

SoupSession* NetworkSessionSoup::soupSession() const
{
    return m_networkSession->soupSession();
}

void NetworkSessionSoup::setCookiePersistentStorage(const String& storagePath, SoupCookiePersistentStorageType storageType)
{
    auto* storageSession = networkStorageSession();
    if (!storageSession)
        return;

    GRefPtr<SoupCookieJar> jar;
    switch (storageType) {
    case SoupCookiePersistentStorageType::Text:
        jar = adoptGRef(soup_cookie_jar_text_new(storagePath.utf8().data(), FALSE));
        break;
    case SoupCookiePersistentStorageType::SQLite:
        jar = adoptGRef(soup_cookie_jar_db_new(storagePath.utf8().data(), FALSE));
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    soup_cookie_jar_set_accept_policy(jar.get(), soup_cookie_jar_get_accept_policy(storageSession->cookieStorage()));
    storageSession->setCookieStorage(WTFMove(jar));

    m_networkSession->setCookieJar(storageSession->cookieStorage());
}

void NetworkSessionSoup::clearCredentials()
{
#if SOUP_CHECK_VERSION(2, 57, 1)
    soup_auth_manager_clear_cached_credentials(SOUP_AUTH_MANAGER(soup_session_get_feature(soupSession(), SOUP_TYPE_AUTH_MANAGER)));
#endif
}

#if USE(SOUP2)
static gboolean webSocketAcceptCertificateCallback(GTlsConnection*, GTlsCertificate* certificate, GTlsCertificateFlags errors, SoupMessage* soupMessage)
{
    if (DeprecatedGlobalSettings::allowsAnySSLCertificate())
        return TRUE;

    return !SoupNetworkSession::checkTLSErrors(soupURIToURL(soup_message_get_uri(soupMessage)), certificate, errors);
}

static void webSocketMessageNetworkEventCallback(SoupMessage* soupMessage, GSocketClientEvent event, GIOStream* connection)
{
    if (event != G_SOCKET_CLIENT_TLS_HANDSHAKING)
        return;

    g_signal_connect(connection, "accept-certificate", G_CALLBACK(webSocketAcceptCertificateCallback), soupMessage);
}
#endif

std::unique_ptr<WebSocketTask> NetworkSessionSoup::createWebSocketTask(NetworkSocketChannel& channel, const ResourceRequest& request, const String& protocol)
{
    GRefPtr<SoupMessage> soupMessage = request.createSoupMessage(blobRegistry());
    if (!soupMessage)
        return nullptr;

    if (request.url().protocolIs("wss")) {
#if USE(SOUP2)
        g_signal_connect(soupMessage.get(), "network-event", G_CALLBACK(webSocketMessageNetworkEventCallback), nullptr);
#else
        g_signal_connect(soupMessage.get(), "accept-certificate", G_CALLBACK(+[](SoupMessage* message, GTlsCertificate* certificate, GTlsCertificateFlags errors, gpointer) -> gboolean {
            if (DeprecatedGlobalSettings::allowsAnySSLCertificate())
                return TRUE;

            return !SoupNetworkSession::checkTLSErrors(soupURIToURL(soup_message_get_uri(message)), certificate, errors);
        }), nullptr);
#endif
    }
    return makeUnique<WebSocketTask>(channel, request, soupSession(), soupMessage.get(), protocol);
}

} // namespace WebKit
