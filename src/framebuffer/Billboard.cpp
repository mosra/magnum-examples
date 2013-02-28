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

#include "Billboard.h"

#include <Buffer.h>
#include <Primitives/Square.h>
#include <SceneGraph/Camera2D.h>
#include <Trade/MeshData2D.h>

namespace Magnum { namespace Examples {

Billboard::Billboard(Trade::ImageData2D* image, Buffer* colorCorrectionBuffer, Object2D* parent, SceneGraph::DrawableGroup2D<>* group): Object2D(parent), SceneGraph::Drawable2D<>(this, group) {
    Trade::MeshData2D square = Primitives::Square::solid();
    buffer.setData(*square.positions(0), Buffer::Usage::StaticDraw);
    mesh.setPrimitive(square.primitive())
        ->setVertexCount(square.positions(0)->size())
        ->addVertexBuffer(&buffer, 0, ColorCorrectionShader::Position());

    texture.setWrapping(Texture2D::Wrapping::ClampToBorder)
        ->setMagnificationFilter(Texture2D::Filter::LinearInterpolation)
        ->setMinificationFilter(Texture2D::Filter::LinearInterpolation)
        ->setImage(0, Texture2D::InternalFormat::RGBA8, image);

    colorCorrectionTexture.setBuffer(BufferTexture::InternalFormat::R32F, colorCorrectionBuffer);

    scale(Vector2::yScale(Float(image->size()[1])/image->size()[0]));
}

void Billboard::draw(const Matrix3& transformationMatrix, SceneGraph::AbstractCamera2D<>* camera) {
    shader.setTransformationProjectionMatrix(camera->projectionMatrix()*transformationMatrix)
        ->use();
    texture.bind(ColorCorrectionShader::TextureLayer);
    colorCorrectionTexture.bind(ColorCorrectionShader::ColorCorrectionTextureLayer);

    mesh.draw();
}

}}
