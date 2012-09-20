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

#include <CubeMapTexture.h>
#include <MeshTools/CompressIndices.h>
#include <MeshTools/Interleave.h>
#include <Primitives/UVSphere.h>
#include <SceneGraph/Camera.h>

#include "ReflectionShader.h"

namespace Magnum { namespace Examples {

Reflector::Reflector(CubeMapTexture* texture, Texture2D* tarnishTexture, SceneGraph::Object3D* parent): Object3D(parent), texture(texture), tarnishTexture(tarnishTexture) {
    Buffer* buffer = sphere.addBuffer(Mesh::BufferType::Interleaved);
    Primitives::UVSphere sphereData(16, 32, Primitives::UVSphere::TextureCoords::Generate);
    MeshTools::interleave(&sphere, buffer, Buffer::Usage::StaticDraw, *sphereData.positions(0), *sphereData.textureCoords2D(0));
    sphere.setVertexCount(sphereData.positions(0)->size())
        ->bindAttribute<ReflectionShader::Position>(buffer)
        ->bindAttribute<ReflectionShader::TextureCoords>(buffer);
    MeshTools::compressIndices(&sphere, Buffer::Usage::StaticDraw, *sphereData.indices());
}

void Reflector::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D* camera) {
    Matrix4 cameraMatrix = camera->absoluteTransformation();
    cameraMatrix[3] = Vector4();

    shader()->use();
    shader()->setModelViewMatrix(transformationMatrix)
        ->setProjectionMatrix(camera->projectionMatrix())
        ->setReflectivity(2.0f)
        ->setDiffuseColor(Color3<GLfloat>(0.3f))
        ->setCameraMatrix(cameraMatrix);

    texture->bind(ReflectionShader::TextureLayer);
    tarnishTexture->bind(ReflectionShader::TarnishTextureLayer);

    sphere.draw();
}

ReflectionShader* Reflector::shader() {
    static ReflectionShader* _shader = 0;
    if(!_shader) _shader = new ReflectionShader;
    return _shader;
}

}}
