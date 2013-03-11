#ifndef Magnum_Examples_MotionBlurCamera_h
#define Magnum_Examples_MotionBlurCamera_h
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

#include <BufferImage.h>
#include <Texture.h>
#include <AbstractShaderProgram.h>
#include <Mesh.h>
#include <SceneGraph/Camera3D.h>

#include "Types.h"

namespace Magnum { namespace Examples {

class MotionBlurCamera: public SceneGraph::Camera3D<> {
    public:
        static const Int FrameCount = 7;

        MotionBlurCamera(SceneGraph::AbstractObject3D<>* object);

        ~MotionBlurCamera();

        void setViewport(const Vector2i& size) override;
        void draw(SceneGraph::DrawableGroup3D<>& group) override;

    private:
        class MotionBlurShader: public AbstractShaderProgram {
            public:
                typedef Attribute<0, Vector2> Position;

                /* Frame texture layers are from 0 to FrameCount */

                MotionBlurShader();
        };

        class MotionBlurCanvas: public Object3D {
            public:
                MotionBlurCanvas(Texture2D** frames, Object3D* parent = nullptr);

                void draw(std::size_t currentFrame);

            private:
                MotionBlurShader shader;
                Buffer buffer;
                Mesh mesh;
                Texture2D** frames;
        };

        BufferImage2D framebuffer;
        Texture2D* frames[FrameCount];
        std::size_t currentFrame;
        MotionBlurCanvas canvas;
};

}}

#endif
