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

#include <Containers/Array.h>
#include <PluginManager/Manager.h>
#include <Buffer.h>
#include <DefaultFramebuffer.h>
#include <Mesh.h>
#include <Texture.h>
#include <TextureFormat.h>
#ifdef CORRADE_TARGET_NACL
#include <Platform/NaClApplication.h>
#elif defined(CORRADE_TARGET_EMSCRIPTEN)
#include <Platform/Sdl2Application.h>
#else
#include <Platform/GlutApplication.h>
#endif
#include <Shaders/Flat.h>
#include <Trade/AbstractImporter.h>
#include <Trade/ImageData.h>

#include "configure.h"

#ifdef MAGNUM_BUILD_STATIC
/* Import shader resources in static build */
#include <Shaders/magnumShadersResourceImport.hpp>

/* Import plugins in static build */
static int importStaticPlugins() {
    CORRADE_PLUGIN_IMPORT(TgaImporter)
    return 0;
} CORRADE_AUTOMATIC_INITIALIZER(importStaticPlugins)
#endif

namespace Magnum { namespace Examples {

class TexturedTriangleExample: public Platform::Application {
    public:
        TexturedTriangleExample(const Arguments& arguments);

    private:
        void drawEvent() override;

        Buffer buffer;
        Mesh mesh;
        Shaders::Flat2D shader;
        Texture2D texture;
};

TexturedTriangleExample::TexturedTriangleExample(const Arguments& arguments): Platform::Application(arguments, Configuration()
    #ifndef CORRADE_TARGET_NACL
    .setTitle("Textured triangle example")
    #endif
), shader(Shaders::Flat2D::Flag::Textured) {
    constexpr static Vector2 data[] = {
        {-0.5f, -0.5f}, {0.0f, 0.0f}, /* Left vertex position and texture coordinate */
        { 0.5f, -0.5f}, {1.0f, 0.0f}, /* Right vertex position and texture coordinate */
        { 0.0f,  0.5f}, {0.5f, 1.0f}  /* Top vertex position and texture coordinate */
    };

    buffer.setData(data, BufferUsage::StaticDraw);
    mesh.setPrimitive(MeshPrimitive::Triangles)
        .setVertexCount(3)
        .addVertexBuffer(buffer, 0, Shaders::Flat2D::Position(), Shaders::Flat2D::TextureCoordinates());

    /* Load TGA importer plugin */
    PluginManager::Manager<Trade::AbstractImporter> manager(MAGNUM_PLUGINS_IMPORTER_DIR);
    if(!(manager.load("TgaImporter") & PluginManager::LoadState::Loaded)) {
        Error() << "Cannot load TgaImporter plugin";
        std::exit(1);
    }
    std::unique_ptr<Trade::AbstractImporter> importer = manager.instance("TgaImporter");
    CORRADE_INTERNAL_ASSERT(importer);

    /* Load the texture */
    Utility::Resource rs("data");
    if(!importer->openData(rs.getRaw("stone.tga"))) {
        Error() << "Cannot load texture";
        std::exit(2);
    }

    /* Set texture data and parameters */
    std::optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(image);
    texture.setWrapping(Sampler::Wrapping::ClampToEdge)
        .setMagnificationFilter(Sampler::Filter::Linear)
        .setMinificationFilter(Sampler::Filter::Linear);

    #ifndef MAGNUM_TARGET_GLES
    texture.setImage(0, TextureFormat::RGB8, *image);
    #else
    texture.setImage(0, TextureFormat::RGB, *image);
    #endif
}

void TexturedTriangleExample::drawEvent() {
    defaultFramebuffer.bind(FramebufferTarget::Draw);
    defaultFramebuffer.clear(FramebufferClear::Color);

    shader.setTransformationProjectionMatrix({})
        .setColor({1.0f, 0.7f, 0.7f})
        .use();
    texture.bind(Shaders::Flat2D::TextureLayer);
    mesh.draw();

    swapBuffers();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::TexturedTriangleExample)
