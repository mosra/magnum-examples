/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013 Vladimír Vondruš <mosra@centrum.cz>

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

#include <array>
#include <sstream>
#include <PluginManager/PluginManager.h>
#include <Buffer.h>
#include <DefaultFramebuffer.h>
#include <Mesh.h>
#include <Texture.h>
#include <MeshTools/Interleave.h>
#include <Platform/GlutApplication.h>
#include <Trade/AbstractImporter.h>
#include <Trade/ImageData.h>

#include "TexturedTriangleShader.h"
#include "configure.h"

namespace Magnum { namespace Examples {

class TexturedTriangleExample: public Platform::GlutApplication {
    public:
        TexturedTriangleExample(int& argc, char** argv);

    protected:
        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;

    private:
        Buffer buffer;
        Magnum::Mesh mesh;
        TexturedTriangleShader shader;
        Magnum::Texture2D texture;
};

TexturedTriangleExample::TexturedTriangleExample(int& argc, char** argv): GlutApplication(argc, argv, "Textured triangle example") {
    constexpr static std::array<Vector2, 3> positions{{
        {-0.5f, -0.5f},
        {0.5f, -0.5f},
        {0.0f, 0.5f}
    }};
    constexpr static std::array<Vector2, 3> textureCoordinates{{
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.5f, 1.0f}
    }};

    MeshTools::interleave(&mesh, &buffer, Buffer::Usage::StaticDraw,
        positions, textureCoordinates);
    mesh.setPrimitive(Mesh::Primitive::Triangles)
        ->setVertexCount(3)
        ->addInterleavedVertexBuffer(&buffer, 0, TexturedTriangleShader::Position(), TexturedTriangleShader::TextureCoordinates());

    /* Load TGA importer plugin */
    Corrade::PluginManager::PluginManager<Trade::AbstractImporter> manager(MAGNUM_PLUGINS_IMPORTER_DIR);
    Trade::AbstractImporter* importer;
    if(manager.load("TgaImporter") != Corrade::PluginManager::LoadState::Loaded || !(importer = manager.instance("TgaImporter"))) {
        Error() << "Cannot load TgaImporter plugin from" << manager.pluginDirectory();
        std::exit(1);
    }

    /* Load the texture */
    Corrade::Utility::Resource rs("data");
    std::istringstream in(rs.get("stone.tga"));
    if(!importer->open(in) || !importer->image2DCount()) {
        Error() << "Cannot load texture";
        std::exit(2);
    }

    /* Set texture data and parameters */
    texture.setWrapping(Texture2D::Wrapping::ClampToEdge)
        ->setMagnificationFilter(Texture2D::Filter::Linear)
        ->setMinificationFilter(Texture2D::Filter::Linear)
        ->setImage(0, Texture2D::InternalFormat::RGB8, importer->image2D(0));

    /* We don't need the importer plugin anymore */
    delete importer;
}

void TexturedTriangleExample::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});
}

void TexturedTriangleExample::drawEvent() {
    defaultFramebuffer.clear(DefaultFramebuffer::Clear::Color);

    shader.setBaseColor({1.0f, 0.7f, 0.7f})
        ->use();
    texture.bind(TexturedTriangleShader::TextureLayer);
    mesh.draw();

    swapBuffers();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::TexturedTriangleExample)
