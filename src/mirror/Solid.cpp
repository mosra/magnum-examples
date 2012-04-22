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

#include "Solid.h"

#include "Camera.h"
#include "Shaders/PhongShader.h"
#include "MeshTools/Interleave.h"
#include "MeshTools/CompressIndices.h"

#include "PointLight.h"

using namespace Magnum::Shaders;

namespace Magnum { namespace Examples {

Solid::Solid(const Trade::MeshData& data, Shaders::PhongShader* shader, PointLight* light, const Vector3& diffuseColor, Object* parent): Object(parent), shader(shader), light(light), diffuseColor(diffuseColor) {
    Buffer* buffer = mesh.addBuffer(true);
    MeshTools::interleave(&mesh, buffer, Buffer::Usage::StaticDraw, *data.vertices(0), *data.normals(0));
    MeshTools::compressIndices(&mesh, Buffer::Usage::StaticDraw, *data.indices());
    mesh.bindAttribute<PhongShader::Vertex>(buffer);
    mesh.bindAttribute<PhongShader::Normal>(buffer);
}

void Solid::draw(const Magnum::Matrix4& transformationMatrix, Magnum::Camera* camera) {
    shader->use();
    shader->setLightUniform(light->position(camera));
    shader->setLightAmbientColorUniform(light->ambientColor());
    shader->setLightDiffuseColorUniform(light->diffuseColor());
    shader->setLightSpecularColorUniform(light->specularColor());
    shader->setAmbientColorUniform({0.05f, 0.05f, 0.05f});
    shader->setDiffuseColorUniform(diffuseColor);
    shader->setSpecularColorUniform({0.9f, 0.9f, 0.87f});
    shader->setShininessUniform(128);
    shader->setTransformationMatrixUniform(transformationMatrix);
    shader->setProjectionMatrixUniform(camera->projectionMatrix());

    mesh.draw();
}

}}
