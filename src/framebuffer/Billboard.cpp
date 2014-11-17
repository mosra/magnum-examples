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

#include "Billboard.h"

#include <Magnum/Buffer.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/Primitives/Square.h>
#include <Magnum/SceneGraph/Camera2D.h>
#include <Magnum/Trade/MeshData2D.h>

namespace Magnum { namespace Examples {

Billboard::Billboard(const Trade::ImageData2D& image, Buffer* colorCorrectionBuffer, Object2D* parent, SceneGraph::DrawableGroup2D* group): Object2D(parent), SceneGraph::Drawable2D(*this, group) {
    Trade::MeshData2D square = Primitives::Square::solid();

    buffer.setData(square.positions(0), BufferUsage::StaticDraw);

    mesh.setPrimitive(square.primitive())
        .setCount(square.positions(0).size())
        .addVertexBuffer(buffer, 0, ColorCorrectionShader::Position{});

    texture.setWrapping(Sampler::Wrapping::ClampToBorder)
        .setMagnificationFilter(Sampler::Filter::Linear)
        .setMinificationFilter(Sampler::Filter::Linear)
        .setStorage(1, TextureFormat::RGBA8, image.size())
        .setSubImage(0, {}, image);

    colorCorrectionTexture.setBuffer(BufferTextureFormat::R32F, *colorCorrectionBuffer);

    scale(Vector2::yScale(Float(image.size()[1])/image.size()[0]));
}

void Billboard::draw(const Matrix3& transformationMatrix, SceneGraph::AbstractCamera2D& camera) {
    shader.setTransformationProjectionMatrix(camera.projectionMatrix()*transformationMatrix)
        .setTexture(texture)
        .setColorCorrectionTexture(colorCorrectionTexture);

    mesh.draw(shader);
}

}}
