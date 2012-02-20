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

#include "Reflector.h"

#include "CubeMapTexture.h"
#include "MeshBuilder.h"
#include "Camera.h"
#include "Primitives/Icosphere.h"

#include "ReflectionShader.h"

namespace Magnum { namespace Examples {

Reflector::Reflector(CubeMapTexture* texture, Object* parent): Object(parent), texture(texture) {
    Buffer* buffer = sphere.addBuffer(false);
    Primitives::Icosphere<5>().build(&sphere, buffer);
    sphere.bindAttribute<Vector4>(buffer, ReflectionShader::Vertex);
}

void Reflector::draw(const Matrix4& transformationMatrix, Camera* camera) {
    texture->bind();
    shader()->use();
    shader()->setModelViewMatrixUniform(transformationMatrix);
    shader()->setProjectionMatrixUniform(camera->projectionMatrix());
    Matrix4 cameraMatrix = camera->absoluteTransformation();
    cameraMatrix.set(3, Vector4());
    shader()->setReflectivityUniform(2.0f);
    shader()->setDiffuseColorUniform(Vector3(0.3f, 0.3f, 0.3f));
    shader()->setCameraMatrixUniform(cameraMatrix);
    shader()->setTextureUniform(texture);
    sphere.draw();
}

ReflectionShader* Reflector::shader() {
    static ReflectionShader* _shader = 0;
    if(!_shader) _shader = new ReflectionShader;
    return _shader;
}

}}
