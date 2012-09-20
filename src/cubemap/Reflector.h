#ifndef Magnum_Examples_Reflector_h
#define Magnum_Examples_Reflector_h
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

#include <SceneGraph/Object.h>
#include <IndexedMesh.h>

#include "ReflectionShader.h"

namespace Magnum {

template<size_t> class Texture;
typedef Texture<2> Texture2D;

class CubeMapTexture;

namespace Examples {

class Reflector: public SceneGraph::Object3D {
    public:
        Reflector(CubeMapTexture* texture, Texture2D* tarnishTexture, SceneGraph::Object3D* parent = nullptr);

        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D* camera);

    private:
        IndexedMesh sphere;
        ReflectionShader* shader();
        CubeMapTexture* texture;
        Texture2D* tarnishTexture;
};

}}

#endif
