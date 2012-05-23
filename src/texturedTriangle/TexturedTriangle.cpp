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

#include "Buffer.h"

using namespace std;

namespace Magnum { namespace Examples {

TexturedTriangle::TexturedTriangle(Trade::ImageData2D* image, Object* parent): Object(parent), mesh(Mesh::Primitive::Triangles, 3) {
    /* Vertices and texture coordinates, interleaved */
    GLfloat data[] = {
        -0.5f, -0.5f, 0.0f, 1.0f,   0.0f, 0.0f,  /* Lower left vertex */
        0.5f, -0.5f, 0.0f, 1.0f,    1.0f, 0.0f,  /* Lower right vertex */
        0.0f, 0.5f, 0.0f, 1.0f,     0.5f, 1.0f,  /* Top vertex */
    };

    /* Fill the mesh with data */
    Buffer* buffer = mesh.addBuffer(Mesh::BufferType::Interleaved);
    buffer->setData(sizeof(data), data, Buffer::Usage::StaticDraw);

    /* Bind attributes (first vertex data, then color data) */
    mesh.bindAttribute<TexturedIdentityShader::Vertex>(buffer);
    mesh.bindAttribute<TexturedIdentityShader::TextureCoordinates>(buffer);

    /* Texture */
    texture.setData(0, Texture2D::Format::RGB, image);
    texture.setMagnificationFilter(Texture2D::Filter::LinearInterpolation);
    texture.setMinificationFilter(Texture2D::Filter::LinearInterpolation);
    texture.setWrapping(Math::Vector2<AbstractTexture::Wrapping>(Texture2D::Wrapping::ClampToEdge, Texture2D::Wrapping::ClampToEdge));
}

void TexturedTriangle::draw(const Matrix4& transformationMatrix, Camera* camera) {
    texture.bind();
    shader.use();
    shader.setTextureUniform(&texture);
    mesh.draw();
}

}}
