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

#include "Object.h"
#include "Mesh.h"
#include "Camera.h"
#include "Shaders/PhongShader.h"
#include "Trade/PhongMaterialData.h"

namespace Magnum { namespace Examples {

class ViewedObject: public Object {
    public:
        ViewedObject(Mesh* mesh, Trade::PhongMaterialData* material, Shaders::PhongShader* shader, Object* parent = nullptr): Object(parent), mesh(mesh), ambientColor(material->ambientColor()), diffuseColor(material->diffuseColor()), specularColor(material->specularColor()), shininess(material->shininess()), shader(shader) {}

        virtual void draw(const Matrix4& transformationMatrix, Camera* camera) {
            shader->use();
            shader->setAmbientColorUniform(ambientColor);
            shader->setDiffuseColorUniform(diffuseColor);
            shader->setSpecularColorUniform(specularColor);
            shader->setShininessUniform(shininess);

            shader->setLightAmbientColorUniform(Vector3());
            shader->setLightDiffuseColorUniform(Vector3(1.0f, 1.0f, 1.0f));
            shader->setLightSpecularColorUniform(Vector3(1.0f, 1.0f, 1.0f));
            shader->setLightUniform(Vector3(-3.0f, 10.0f, 10.0f));

            shader->setTransformationMatrixUniform(transformationMatrix);
            shader->setProjectionMatrixUniform(camera->projectionMatrix());

            mesh->draw();
        }

    private:
        Mesh* mesh;
        Vector3 ambientColor,
            diffuseColor,
            specularColor;
        GLfloat shininess;
        Shaders::PhongShader* shader;
};

}}

#endif
