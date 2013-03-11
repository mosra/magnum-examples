/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013 Vladimír Vondruš <mosra@centrum.cz>

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

#include "ColorCorrectionCamera.h"

#include <DefaultFramebuffer.h>

#include "ColorCorrectionShader.h"

namespace Magnum { namespace Examples {

ColorCorrectionCamera::ColorCorrectionCamera(SceneGraph::AbstractObject2D<>* object): Camera2D(object), framebuffer(Rectanglei::fromSize(defaultFramebuffer.viewport().bottomLeft(), defaultFramebuffer.viewport().size()/2)) {
    setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Clip);

    original.setStorage(Renderbuffer::InternalFormat::RGBA8, framebuffer.viewport().size());
    grayscale.setStorage(Renderbuffer::InternalFormat::RGBA8, framebuffer.viewport().size());
    corrected.setStorage(Renderbuffer::InternalFormat::RGBA8, framebuffer.viewport().size());

    framebuffer.attachRenderbuffer(Framebuffer::ColorAttachment(Original), &original);
    framebuffer.attachRenderbuffer(Framebuffer::ColorAttachment(Grayscale), &grayscale);
    framebuffer.attachRenderbuffer(Framebuffer::ColorAttachment(Corrected), &corrected);

    framebuffer.mapForDraw({{ColorCorrectionShader::OriginalColorOutput, Framebuffer::ColorAttachment(Original)},
                            {ColorCorrectionShader::GrayscaleOutput, Framebuffer::ColorAttachment(Grayscale)},
                            {ColorCorrectionShader::ColorCorrectedOutput, Framebuffer::ColorAttachment(Corrected)}});
}

void ColorCorrectionCamera::draw(SceneGraph::DrawableGroup2D<>& group) {
    /* Draw original scene */
    framebuffer.clear(AbstractFramebuffer::Clear::Color);
    framebuffer.bind(AbstractFramebuffer::Target::Draw);
    Camera2D::draw(group);

    /* Original image at top left */
    framebuffer.mapForRead(Framebuffer::ColorAttachment(Original));
    AbstractFramebuffer::blit(framebuffer, defaultFramebuffer,
        framebuffer.viewport(),
        {{0, defaultFramebuffer.viewport().height()/2}, {defaultFramebuffer.viewport().width()/2, defaultFramebuffer.viewport().height()}},
        AbstractFramebuffer::Blit::ColorBuffer, AbstractFramebuffer::BlitFilter::Linear);

    /* Grayscale at top right */
    framebuffer.mapForRead(Framebuffer::ColorAttachment(Grayscale));
    AbstractFramebuffer::blit(framebuffer, defaultFramebuffer,
        framebuffer.viewport(),
        {defaultFramebuffer.viewport().size()/2, defaultFramebuffer.viewport().size()},
        AbstractFramebuffer::Blit::ColorBuffer, AbstractFramebuffer::BlitFilter::Linear);

    /* Color corrected at bottom */
    framebuffer.mapForRead(Framebuffer::ColorAttachment(Corrected));
    AbstractFramebuffer::blit(framebuffer, defaultFramebuffer,
        framebuffer.viewport(),
        {{defaultFramebuffer.viewport().width()/4, 0}, {defaultFramebuffer.viewport().width()*3/4, defaultFramebuffer.viewport().height()/2}},
        AbstractFramebuffer::Blit::ColorBuffer, AbstractFramebuffer::BlitFilter::Linear);
}

void ColorCorrectionCamera::setViewport(const Vector2i& size) {
    Camera2D::setViewport(size/2);

    /* Reset storage for renderbuffer */
    if(framebuffer.viewport().size() != size/2) {
        framebuffer.setViewport({{}, size/2});
        original.setStorage(Renderbuffer::InternalFormat::RGBA8, framebuffer.viewport().size());
        grayscale.setStorage(Renderbuffer::InternalFormat::RGBA8, framebuffer.viewport().size());
        corrected.setStorage(Renderbuffer::InternalFormat::RGBA8, framebuffer.viewport().size());
    }
}

}}
