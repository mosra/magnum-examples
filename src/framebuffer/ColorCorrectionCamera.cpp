/*
    Copyright © 2010, 2011, 2012 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Magnum.

    Magnum is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Magnum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
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
