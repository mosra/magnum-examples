#ifndef Magnum_Examples_Solid_h
#define Magnum_Examples_Solid_h
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

#include "IndexedMesh.h"
#include "Object.h"
#include "Trade/MeshData.h"

namespace Magnum {

namespace Shaders {
    class PhongShader;
}

namespace Examples {

class PointLight;

class Solid: public Object {
    public:
        Solid(const Trade::MeshData& data, Shaders::PhongShader* shader, PointLight* light, const Vector3& diffuseColor, Object* parent = nullptr);
        void draw(const Magnum::Matrix4& transformationMatrix, Magnum::Camera* camera);

    private:
        IndexedMesh mesh;
        Shaders::PhongShader* shader;
        PointLight* light;
        Vector3 diffuseColor;
};

}

}

#endif
