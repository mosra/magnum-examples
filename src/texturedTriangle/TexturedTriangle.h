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

#include "Object.h"
#include "Mesh.h"
#include "Texture.h"
#include "TexturedIdentityShader.h"

namespace Magnum { namespace Examples {

class TexturedTriangle: public Object {
    public:
        TexturedTriangle(Trade::Image2D* image, Object* parent = 0);

        void draw(const Matrix4& transformationMatrix, Camera* camera);

    private:
        Mesh mesh;
        TexturedIdentityShader shader;
        Texture2D texture;
};

}}

#endif
