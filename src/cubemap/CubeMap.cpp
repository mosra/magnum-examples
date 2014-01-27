/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "CubeMap.h"

#include <fstream>
#include <Corrade/Utility/Resource.h>
#include <Magnum/CubeMapTexture.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/MeshTools/FlipNormals.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/Camera3D.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData3D.h>

#include "CubeMapShader.h"

namespace Magnum { namespace Examples {

CubeMap::CubeMap(const std::string& prefix, Object3D* parent, SceneGraph::DrawableGroup3D* group): Object3D(parent), SceneGraph::Drawable3D(*this, group) {
    CubeMapResourceManager& resourceManager = CubeMapResourceManager::instance();

    /* Cube mesh */
    if(!(cube = resourceManager.get<Mesh>("cube"))) {
        Mesh* mesh = new Mesh;
        Buffer* buffer = new Buffer;
        Buffer* indexBuffer = new Buffer;

        Trade::MeshData3D cubeData = Primitives::Cube::solid();
        MeshTools::flipFaceWinding(cubeData.indices());
        MeshTools::compressIndices(*mesh, *indexBuffer, BufferUsage::StaticDraw, cubeData.indices());
        MeshTools::interleave(*mesh, *buffer, BufferUsage::StaticDraw, cubeData.positions(0));
        mesh->setPrimitive(cubeData.primitive())
            .addVertexBuffer(*buffer, 0, CubeMapShader::Position());

        resourceManager.set("cube-buffer", buffer, ResourceDataState::Final, ResourcePolicy::Resident)
            .set("cube-index-buffer", indexBuffer, ResourceDataState::Final, ResourcePolicy::Resident)
            .set(cube.key(), mesh, ResourceDataState::Final, ResourcePolicy::Resident);
    }

    /* Cube map texture */
    if(!(texture = resourceManager.get<CubeMapTexture>("texture"))) {
        CubeMapTexture* cubeMap = new CubeMapTexture;

        cubeMap->setWrapping(Sampler::Wrapping::ClampToEdge)
            .setMagnificationFilter(Sampler::Filter::Linear)
            .setMinificationFilter(Sampler::Filter::Linear, Sampler::Mipmap::Linear);

        Resource<Trade::AbstractImporter> importer = resourceManager.get<Trade::AbstractImporter>("jpeg-importer");

        /* Configure texture storage using size of first image */
        importer->openFile(prefix + "+x.jpg");
        std::optional<Trade::ImageData2D> image = importer->image2D(0);
        CORRADE_INTERNAL_ASSERT(image);
        Vector2i size = image->size();
        cubeMap->setStorage(Math::log2(size.min())+1, TextureFormat::RGB8, size)
            .setSubImage(CubeMapTexture::Coordinate::PositiveX, 0, {}, *image);

        importer->openFile(prefix + "-x.jpg");
        CORRADE_INTERNAL_ASSERT_OUTPUT(image = importer->image2D(0));
        cubeMap->setSubImage(CubeMapTexture::Coordinate::NegativeX, 0, {}, *image);

        importer->openFile(prefix + "+y.jpg");
        CORRADE_INTERNAL_ASSERT_OUTPUT(image = importer->image2D(0));
        cubeMap->setSubImage(CubeMapTexture::Coordinate::PositiveY, 0, {}, *image);

        importer->openFile(prefix + "-y.jpg");
        CORRADE_INTERNAL_ASSERT_OUTPUT(image = importer->image2D(0));
        cubeMap->setSubImage(CubeMapTexture::Coordinate::NegativeY, 0, {}, *image);

        importer->openFile(prefix + "+z.jpg");
        CORRADE_INTERNAL_ASSERT_OUTPUT(image = importer->image2D(0));
        cubeMap->setSubImage(CubeMapTexture::Coordinate::PositiveZ, 0, {}, *image);

        importer->openFile(prefix + "-z.jpg");
        CORRADE_INTERNAL_ASSERT_OUTPUT(image = importer->image2D(0));
        cubeMap->setSubImage(CubeMapTexture::Coordinate::NegativeZ, 0, {}, *image);

        cubeMap->generateMipmap();

        resourceManager.set(texture.key(), cubeMap, ResourceDataState::Final, ResourcePolicy::Manual);
    }

    /* Shader */
    if(!(shader = resourceManager.get<AbstractShaderProgram, CubeMapShader>("shader")))
        resourceManager.set<AbstractShaderProgram>(shader.key(), new CubeMapShader, ResourceDataState::Final, ResourcePolicy::Manual);
}

void CubeMap::draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D& camera) {
    shader->setTransformationProjectionMatrix(camera.projectionMatrix()*transformationMatrix)
        .use();

    texture->bind(CubeMapShader::TextureLayer);

    cube->draw();
}

}}
