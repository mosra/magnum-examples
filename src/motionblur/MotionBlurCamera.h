#ifndef Magnum_Examples_MotionBlurCamera_h
#define Magnum_Examples_MotionBlurCamera_h
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

#include "Buffer.h"
#include "BufferedImage.h"
#include "Texture.h"
#include "AbstractShaderProgram.h"
#include "Mesh.h"

namespace Magnum { namespace Examples {

class MotionBlurCamera: public Camera {
    public:
        static const size_t FrameCount = 7;

        MotionBlurCamera(Object* parent = nullptr);

        ~MotionBlurCamera();

        void setViewport(const Math::Vector2<GLsizei>& size);
        void draw();

    private:
        class MotionBlurShader: public AbstractShaderProgram {
            public:
                enum Attribute {
                    Vertex = 0
                };

                MotionBlurShader();

                inline void setFrameUniform(size_t id, Texture2D* frame) {
                    setUniform(frameUniforms[id], frame);
                }

            private:
                GLint frameUniforms[MotionBlurCamera::FrameCount];
        };

        class MotionBlurCanvas: public Object {
            public:
                MotionBlurCanvas(Texture2D** frames, Object* parent = nullptr);

                void draw(size_t currentFrame);

            private:
                MotionBlurShader shader;
                Mesh mesh;
                Texture2D** frames;
        };

        BufferedImage2D framebuffer;
        Texture2D* frames[FrameCount];
        size_t currentFrame;
        MotionBlurCanvas canvas;
};

}}

#endif
