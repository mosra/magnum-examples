#ifndef Magnum_Examples_TexturedTriangle_h
#define Magnum_Examples_TexturedTriangle_h
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

#include <Object.h>
#include <Mesh.h>
#include <Texture.h>
#include <Trade/ImageData.h>

#include "TexturedTriangleShader.h"

namespace Magnum { namespace Examples {

class TexturedTriangle: public Magnum::Object {
    public:
        TexturedTriangle(Magnum::Trade::ImageData2D* image, Magnum::Object* parent = nullptr);

        void draw(const Magnum::Matrix4& transformationMatrix, Magnum::Camera* camera);

    private:
        Magnum::Mesh mesh;
        TexturedTriangleShader shader;
        Magnum::Texture2D texture;
};

}}

#endif
