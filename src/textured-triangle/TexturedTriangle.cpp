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

#include "TexturedTriangle.h"

#include <array>

#include <Buffer.h>
#include <MeshTools/Interleave.h>

namespace Magnum { namespace Examples {

TexturedTriangle::TexturedTriangle(Magnum::Trade::ImageData2D* image, Magnum::Object* parent): Object(parent), mesh(Magnum::Mesh::Primitive::Triangles, 3) {
    constexpr static std::array<Vector4, 3> vertices{{
        {-0.5f, -0.5f, 0.0f},
        {0.5f, -0.5f, 0.0f},
        {0.0f, 0.5f, 0.0f}
    }};
    constexpr static std::array<Vector2, 3> textureCoordinates{{
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.5f, 1.0f}
    }};

    Buffer* buffer = mesh.addBuffer(Mesh::BufferType::Interleaved);
    MeshTools::interleave(&mesh, buffer, Buffer::Usage::StaticDraw, vertices, textureCoordinates);

    mesh.bindAttribute<TexturedTriangleShader::Vertex>(buffer);
    mesh.bindAttribute<TexturedTriangleShader::TextureCoordinates>(buffer);

    texture.setData(0, Magnum::Texture2D::Format::RGB, image);
    texture.setMagnificationFilter(Magnum::Texture2D::Filter::LinearInterpolation);
    texture.setMinificationFilter(Magnum::Texture2D::Filter::LinearInterpolation);
    texture.setWrapping(Magnum::Math::Vector2<Magnum::Texture2D::Wrapping>(Magnum::Texture2D::Wrapping::ClampToEdge));
}

void TexturedTriangle::draw(const Magnum::Matrix4&, Magnum::Camera*) {
    shader.use();
    shader.setBaseColorUniform({1.0f, 0.7f, 0.7f});
    texture.bind(TexturedTriangleShader::TextureLayer);
    mesh.draw();
}

}}
