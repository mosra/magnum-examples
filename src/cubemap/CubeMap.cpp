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
#include <MeshTools/FlipNormals.h>
#include <MeshTools/Interleave.h>
#include <MeshTools/CompressIndices.h>
#include <Primitives/Cube.h>
#include <SceneGraph/Scene.h>
#include <SceneGraph/Camera3D.h>
#include <Trade/AbstractImporter.h>
#include <Trade/ImageData.h>
#include <Trade/MeshData3D.h>

#include "CubeMapShader.h"

using namespace Corrade::Utility;

namespace Magnum { namespace Examples {

CubeMap::CubeMap(const std::string& prefix, Object3D* parent, SceneGraph::DrawableGroup3D<>* group): Object3D(parent), SceneGraph::Drawable3D<>(this, group) {
    CubeMapResourceManager* resourceManager = CubeMapResourceManager::instance();

    /* Cube mesh */
    if(!(cube = resourceManager->get<Mesh>("cube"))) {
        Mesh* mesh = new Mesh;
        Buffer* buffer = new Buffer;
        Buffer* indexBuffer = new Buffer;

        Trade::MeshData3D cubeData = Primitives::Cube::solid();
        MeshTools::flipFaceWinding(*cubeData.indices());
        MeshTools::compressIndices(mesh, indexBuffer, Buffer::Usage::StaticDraw, *cubeData.indices());
        MeshTools::interleave(mesh, buffer, Buffer::Usage::StaticDraw, *cubeData.positions(0));
        mesh->setPrimitive(cubeData.primitive())
            ->addVertexBuffer(buffer, 0, CubeMapShader::Position());

        resourceManager->set("cube-buffer", buffer, ResourceDataState::Final, ResourcePolicy::Resident);
        resourceManager->set("cube-index-buffer", indexBuffer, ResourceDataState::Final, ResourcePolicy::Resident);
        resourceManager->set(cube.key(), mesh, ResourceDataState::Final, ResourcePolicy::Resident);
    }

    /* Cube map texture */
    if(!(texture = resourceManager->get<CubeMapTexture>("texture"))) {
        CubeMapTexture* cubeMap = new CubeMapTexture;

        cubeMap->setWrapping(CubeMapTexture::Wrapping::ClampToEdge)
            ->setMagnificationFilter(CubeMapTexture::Filter::LinearInterpolation)
            ->setMinificationFilter(CubeMapTexture::Filter::LinearInterpolation, CubeMapTexture::Mipmap::LinearInterpolation);

        Resource<Trade::AbstractImporter> importer = resourceManager->get<Trade::AbstractImporter>("tga-importer");
        importer->open(prefix + "+x.tga");

        /* Configure texture storage using size of first image */
        Vector2i size = importer->image2D(0)->size();
        cubeMap->setStorage(Math::log2(size.min())+1, CubeMapTexture::InternalFormat::RGB8, size);

        cubeMap->setSubImage(CubeMapTexture::PositiveX, 0, {}, importer->image2D(0));
        importer->open(prefix + "-x.tga");
        cubeMap->setSubImage(CubeMapTexture::NegativeX, 0, {}, importer->image2D(0));
        importer->open(prefix + "+y.tga");
        cubeMap->setSubImage(CubeMapTexture::PositiveY, 0, {}, importer->image2D(0));
        importer->open(prefix + "-y.tga");
        cubeMap->setSubImage(CubeMapTexture::NegativeY, 0, {}, importer->image2D(0));
        importer->open(prefix + "+z.tga");
        cubeMap->setSubImage(CubeMapTexture::PositiveZ, 0, {}, importer->image2D(0));
        importer->open(prefix + "-z.tga");
        cubeMap->setSubImage(CubeMapTexture::NegativeZ, 0, {}, importer->image2D(0));

        cubeMap->generateMipmap();

        resourceManager->set(texture.key(), cubeMap, ResourceDataState::Final, ResourcePolicy::Manual);
    }

    /* Shader */
    if(!(shader = resourceManager->get<AbstractShaderProgram, CubeMapShader>("shader")))
        resourceManager->set<AbstractShaderProgram>(shader.key(), new CubeMapShader, ResourceDataState::Final, ResourcePolicy::Manual);
}

void CubeMap::draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D<>* camera) {
    shader->setTransformationProjectionMatrix(camera->projectionMatrix()*transformationMatrix)
        ->use();

    texture->bind(CubeMapShader::TextureLayer);

    cube->draw();
}

}}
