/*
 * Copyright (C) 2018 Metrological Group B.V.
 * Copyright (C) 2018 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "NicosiaGC3DLayer.h"

#if USE(NICOSIA) && USE(TEXTURE_MAPPER)

#if USE(COORDINATED_GRAPHICS_THREADED)
#include "TextureMapperGL.h"
#include "TextureMapperPlatformLayerBuffer.h"
#include "TextureMapperPlatformLayerProxy.h"
#endif

#include "GLContext.h"
#include "HostWindow.h"

namespace Nicosia {

using namespace WebCore;

static std::unique_ptr<GLContext> s_windowContext;

GC3DLayer::GC3DLayer(GraphicsContext3D& context, GraphicsContext3D::RenderStyle renderStyle, const HostWindow* hostWindow)
    : m_context(context)
    , m_renderStyle(renderStyle)
    , m_contentLayer(Nicosia::ContentLayer::create(Nicosia::ContentLayerTextureMapperImpl::createFactory(*this)))
{
    switch (renderStyle) {
    case GraphicsContext3D::RenderOffscreen:
        m_glContext = GLContext::createOffscreenContext(&PlatformDisplay::sharedDisplayForCompositing());
        break;
    case GraphicsContext3D::RenderDirectlyToHostWindow:
        if (!s_windowContext)
            s_windowContext = GLContext::createContextForWindow(reinterpret_cast<GLNativeWindowType>(hostWindow->nativeWindowID()), &PlatformDisplay::sharedDisplayForCompositing());
        break;
    }
}

GC3DLayer::~GC3DLayer()
{
    downcast<ContentLayerTextureMapperImpl>(m_contentLayer->impl()).invalidateClient();
}

GLContext* GC3DLayer::glContext()
{
    ASSERT((m_glContext && m_renderStyle == GraphicsContext3D::RenderOffscreen) || (s_windowContext && m_renderStyle == GraphicsContext3D::RenderDirectlyToHostWindow));
    return m_renderStyle == GraphicsContext3D::RenderOffscreen ? m_glContext.get() : s_windowContext.get();
}

bool GC3DLayer::makeContextCurrent()
{
    ASSERT(glContext());
    return glContext()->makeContextCurrent();
}

PlatformGraphicsContext3D GC3DLayer::platformContext()
{
    ASSERT(glContext());
    return glContext()->platformContext();
}

void GC3DLayer::swapBuffersIfNeeded()
{
    if (m_renderStyle == GraphicsContext3D::RenderDirectlyToHostWindow) {
        glContext()->swapBuffers();
        return;
    }

#if USE(COORDINATED_GRAPHICS_THREADED)
    if (m_context.layerComposited())
        return;

    m_context.prepareTexture();
    IntSize textureSize(m_context.m_currentWidth, m_context.m_currentHeight);
    TextureMapperGL::Flags flags = TextureMapperGL::ShouldFlipTexture | (m_context.m_attrs.alpha ? TextureMapperGL::ShouldBlend : 0);

    {
        auto& proxy = downcast<Nicosia::ContentLayerTextureMapperImpl>(m_contentLayer->impl()).proxy();

        LockHolder holder(proxy.lock());
        proxy.pushNextBuffer(std::make_unique<TextureMapperPlatformLayerBuffer>(m_context.m_compositorTexture, textureSize, flags, m_context.m_internalColorFormat));
    }

    m_context.markLayerComposited();
#endif
}

} // namespace Nicosia

#endif // USE(NICOSIA) && USE(TEXTURE_MAPPER)
