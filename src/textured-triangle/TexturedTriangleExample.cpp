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

#include "TexturedTriangleExample.h"

#include <array>
#include <PluginManager/PluginManager.h>
#include <Math/Point2D.h>
#include <Buffer.h>
#include <Framebuffer.h>
#include <MeshTools/Interleave.h>
#include <Trade/AbstractImporter.h>
#include <Trade/ImageData.h>

#include "configure.h"

namespace Magnum { namespace Examples {

TexturedTriangleExample::TexturedTriangleExample(int& argc, char** argv): GlutWindowContext(argc, argv, "Textured triangle example"), mesh(Magnum::Mesh::Primitive::Triangles, 3) {
    constexpr static std::array<Point2D, 3> positions{{
        {-0.5f, -0.5f},
        {0.5f, -0.5f},
        {0.0f, 0.5f}
    }};
    constexpr static std::array<Vector2, 3> textureCoordinates{{
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.5f, 1.0f}
    }};

    Buffer* buffer = mesh.addBuffer(Mesh::BufferType::Interleaved);
    Magnum::MeshTools::interleave(&mesh, buffer, Buffer::Usage::StaticDraw,
                                  positions, textureCoordinates);

    mesh.bindAttribute<TexturedTriangleShader::Position>(buffer);
    mesh.bindAttribute<TexturedTriangleShader::TextureCoordinates>(buffer);

    /* Load TGA importer plugin */
    Corrade::PluginManager::PluginManager<Trade::AbstractImporter> manager(MAGNUM_PLUGINS_IMPORTER_DIR);
    Magnum::Trade::AbstractImporter* importer;
    if(manager.load("TgaImporter") != Corrade::PluginManager::AbstractPluginManager::LoadOk || !(importer = manager.instance("TgaImporter"))) {
        Corrade::Utility::Error() << "Cannot load TgaImporter plugin from" << manager.pluginDirectory();
        exit(1);
    }

    /* Load the texture */
    Corrade::Utility::Resource rs("data");
    std::istringstream in(rs.get("stone.tga"));
    if(!importer->open(in) || !importer->image2DCount()) {
        Corrade::Utility::Error() << "Cannot load texture";
        exit(2);
    }

    /* Set texture data and parameters */
    texture.setData(0, Magnum::Texture2D::Format::RGB, importer->image2D(0))
        ->setWrapping(Magnum::Math::Vector2<Magnum::Texture2D::Wrapping>(Magnum::Texture2D::Wrapping::ClampToEdge))
        ->setMagnificationFilter(Magnum::Texture2D::Filter::LinearInterpolation)
        ->setMinificationFilter(Magnum::Texture2D::Filter::LinearInterpolation);

    /* We don't need the importer plugin anymore */
    delete importer;
}

void TexturedTriangleExample::viewportEvent(const Math::Vector2<GLsizei>& size) {
    Magnum::Framebuffer::setViewport({0, 0}, size);
}

void TexturedTriangleExample::drawEvent() {
    Magnum::Framebuffer::clear(Framebuffer::Clear::Color);

    shader.use();
    shader.setBaseColor({1.0f, 0.7f, 0.7f});
    texture.bind(TexturedTriangleShader::TextureLayer);
    mesh.draw();

    swapBuffers();
}

}}

int main(int argc, char** argv) {
    Magnum::Examples::TexturedTriangleExample e(argc, argv);
    return e.exec();
}

