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

#include "CubeMap.h"

#include <fstream>
#include <Utility/Resource.h>
#include <Math/Constants.h>
#include <CubeMapTexture.h>
#include <IndexedMesh.h>
#include <MeshTools/FlipNormals.h>
#include <MeshTools/Interleave.h>
#include <MeshTools/CompressIndices.h>
#include <Primitives/Cube.h>
#include <SceneGraph/Scene.h>
#include <SceneGraph/Camera.h>
#include <Trade/AbstractImporter.h>
#include <Trade/ImageData.h>

#include "CubeMapShader.h"
#include "CubeMapResourceManager.h"

using namespace std;
using namespace Corrade::Utility;

namespace Magnum { namespace Examples {

CubeMap::CubeMap(const string& prefix, SceneGraph::Object3D* parent): Object3D(parent) {
    CubeMapResourceManager* resourceManager = CubeMapResourceManager::instance();

    /* Cube mesh */
    if(!(cube = resourceManager->get<IndexedMesh>("cube"))) {
        IndexedMesh* mesh = new IndexedMesh;
        Buffer* buffer = mesh->addBuffer(Mesh::BufferType::NonInterleaved);

        Primitives::Cube cubeData;
        mesh->setPrimitive(cubeData.primitive())
            ->bindAttribute<CubeMapShader::Position>(buffer);
        MeshTools::flipFaceWinding(*cubeData.indices());
        MeshTools::compressIndices(mesh, Buffer::Usage::StaticDraw, *cubeData.indices());
        MeshTools::interleave(mesh, buffer, Buffer::Usage::StaticDraw, *cubeData.positions(0));

        resourceManager->set("cube", mesh, ResourceDataState::Final, ResourcePolicy::Resident);
    }

    /* Cube map texture */
    if(!(texture = resourceManager->get<CubeMapTexture>("texture"))) {
        CubeMapTexture* texture = new CubeMapTexture;

        Resource<Trade::AbstractImporter> importer = resourceManager->get<Trade::AbstractImporter>("tga-importer");
        importer->open(prefix + "+x.tga");
        texture->setData(CubeMapTexture::PositiveX, 0, AbstractTexture::Format::RGB, importer->image2D(0));
        importer->open(prefix + "-x.tga");
        texture->setData(CubeMapTexture::NegativeX, 0, AbstractTexture::Format::RGB, importer->image2D(0));
        importer->open(prefix + "+y.tga");
        texture->setData(CubeMapTexture::PositiveY, 0, AbstractTexture::Format::RGB, importer->image2D(0));
        importer->open(prefix + "-y.tga");
        texture->setData(CubeMapTexture::NegativeY, 0, AbstractTexture::Format::RGB, importer->image2D(0));
        importer->open(prefix + "+z.tga");
        texture->setData(CubeMapTexture::PositiveZ, 0, AbstractTexture::Format::RGB, importer->image2D(0));
        importer->open(prefix + "-z.tga");
        texture->setData(CubeMapTexture::NegativeZ, 0, AbstractTexture::Format::RGB, importer->image2D(0));

        texture->setWrapping(Math::Vector3<CubeMapTexture::Wrapping>(CubeMapTexture::Wrapping::ClampToEdge))
            ->setMagnificationFilter(CubeMapTexture::Filter::LinearInterpolation)
            ->setMinificationFilter(CubeMapTexture::Filter::LinearInterpolation, CubeMapTexture::Mipmap::LinearInterpolation)
            ->generateMipmap();

        resourceManager->set("texture", texture, ResourceDataState::Final, ResourcePolicy::Manual);
    }

    /* Shader */
    if(!(shader = resourceManager->get<AbstractShaderProgram, CubeMapShader>("shader")))
        resourceManager->set<AbstractShaderProgram>("shader", new CubeMapShader, ResourceDataState::Final, ResourcePolicy::Manual);
}

void CubeMap::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D* camera) {
    shader->use();

    shader->setTransformationProjectionMatrix(camera->projectionMatrix()*transformationMatrix);
    texture->bind(CubeMapShader::TextureLayer);

    cube->draw();
}

}}
