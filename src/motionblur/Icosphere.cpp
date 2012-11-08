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

#include "Icosphere.h"

#include "Mesh.h"
#include "SceneGraph/Camera.h"
#include "Shaders/PhongShader.h"

namespace Magnum { namespace Examples {

Icosphere::Icosphere(Mesh* mesh, Shaders::PhongShader* shader, const Vector3& color, Object3D* parent, SceneGraph::DrawableGroup3D<>* group): Object3D(parent), SceneGraph::Drawable3D<>(this, group), mesh(mesh), shader(shader), color(color) {
    scale(Vector3(0.1f));
}

void Icosphere::draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D<>* camera) {
    shader->setDiffuseColor(color)
        ->setSpecularColor(Color3<GLfloat>(1.0f))
        ->setShininess(20)
        ->setLightPosition({3.0f, -3.0f, 3.0f})
        ->setTransformation(transformationMatrix)
        ->setProjection(camera->projectionMatrix())
        ->use();

    mesh->draw();
}

}}
