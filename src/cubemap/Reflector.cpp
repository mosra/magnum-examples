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
#include "Camera.h"
#include "Primitives/UVSphere.h"
#include "MeshTools/CompressIndices.h"
#include "MeshTools/Interleave.h"

#include "ReflectionShader.h"

namespace Magnum { namespace Examples {

Reflector::Reflector(CubeMapTexture* texture, Texture2D* tarnishTexture, Object* parent): Object(parent), texture(texture), tarnishTexture(tarnishTexture) {
    Buffer* buffer = sphere.addBuffer(true);
    Primitives::UVSphere sphereData(16, 32, Primitives::UVSphere::TextureCoords::Generate);
    MeshTools::interleave(&sphere, buffer, Buffer::Usage::StaticDraw, *sphereData.vertices(0), *sphereData.textureCoords2D(0));
    sphere.setVertexCount(sphereData.vertices(0)->size());
    sphere.bindAttribute<ReflectionShader::Vertex>(buffer);
    sphere.bindAttribute<ReflectionShader::TextureCoords>(buffer);
    MeshTools::compressIndices(&sphere, Buffer::Usage::StaticDraw, *sphereData.indices());
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
    shader()->setTarnishTextureUniform(tarnishTexture);
    sphere.draw();
}

ReflectionShader* Reflector::shader() {
    static ReflectionShader* _shader = 0;
    if(!_shader) _shader = new ReflectionShader;
    return _shader;
}

}}
