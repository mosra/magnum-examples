#ifndef Magnum_Examples_Icosphere_h
#define Magnum_Examples_Icosphere_h
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

namespace Magnum {

class Mesh;

namespace Shaders {
    class PhongShader;
}

namespace Examples {

class Icosphere: public Object {
    public:
        Icosphere(Mesh* mesh, Shaders::PhongShader* shader, const Vector3& color, Object* parent = nullptr);

        inline void advance(GLfloat angle) { rotate(angle, Vector3::zAxis()); }

        void draw(const Matrix4& transformationMatrix, Camera* camera);

    private:
        Mesh* mesh;
        Shaders::PhongShader* shader;
        Vector3 color;
};

}}

#endif
