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

#include "Triangle.h"

#include "Buffer.h"

namespace Magnum { namespace Examples {

Triangle::Triangle(Magnum::Object* parent): Object(parent), mesh(Magnum::Mesh::Primitive::Triangles, 3) {
    static constexpr Magnum::Vector4 data[] = {
        Magnum::Vector4(-0.5f, -0.5f, 0.0f),    Magnum::Vector4(1.0f, 0.0f, 0.0f),  /* Left vertex, red color */
        Magnum::Vector4(0.5f, -0.5f, 0.0f),     Magnum::Vector4(0.0f, 1.0f, 0.0f),  /* Right vertex, green color */
        Magnum::Vector4(0.0f, 0.5f, 0.0f),      Magnum::Vector4(0.0f, 0.0f, 1.0f)   /* Top vertex, blue color */
    };

    Magnum::Buffer* buffer = mesh.addBuffer(Magnum::Mesh::BufferType::Interleaved);
    buffer->setData(sizeof(data), data, Magnum::Buffer::Usage::StaticDraw);

    mesh.bindAttribute<TriangleShader::Vertex>(buffer);
    mesh.bindAttribute<TriangleShader::Color>(buffer);
}

void Triangle::draw(const Magnum::Matrix4&, Magnum::Camera*) {
    shader.use();
    mesh.draw();
}

}}
