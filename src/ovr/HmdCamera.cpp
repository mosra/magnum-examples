/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015
              Vladimír Vondruš <mosra@centrum.cz>
    Copyright © 2015
              Jonathan Hale <squareys@googlemail.com>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "HmdCamera.h"

#include <Magnum/Math/Vector.h>
#include <Magnum/Texture.h>
#include <Magnum/Framebuffer.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/ColorFormat.h>
#include <Magnum/Extensions.h>
#include <Magnum/Context.h>
#include <Magnum/Image.h>
#include <Magnum/DefaultFramebuffer.h>

#include <Magnum/LibOvrIntegration/Hmd.h>

namespace Magnum { namespace Examples {

using namespace LibOvrIntegration;

HmdCamera::HmdCamera(Hmd& hmd, int eye, SceneGraph::AbstractObject3D& object): SceneGraph::Camera3D(object), _hmd(hmd) {
    _textureSize = _hmd.getFovTextureSize(eye);

    createEyeRenderTexture();

    setViewport(_textureSize);

    const Float near = 0.001f;
    const Float far = 100.0f;
    setPerspective(_hmd.defaultEyeFov(eye) * near, near, far);
}

void HmdCamera::createEyeRenderTexture() {
    _textureSet =_hmd.createSwapTextureSet(TextureFormat::RGBA, _textureSize);

    /*
     * create the framebuffer which will be used to render to the current texture
     * of the texture set later.
     */
    _framebuffer.reset(new Framebuffer({{}, _textureSize}));
    _framebuffer->mapForDraw(Framebuffer::ColorAttachment(0));

    /* setup depth attachment */
    ColorType type = ColorType::UnsignedInt;
    TextureFormat format = TextureFormat::DepthComponent24;

    if(Context::current()->isExtensionSupported<Extensions::GL::ARB::depth_buffer_float>()) {
        format = TextureFormat::DepthComponent32F;
        type = ColorType::Float;
    }

    Image2D image(ColorFormat::DepthComponent, type, _textureSize, nullptr);

    _depth.reset(new Texture2D());
    _depth->setMinificationFilter(Sampler::Filter::Linear)
           .setMagnificationFilter(Sampler::Filter::Linear)
           .setWrapping(Sampler::Wrapping::ClampToEdge)
           .setStorage(1, format, _textureSize)
           .subImage(0, {{}, _textureSize}, image);
}

void HmdCamera::draw(SceneGraph::DrawableGroup3D& group) {
    /* increment to use next texture */
    _textureSet->increment();

    /* switch to eye render target and bind render textures */
    _framebuffer->bind();

    _framebuffer->attachTexture(Framebuffer::ColorAttachment(0), _textureSet->getActiveTexture(), 0)
                .attachTexture(Framebuffer::BufferAttachment::Depth, *_depth, 0)
    /* clear with the standard grey so that at least that will be visible in case the scene is not
     * correctly set up. */
                .clear(FramebufferClear::Color | FramebufferClear::Depth);

    /* render scene */
    SceneGraph::Camera3D::draw(group);

    /* Reasoning for the next two lines, taken from the Oculus SDK examples code:
     * Without this, [during the next frame, this method] would bind a framebuffer with an
     * invalid COLOR_ATTACHMENT0 because the texture ID associated with COLOR_ATTACHMENT0
     * had been unlocked by calling wglDXUnlockObjectsNV. */
    _framebuffer->detach(Framebuffer::ColorAttachment(0))
                 .detach(Framebuffer::BufferAttachment::Depth);

}

}}
