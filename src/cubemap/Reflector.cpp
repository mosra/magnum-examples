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

#include <Utility/Resource.h>
#include <CubeMapTexture.h>
#include <IndexedMesh.h>
#include <MeshTools/CompressIndices.h>
#include <MeshTools/Interleave.h>
#include <Primitives/UVSphere.h>
#include <SceneGraph/Camera.h>
#include <Trade/AbstractImporter.h>
#include <Trade/ImageData.h>

#include "CubeMapResourceManager.h"
#include "ReflectionShader.h"

namespace Magnum { namespace Examples {

Reflector::Reflector(SceneGraph::Object3D* parent): Object3D(parent) {
    CubeMapResourceManager* resourceManager = CubeMapResourceManager::instance();

    /* Sphere mesh */
    if(!(sphere = resourceManager->get<IndexedMesh>("sphere"))) {
        IndexedMesh* mesh = new IndexedMesh;
        Buffer* buffer = mesh->addBuffer(Mesh::BufferType::Interleaved);
        Primitives::UVSphere sphereData(16, 32, Primitives::UVSphere::TextureCoords::Generate);
        MeshTools::interleave(mesh, buffer, Buffer::Usage::StaticDraw, *sphereData.positions(0), *sphereData.textureCoords2D(0));
        mesh->bindAttribute<ReflectionShader::Position>(buffer)
            ->bindAttribute<ReflectionShader::TextureCoords>(buffer);
        MeshTools::compressIndices(mesh, Buffer::Usage::StaticDraw, *sphereData.indices());

        resourceManager->set("sphere", mesh, ResourceDataState::Final, ResourcePolicy::Resident);
    }

    /* Reflector shader */
    if(!(shader = resourceManager->get<AbstractShaderProgram, ReflectionShader>("reflector-shader")))
        resourceManager->set<AbstractShaderProgram>("reflector-shader", new ReflectionShader, ResourceDataState::Final, ResourcePolicy::Resident);

    /* Texture (created in CubeMap class) */
    texture = resourceManager->get<CubeMapTexture>("texture");

    /* Tarnish texture */
    if(!(tarnishTexture = resourceManager->get<Texture2D>("tarnish-texture"))) {
        Resource<Trade::AbstractImporter> importer = resourceManager->get<Trade::AbstractImporter>("tga-importer");
        Corrade::Utility::Resource rs("data");
        std::istringstream in(rs.get("tarnish.tga"));
        importer->open(in);

        Texture2D* tarnishTexture = new Texture2D;
        tarnishTexture->setData(0, AbstractTexture::Format::RGB, importer->image2D(0))
            ->setWrapping({CubeMapTexture::Wrapping::ClampToEdge, CubeMapTexture::Wrapping::ClampToEdge})
            ->setMagnificationFilter(CubeMapTexture::Filter::LinearInterpolation)
            ->setMinificationFilter(CubeMapTexture::Filter::LinearInterpolation, CubeMapTexture::Mipmap::LinearInterpolation)
            ->generateMipmap();

        resourceManager->set<Texture2D>("tarnish-texture", tarnishTexture, ResourceDataState::Final, ResourcePolicy::Resident);
    }
}

void Reflector::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D* camera) {
    Matrix4 cameraMatrix = camera->absoluteTransformation();
    cameraMatrix[3] = Vector4();

    shader->use();
    shader->setModelViewMatrix(transformationMatrix)
        ->setProjectionMatrix(camera->projectionMatrix())
        ->setReflectivity(2.0f)
        ->setDiffuseColor(Color3<GLfloat>(0.3f))
        ->setCameraMatrix(cameraMatrix);

    texture->bind(ReflectionShader::TextureLayer);
    tarnishTexture->bind(ReflectionShader::TarnishTextureLayer);

    sphere->draw();
}

}}
