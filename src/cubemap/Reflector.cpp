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
#include "ReflectorShader.h"

namespace Magnum { namespace Examples {

Reflector::Reflector(SceneGraph::Object3D* parent): Object3D(parent) {
    CubeMapResourceManager* resourceManager = CubeMapResourceManager::instance();

    /* Sphere mesh */
    if(!(sphere = resourceManager->get<IndexedMesh>("sphere"))) {
        IndexedMesh* mesh = new IndexedMesh;
        Buffer* buffer = new Buffer;
        Buffer* indexBuffer = new Buffer;

        Primitives::UVSphere sphereData(16, 32, Primitives::UVSphere::TextureCoords::Generate);
        MeshTools::interleave(mesh, buffer, Buffer::Usage::StaticDraw, *sphereData.positions(0), *sphereData.textureCoords2D(0));
        MeshTools::compressIndices(mesh, indexBuffer, Buffer::Usage::StaticDraw, *sphereData.indices());
        mesh->setPrimitive(sphereData.primitive())
            ->addInterleavedVertexBuffer(buffer, 0, ReflectorShader::Position(), ReflectorShader::TextureCoords());

        resourceManager->set("sphere-buffer", buffer, ResourceDataState::Final, ResourcePolicy::Resident);
        resourceManager->set("sphere-index-buffer", indexBuffer, ResourceDataState::Final, ResourcePolicy::Resident);
        resourceManager->set(sphere.key(), mesh, ResourceDataState::Final, ResourcePolicy::Resident);
    }

    /* Tarnish texture */
    if(!(tarnishTexture = resourceManager->get<Texture2D>("tarnish-texture"))) {
        Resource<Trade::AbstractImporter> importer = resourceManager->get<Trade::AbstractImporter>("tga-importer");
        Corrade::Utility::Resource rs("data");
        std::istringstream in(rs.get("tarnish.tga"));
        importer->open(in);

        Texture2D* texture = new Texture2D;
        texture->setWrapping({CubeMapTexture::Wrapping::ClampToEdge, CubeMapTexture::Wrapping::ClampToEdge})
            ->setMagnificationFilter(CubeMapTexture::Filter::LinearInterpolation)
            ->setMinificationFilter(CubeMapTexture::Filter::LinearInterpolation, CubeMapTexture::Mipmap::LinearInterpolation)
            ->setData(0, AbstractTexture::Format::RGB, importer->image2D(0))
            ->generateMipmap();

        resourceManager->set<Texture2D>(tarnishTexture.key(), texture, ResourceDataState::Final, ResourcePolicy::Resident);
    }

    /* Reflector shader */
    if(!(shader = resourceManager->get<AbstractShaderProgram, ReflectorShader>("reflector-shader")))
        resourceManager->set<AbstractShaderProgram>(shader.key(), new ReflectorShader, ResourceDataState::Final, ResourcePolicy::Resident);

    /* Texture (created in CubeMap class) */
    texture = resourceManager->get<CubeMapTexture>("texture");
}

void Reflector::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D* camera) {
    shader->setTransformationMatrix(transformationMatrix)
        ->setNormalMatrix(transformationMatrix.rotation())
        ->setProjectionMatrix(camera->projectionMatrix())
        ->setReflectivity(2.0f)
        ->setDiffuseColor(Color3<GLfloat>(0.3f))
        ->setCameraMatrix(camera->absoluteTransformation().rotation())
        ->use();

    texture->bind(ReflectorShader::TextureLayer);
    tarnishTexture->bind(ReflectorShader::TarnishTextureLayer);

    sphere->draw();
}

}}
