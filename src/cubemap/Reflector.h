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
#include <ResourceManager.h>

namespace Magnum {

class AbstractShaderProgram;
class CubeMapTexture;
class IndexedMesh;
template<std::uint8_t> class Texture;
typedef Texture<2> Texture2D;

namespace Examples {

class ReflectorShader;

class Reflector: public SceneGraph::Object3D {
    public:
        Reflector(SceneGraph::Object3D* parent = nullptr);

        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D* camera);

    private:
        Resource<IndexedMesh> sphere;
        Resource<AbstractShaderProgram, ReflectorShader> shader;
        Resource<CubeMapTexture> texture;
        Resource<Texture2D> tarnishTexture;
};

}}

#endif
