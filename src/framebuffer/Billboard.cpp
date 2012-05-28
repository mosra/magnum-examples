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

#include "Buffer.h"
#include "Camera.h"
#include "Primitives/Plane.h"

namespace Magnum { namespace Examples {

Billboard::Billboard(Trade::ImageData2D* image, Buffer* colorCorrectionBuffer, Object* parent): Object(parent), mesh(Mesh::Primitive::TriangleStrip, 4), colorCorrectionTexture(1) {
    Primitives::Plane plane;

    Buffer* buffer = mesh.addBuffer(Mesh::BufferType::NonInterleaved);
    buffer->setData(*plane.vertices(0), Buffer::Usage::StaticDraw);
    mesh.bindAttribute<ColorCorrectionShader::Vertex>(buffer);

    texture.setWrapping({AbstractTexture::Wrapping::ClampToBorder, AbstractTexture::Wrapping::ClampToBorder});
    texture.setMagnificationFilter(AbstractTexture::Filter::LinearInterpolation);
    texture.setMinificationFilter(AbstractTexture::Filter::LinearInterpolation);
    texture.setData(0, AbstractTexture::Format::RGBA, image);

    colorCorrectionTexture.setBuffer(BufferedTexture::Components::Red|BufferedTexture::ComponentType::Float, colorCorrectionBuffer);

    scale({1, GLfloat(image->dimensions()[1])/image->dimensions()[0], 1});
}

void Billboard::draw(const Matrix4& transformationMatrix, Camera* camera) {
    shader.use();
    shader.setMatrixUniform(camera->projectionMatrix()*transformationMatrix);
    shader.setTextureUniform(&texture);
    shader.setCorrectionTextureUniform(&colorCorrectionTexture);

    mesh.draw();
}

}}
