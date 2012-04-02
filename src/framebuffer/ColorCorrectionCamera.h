#ifndef Magnum_Examples_ColorCorrectionCamera_h
#define Magnum_Examples_ColorCorrectionCamera_h
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

#include "Camera.h"
#include "Framebuffer.h"
#include "Renderbuffer.h"

namespace Magnum { namespace Examples {

class ColorCorrectionCamera: public Camera {
    public:
        ColorCorrectionCamera(Object* parent = nullptr);

        void draw();
        void setViewport(const Math::Vector2<GLsizei>& size);

    private:
        enum { /* Color attachments */
            Original = 0,
            Grayscale = 1,
            Corrected = 2
        };

        bool initialized;
        Framebuffer framebuffer;
        Renderbuffer original,
            grayscale,
            corrected;
};

}}

#endif
