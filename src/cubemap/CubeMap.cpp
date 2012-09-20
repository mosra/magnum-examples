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
#include <MeshTools/CompressIndices.h>
#include <Primitives/Cube.h>
#include <SceneGraph/Scene.h>
#include <SceneGraph/Camera.h>
#include <Trade/AbstractImporter.h>
#include <Trade/ImageData.h>

#include "Reflector.h"

using namespace std;
using namespace Corrade::Utility;

namespace Magnum { namespace Examples {

CubeMap::CubeMap(Trade::AbstractImporter* importer, const string& prefix, SceneGraph::Object3D* parent): Object3D(parent) {
    Primitives::Cube cubeData;
    MeshTools::flipFaceWinding(*cubeData.indices());
    Buffer* buffer = cube.addBuffer(Mesh::BufferType::NonInterleaved);
    buffer->setData(*cubeData.positions(0), Buffer::Usage::StaticDraw);
    cube.setVertexCount(cubeData.positions(0)->size())
        ->bindAttribute<CubeMapShader::Position>(buffer);
    MeshTools::compressIndices(&cube, Buffer::Usage::StaticDraw, *cubeData.indices());

    scale(Vector3(20.0f));

    /* Textures */
    Trade::ImageData2D* image;
    importer->open(prefix + "+x.tga");
    image = importer->image2D(0);
    texture.setData(CubeMapTexture::PositiveX, 0, AbstractTexture::Format::RGB, image);

    importer->open(prefix + "-x.tga");
    image = importer->image2D(0);
    texture.setData(CubeMapTexture::NegativeX, 0, AbstractTexture::Format::RGB, image);

    importer->open(prefix + "+y.tga");
    image = importer->image2D(0);
    texture.setData(CubeMapTexture::PositiveY, 0, AbstractTexture::Format::RGB, image);

    importer->open(prefix + "-y.tga");
    image = importer->image2D(0);
    texture.setData(CubeMapTexture::NegativeY, 0, AbstractTexture::Format::RGB, image);

    importer->open(prefix + "+z.tga");
    image = importer->image2D(0);
    texture.setData(CubeMapTexture::PositiveZ, 0, AbstractTexture::Format::RGB, image);

    importer->open(prefix + "-z.tga");
    image = importer->image2D(0);
    texture.setData(CubeMapTexture::NegativeZ, 0, AbstractTexture::Format::RGB, image);

    texture.setWrapping(Math::Vector3<CubeMapTexture::Wrapping>(CubeMapTexture::Wrapping::ClampToEdge))
        ->setMagnificationFilter(CubeMapTexture::Filter::LinearInterpolation)
        ->setMinificationFilter(CubeMapTexture::Filter::LinearInterpolation, CubeMapTexture::Mipmap::LinearInterpolation)
        ->generateMipmap();

    /* Tarnish texture */
    Resource rs("data");
    std::istringstream in(rs.get("tarnish.tga"));
    importer->open(in);
    tarnishTexture.setData(0, AbstractTexture::Format::RGB, importer->image2D(0))
        ->setWrapping({CubeMapTexture::Wrapping::ClampToEdge, CubeMapTexture::Wrapping::ClampToEdge})
        ->setMagnificationFilter(CubeMapTexture::Filter::LinearInterpolation)
        ->setMinificationFilter(CubeMapTexture::Filter::LinearInterpolation, CubeMapTexture::Mipmap::LinearInterpolation)
        ->generateMipmap();

    (new Reflector(&texture, &tarnishTexture, scene()))
        ->scale(Vector3(0.5f))
        ->translate(Vector3::xAxis(-0.5f));

    (new Reflector(&texture, &tarnishTexture, scene()))
        ->scale(Vector3(0.3f))
        ->rotate(deg(37.0f), Vector3::xAxis())
        ->translate(Vector3::xAxis(0.3f));
}

void CubeMap::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D* camera) {
    shader.use();
    shader.setModelViewProjectionMatrix(camera->projectionMatrix()*transformationMatrix);
    texture.bind(CubeMapShader::TextureLayer);
    cube.draw();
}

}}
