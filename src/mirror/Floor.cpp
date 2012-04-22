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

#include "Floor.h"

#include "Camera.h"
#include "MeshTools/Interleave.h"
#include "Primitives/Plane.h"
#include "Shaders/PhongShader.h"

#include "PointLight.h"

using namespace Magnum::Shaders;

namespace Magnum { namespace Examples {

Floor::Floor(PhongShader* shader, PointLight* light, Object* parent): Object(parent), mesh(Mesh::Primitive::TriangleStrip), shader(shader), light(light) {
    Primitives::Plane plane;
    Buffer* buffer = mesh.addBuffer(true);
    MeshTools::interleave(&mesh, buffer, Buffer::Usage::StaticDraw, *plane.vertices(0), *plane.normals(0));
    mesh.bindAttribute<PhongShader::Vertex>(buffer);
    mesh.bindAttribute<PhongShader::Normal>(buffer);

    rotate(deg(-90.0f), Vector3::xAxis());
}

void Floor::draw(const Matrix4& transformationMatrix, Camera* camera) {
    shader->use();
    shader->setLightUniform(light->position(camera));
    shader->setLightAmbientColorUniform(light->ambientColor());
    shader->setLightDiffuseColorUniform(light->diffuseColor());
    shader->setLightSpecularColorUniform(light->specularColor());
    shader->setAmbientColorUniform({0.1f, 0.1f, 0.1f});
    shader->setDiffuseColorUniform({0.9f, 0.9f, 0.9f});
    shader->setSpecularColorUniform({1.0f, 1.0f, 1.0f});
    shader->setShininessUniform(20);
    shader->setTransformationMatrixUniform(transformationMatrix);
    shader->setProjectionMatrixUniform(camera->projectionMatrix());

    mesh.draw();
}


}}
