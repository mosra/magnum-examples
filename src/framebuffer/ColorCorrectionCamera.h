#ifndef Magnum_Examples_ColorCorrectionCamera_h
#define Magnum_Examples_ColorCorrectionCamera_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015
              Vladimír Vondruš <mosra@centrum.cz>

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

#include <Magnum/Framebuffer.h>
#include <Magnum/Renderbuffer.h>
#include <Magnum/SceneGraph/Camera2D.h>

namespace Magnum { namespace Examples {

class ColorCorrectionCamera: public SceneGraph::Camera2D {
    public:
        explicit ColorCorrectionCamera(SceneGraph::AbstractObject2D& object);

        void draw(SceneGraph::DrawableGroup2D& group) override;
        void setViewport(const Vector2i& size) override;

    private:
        enum { /* Color attachments */
            Original = 0,
            Grayscale = 1,
            Corrected = 2
        };

        Framebuffer framebuffer;
        Renderbuffer original,
            grayscale,
            corrected;
};

}}

#endif
