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

#include "Framebuffer.h"

namespace Magnum { namespace Examples {

ColorCorrectionCamera::ColorCorrectionCamera(Object* parent): Camera(parent), initialized(false) {
    setAspectRatioPolicy(AspectRatioPolicy::Clip);
}

void ColorCorrectionCamera::draw() {
    /* Map shader output to color attachments */
    framebuffer.mapForDraw({Original, Grayscale, Corrected});

    /* Draw original scene */
    Camera::draw();

    /* Draw to screen framebuffer */
    Framebuffer::mapDefaultForDraw({Framebuffer::DefaultDrawAttachment::BackLeft});

    /* Original image at top left */
    framebuffer.mapForRead(Original);
    Framebuffer::blit({0, 0}, viewport(),
                      {0, viewport().y()/2}, {viewport().x()/2, viewport().y()},
                      Framebuffer::Blit::Color, AbstractTexture::Filter::LinearInterpolation);

    /* Grayscale at top right */
    framebuffer.mapForRead(Grayscale);
    Framebuffer::blit({0, 0}, viewport(), viewport()/2, viewport(),
                      Framebuffer::Blit::Color, AbstractTexture::Filter::LinearInterpolation);

    /* Color corrected at bottom */
    framebuffer.mapForRead(Corrected);
    Framebuffer::blit({0, 0}, viewport(),
                      {viewport().x()/4, 0}, {viewport().x()*3/4, viewport().y()/2},
                      Framebuffer::Blit::Color, AbstractTexture::Filter::LinearInterpolation);
}

void ColorCorrectionCamera::setViewport(const Math::Vector2<GLsizei>& size) {
    Camera::setViewport(size);

    /* Reet storage for renderbuffer */
    original.setStorage(Renderbuffer::Format::RGB5Alpha1, size);
    grayscale.setStorage(Renderbuffer::Format::RGB5Alpha1, size);
    corrected.setStorage(Renderbuffer::Components::RGBA|Renderbuffer::ComponentType::NormalizedUnsignedByte, size);

    /* If not yet, attach renderbuffers to framebuffer */
    if(!initialized) {
        framebuffer.attachRenderbuffer(Framebuffer::Target::ReadDraw, Original, &original);
        framebuffer.attachRenderbuffer(Framebuffer::Target::ReadDraw, Grayscale, &grayscale);
        framebuffer.attachRenderbuffer(Framebuffer::Target::ReadDraw, Corrected, &corrected);
        initialized = true;
    }
}

}}
