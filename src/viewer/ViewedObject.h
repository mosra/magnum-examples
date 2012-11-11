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

#include "Mesh.h"
#include <SceneGraph/AbstractCamera.h>
#include <SceneGraph/Drawable.h>
#include "SceneGraph/Object.h"
#include "Shaders/PhongShader.h"
#include "Trade/PhongMaterialData.h"

#include "Types.h"

namespace Magnum { namespace Examples {

class ViewedObject: public Object3D, SceneGraph::Drawable3D<> {
    public:
        ViewedObject(Mesh* mesh, Trade::PhongMaterialData* material, Shaders::PhongShader* shader, Object3D* parent, SceneGraph::DrawableGroup3D<>* group): Object3D(parent), SceneGraph::Drawable3D<>(this, group), mesh(mesh), ambientColor(material->ambientColor()), diffuseColor(material->diffuseColor()), specularColor(material->specularColor()), shininess(material->shininess()), shader(shader) {}

        void draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D<>* camera) override {
            shader->setAmbientColor(ambientColor)
                ->setDiffuseColor(diffuseColor)
                ->setSpecularColor(specularColor)
                ->setShininess(shininess)
                ->setLightPosition({-3.0f, 10.0f, 10.0f})
                ->setTransformation(transformationMatrix)
                ->setProjection(camera->projectionMatrix())
                ->use();

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
