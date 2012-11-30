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

#include "Framebuffer.h"
#include "Renderbuffer.h"
#include <SceneGraph/Camera2D.h>

namespace Magnum { namespace Examples {

class ColorCorrectionCamera: public SceneGraph::Camera2D<> {
    public:
        ColorCorrectionCamera(SceneGraph::AbstractObject2D<>* object);

        void draw(SceneGraph::DrawableGroup2D<>& group) override;
        void setViewport(const Vector2i& size) override;

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
