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

#include <BufferImage.h>
#include <Texture.h>
#include <AbstractShaderProgram.h>
#include <Mesh.h>
#include <SceneGraph/Camera3D.h>

#include "Types.h"

namespace Magnum { namespace Examples {

class MotionBlurCamera: public SceneGraph::Camera3D<> {
    public:
        static const GLint FrameCount = 7;

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
