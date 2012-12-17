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
#include "ColorCorrectionShader.h"

#include <DefaultFramebuffer.h>

namespace Magnum { namespace Examples {

ColorCorrectionCamera::ColorCorrectionCamera(SceneGraph::AbstractObject2D<>* object): Camera2D(object), framebuffer(defaultFramebuffer.viewportPosition(), defaultFramebuffer.viewportSize()/2) {
    setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Clip);

    original.setStorage(Renderbuffer::InternalFormat::RGBA8, framebuffer.viewportSize());
    grayscale.setStorage(Renderbuffer::InternalFormat::RGBA8, framebuffer.viewportSize());
    corrected.setStorage(Renderbuffer::InternalFormat::RGBA8, framebuffer.viewportSize());

    framebuffer.attachRenderbuffer(Original, &original);
    framebuffer.attachRenderbuffer(Grayscale, &grayscale);
    framebuffer.attachRenderbuffer(Corrected, &corrected);

    framebuffer.mapForDraw({{ColorCorrectionShader::OriginalColorOutput, Original},
                            {ColorCorrectionShader::GrayscaleOutput, Grayscale},
                            {ColorCorrectionShader::ColorCorrectedOutput, Corrected}});
}

void ColorCorrectionCamera::draw(SceneGraph::DrawableGroup2D<>& group) {
    /* Draw original scene */
    framebuffer.clear(AbstractFramebuffer::Clear::Color);
    framebuffer.bind(AbstractFramebuffer::Target::Draw);
    Camera2D::draw(group);

    /* Original image at top left */
    framebuffer.mapForRead(Original);
    AbstractFramebuffer::blit(framebuffer, defaultFramebuffer,
        {0, 0}, framebuffer.viewportSize(),
        {0, defaultFramebuffer.viewportSize().y()/2},
        {defaultFramebuffer.viewportSize().x()/2, defaultFramebuffer.viewportSize().y()},
        AbstractFramebuffer::Blit::ColorBuffer, AbstractFramebuffer::BlitFilter::LinearInterpolation);

    /* Grayscale at top right */
    framebuffer.mapForRead(Grayscale);
    AbstractFramebuffer::blit(framebuffer, defaultFramebuffer,
        {0, 0}, framebuffer.viewportSize(),
        defaultFramebuffer.viewportSize()/2,
        defaultFramebuffer.viewportSize(),
        AbstractFramebuffer::Blit::ColorBuffer, AbstractFramebuffer::BlitFilter::LinearInterpolation);

    /* Color corrected at bottom */
    framebuffer.mapForRead(Corrected);
    AbstractFramebuffer::blit(framebuffer, defaultFramebuffer,
        {0, 0}, framebuffer.viewportSize(),
        {defaultFramebuffer.viewportSize().x()/4, 0},
        {defaultFramebuffer.viewportSize().x()*3/4, defaultFramebuffer.viewportSize().y()/2},
        AbstractFramebuffer::Blit::ColorBuffer, AbstractFramebuffer::BlitFilter::LinearInterpolation);
}

void ColorCorrectionCamera::setViewport(const Vector2i& size) {
    Camera2D::setViewport(size/2);

    /* Reset storage for renderbuffer */
    if(framebuffer.viewportSize() != size/2) {
        framebuffer.setViewport({}, size/2);
        original.setStorage(Renderbuffer::InternalFormat::RGBA8, framebuffer.viewportSize());
        grayscale.setStorage(Renderbuffer::InternalFormat::RGBA8, framebuffer.viewportSize());
        corrected.setStorage(Renderbuffer::InternalFormat::RGBA8, framebuffer.viewportSize());
    }
}

}}
