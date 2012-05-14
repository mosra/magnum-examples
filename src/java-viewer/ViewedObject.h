#ifndef Magnum_Examples_ViewedObject_h
#define Magnum_Examples_ViewedObject_h
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

#include <Mesh.h>
#include <Camera.h>
#include <Trade/PhongMaterialData.h>

#include "PhongShader.h"

namespace Magnum { namespace Examples {

class ViewedObject: public Object {
    public:
        ViewedObject(Mesh* mesh, Trade::PhongMaterialData* material, PhongShader* shader, Object* parent = nullptr): Object(parent), mesh(mesh), ambientColor(material->ambientColor()), diffuseColor(material->diffuseColor()), specularColor(material->specularColor()), shininess(material->shininess()), shader(shader) {}

        void draw(const Matrix4& transformationMatrix, Camera* camera);

    private:
        Mesh* mesh;
        Vector3 ambientColor,
            diffuseColor,
            specularColor;
        GLfloat shininess;
        PhongShader* shader;
};

}}

#endif
