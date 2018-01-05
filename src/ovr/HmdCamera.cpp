/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 —
            Vladimír Vondruš <mosra@centrum.cz>
        2015, 2016 — Jonathan Hale <squareys@googlemail.com>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "HmdCamera.h"

#include <Magnum/Math/Vector.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Texture.h>
#include <Magnum/Framebuffer.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Extensions.h>
#include <Magnum/Context.h>
#include <Magnum/Image.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/OvrIntegration/Session.h>

namespace Magnum { namespace Examples {

using namespace OvrIntegration;

HmdCamera::HmdCamera(Session& session, int eye, SceneGraph::AbstractObject3D& object): SceneGraph::Camera3D(object), _session(session) {
    _textureSize = _session.fovTextureSize(eye);

    createEyeRenderTexture();

    setViewport(_textureSize);
    setProjectionMatrix(_session.projectionMatrix(eye, 0.001f, 100.0f));
}

void HmdCamera::createEyeRenderTexture() {
    _textureSwapChain =_session.createTextureSwapChain(_textureSize);

    /* Create the framebuffer which will be used to render to the current
       texture of the texture set later. */
    _framebuffer.reset(new Framebuffer({{}, _textureSize}));
    _framebuffer->mapForDraw(Framebuffer::ColorAttachment(0));

    /* Setup depth attachment */
    TextureFormat format = TextureFormat::DepthComponent24;
    if(Magnum::Context::current().isExtensionSupported<Extensions::GL::ARB::depth_buffer_float>())
        format = TextureFormat::DepthComponent32F;

    _depth.reset(new Texture2D);
    _depth->setMinificationFilter(Sampler::Filter::Linear)
        .setMagnificationFilter(Sampler::Filter::Linear)
        .setWrapping(Sampler::Wrapping::ClampToEdge)
        .setStorage(1, format, _textureSize);
}

void HmdCamera::draw(SceneGraph::DrawableGroup3D& group) {
    /* Switch to eye render target and bind render textures */
    _framebuffer->bind();

    _framebuffer->attachTexture(Framebuffer::ColorAttachment(0), _textureSwapChain->activeTexture(), 0)
        .attachTexture(Framebuffer::BufferAttachment::Depth, *_depth, 0)
        /* Clear with the standard grey so that at least that will be visible in
           case the scene is not correctly set up */
        .clear(FramebufferClear::Color | FramebufferClear::Depth);

    /* Render scene */
    SceneGraph::Camera3D::draw(group);

    /* Commit changes and use next texture in chain */
    _textureSwapChain->commit();

    /* Reasoning for the next two lines, taken from the Oculus SDK examples
       code: Without this, [during the next frame, this method] would bind a
       framebuffer with an invalid COLOR_ATTACHMENT0 because the texture ID
       associated with COLOR_ATTACHMENT0 had been unlocked by calling
       wglDXUnlockObjectsNV(). */
    _framebuffer->detach(Framebuffer::ColorAttachment(0))
        .detach(Framebuffer::BufferAttachment::Depth);
}

}}
