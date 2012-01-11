#ifndef Magnum_Examples_CubeMap_h
#define Magnum_Examples_CubeMap_h
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
#include "IndexedMesh.h"

#include "CubeMapShader.h"
#include "CubeMapTexture.h"

namespace Magnum {

class AbstractImporter;

namespace Examples {

class Reflector;

class CubeMap: public Object {
    public:
        CubeMap(AbstractImporter* importer, const std::string& prefix, Object* parent);

        void draw(const Matrix4& transformationMatrix, const Matrix4& projectionMatrix);

    private:
        IndexedMesh cube;
        CubeMapShader shader;
        CubeMapTexture texture;
        Reflector* reflector;
};

}}

#endif
