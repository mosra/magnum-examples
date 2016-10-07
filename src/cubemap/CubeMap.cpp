/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016 —
            Vladimír Vondruš <mosra@centrum.cz>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "CubeMap.h"

#include <Corrade/Utility/Resource.h>
#include <Magnum/Buffer.h>
#include <Magnum/CubeMapTexture.h>
#include <Magnum/Texture.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/MeshTools/FlipNormals.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData3D.h>

#include "CubeMapShader.h"

namespace Magnum { namespace Examples {

CubeMap::CubeMap(const std::string& prefix, Object3D* parent, SceneGraph::DrawableGroup3D* group): Object3D(parent), SceneGraph::Drawable3D(*this, group) {
    CubeMapResourceManager& resourceManager = CubeMapResourceManager::instance();

    /* Cube mesh */
    if(!(_cube = resourceManager.get<Mesh>("cube"))) {
        Trade::MeshData3D cubeData = Primitives::Cube::solid();
        MeshTools::flipFaceWinding(cubeData.indices());

        Buffer* buffer = new Buffer;
        buffer->setData(MeshTools::interleave(cubeData.positions(0)), BufferUsage::StaticDraw);

        Containers::Array<char> indexData;
        Mesh::IndexType indexType;
        UnsignedInt indexStart, indexEnd;
        std::tie(indexData, indexType, indexStart, indexEnd) = MeshTools::compressIndices(cubeData.indices());

        Buffer* indexBuffer = new Buffer;
        indexBuffer->setData(indexData, BufferUsage::StaticDraw);

        Mesh* mesh = new Mesh;
        mesh->setPrimitive(cubeData.primitive())
            .setCount(cubeData.indices().size())
            .addVertexBuffer(*buffer, 0, CubeMapShader::Position{})
            .setIndexBuffer(*indexBuffer, 0, indexType, indexStart, indexEnd);

        resourceManager.set("cube-buffer", buffer, ResourceDataState::Final, ResourcePolicy::Resident)
            .set("cube-index-buffer", indexBuffer, ResourceDataState::Final, ResourcePolicy::Resident)
            .set(_cube.key(), mesh, ResourceDataState::Final, ResourcePolicy::Resident);
    }

    /* Cube map texture */
    if(!(_texture = resourceManager.get<CubeMapTexture>("texture"))) {
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
            .setSubImage(CubeMapCoordinate::PositiveX, 0, {}, *image);

        importer->openFile(prefix + "-x.jpg");
        CORRADE_INTERNAL_ASSERT_OUTPUT(image = importer->image2D(0));
        cubeMap->setSubImage(CubeMapCoordinate::NegativeX, 0, {}, *image);

        importer->openFile(prefix + "+y.jpg");
        CORRADE_INTERNAL_ASSERT_OUTPUT(image = importer->image2D(0));
        cubeMap->setSubImage(CubeMapCoordinate::PositiveY, 0, {}, *image);

        importer->openFile(prefix + "-y.jpg");
        CORRADE_INTERNAL_ASSERT_OUTPUT(image = importer->image2D(0));
        cubeMap->setSubImage(CubeMapCoordinate::NegativeY, 0, {}, *image);

        importer->openFile(prefix + "+z.jpg");
        CORRADE_INTERNAL_ASSERT_OUTPUT(image = importer->image2D(0));
        cubeMap->setSubImage(CubeMapCoordinate::PositiveZ, 0, {}, *image);

        importer->openFile(prefix + "-z.jpg");
        CORRADE_INTERNAL_ASSERT_OUTPUT(image = importer->image2D(0));
        cubeMap->setSubImage(CubeMapCoordinate::NegativeZ, 0, {}, *image);

        cubeMap->generateMipmap();

        resourceManager.set(_texture.key(), cubeMap, ResourceDataState::Final, ResourcePolicy::Manual);
    }

    /* Shader */
    if(!(_shader = resourceManager.get<AbstractShaderProgram, CubeMapShader>("shader")))
        resourceManager.set<AbstractShaderProgram>(_shader.key(), new CubeMapShader, ResourceDataState::Final, ResourcePolicy::Manual);
}

void CubeMap::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader->setTransformationProjectionMatrix(camera.projectionMatrix()*transformationMatrix)
        .setTexture(*_texture);

    _cube->draw(*_shader);
}

}}
