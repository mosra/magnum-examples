#ifndef Magnum_Examples_Reflector_h
#define Magnum_Examples_Reflector_h
/*
    Copyright © 2010, 2011 Vladimír Vondruš <mosra@centrum.cz>

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
#include "IndexedMesh.h"
#include "ReflectionShader.h"

namespace Magnum {

class CubeMapTexture;

namespace Examples {

class Reflector: public Object {
    public:
        Reflector(CubeMapTexture* texture, Object* parent = 0);

        void draw(const Matrix4& transformationMatrix, const Matrix4& projectionMatrix);

    private:
        IndexedMesh sphere;
        ReflectionShader* shader();
        CubeMapTexture* texture;
};

}}

#endif
