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

#include "Camera.h"
#include "Mesh.h"
#include "Shaders/PhongShader.h"

namespace Magnum { namespace Examples {

Icosphere::Icosphere(Mesh* mesh, Shaders::PhongShader* shader, const Vector3& color, Object* parent): Object(parent), mesh(mesh), shader(shader), color(color) {
    scale(0.1f);
}

void Icosphere::draw(const Matrix4& transformationMatrix, Camera* camera) {
    shader->use();
    shader->setDiffuseColorUniform(color);
    shader->setSpecularColorUniform({1.0f, 1.0f, 1.0f});
    shader->setShininessUniform(20);
    shader->setLightDiffuseColorUniform({1.0f, 1.0f, 0.75f});
    shader->setLightSpecularColorUniform({1.0f, 1.0f, 1.0f});
    shader->setLightUniform({3.0f, -3.0f, 3.0f});
    shader->setTransformationMatrixUniform(transformationMatrix);
    shader->setProjectionMatrixUniform(camera->projectionMatrix());

    mesh->draw();
}

}}
