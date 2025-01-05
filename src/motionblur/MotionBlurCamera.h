#ifndef Magnum_Examples_MotionBlur_MotionBlurCamera_h
#define Magnum_Examples_MotionBlur_MotionBlurCamera_h
/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>

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

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/BufferImage.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/SceneGraph/Camera.h>

#include "Types.h"

namespace Magnum { namespace Examples {

class MotionBlurCamera: public SceneGraph::Camera3D {
    public:
        enum: Int { FrameCount = 7 };

        explicit MotionBlurCamera(SceneGraph::AbstractObject3D& object);

        ~MotionBlurCamera();

        void setViewport(const Vector2i& size);
        void draw(SceneGraph::DrawableGroup3D& group);

    private:
        class MotionBlurShader: public GL::AbstractShaderProgram {
            public:
                typedef GL::Attribute<0, Vector2> Position;

                /* Frame texture layers are from 0 to FrameCount */

                MotionBlurShader();
        };

        class MotionBlurCanvas: public Object3D {
            public:
                MotionBlurCanvas(GL::Texture2D** frames, Object3D* parent = nullptr);

                void draw(std::size_t currentFrame);

            private:
                MotionBlurShader shader;
                GL::Buffer buffer;
                GL::Mesh mesh;
                GL::Texture2D** frames;
        };

        GL::BufferImage2D framebuffer;
        GL::Texture2D* frames[FrameCount];
        std::size_t currentFrame;
        MotionBlurCanvas canvas;
};

}}

#endif
