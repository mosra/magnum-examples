/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
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

#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/ImageView.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#ifdef CORRADE_TARGET_ANDROID
#include <Magnum/Platform/AndroidApplication.h>
#elif defined(CORRADE_TARGET_EMSCRIPTEN)
#include <Magnum/Platform/EmscriptenApplication.h>
#else
#include <Magnum/Platform/Sdl2Application.h>
#endif
#include <Magnum/Shaders/Flat.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>

namespace Magnum { namespace Examples {

class TexturedTriangleExample: public Platform::Application {
    public:
        explicit TexturedTriangleExample(const Arguments& arguments);

    private:
        void viewportEvent(ViewportEvent& event) override;
        void drawEvent() override;

        GL::Mesh _mesh;
        Shaders::Flat2D _shader;
        GL::Texture2D _texture;
};

TexturedTriangleExample::TexturedTriangleExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}
        .setTitle("Magnum Textured Triangle Example")
        #ifndef CORRADE_TARGET_ANDROID
        .setWindowFlags(Configuration::WindowFlag::Resizable)
        #endif
    },
    _shader{Shaders::Flat2D::Flag::Textured}
{
    struct TriangleVertex {
        Vector2 position;
        Vector2 textureCoordinates;
    };
    const TriangleVertex data[]{
        {{-0.5f, -0.5f}, {0.0f, 0.0f}}, /* Left position and texture coordinate */
        {{ 0.5f, -0.5f}, {1.0f, 0.0f}}, /* Right position and texture coordinate */
        {{ 0.0f,  0.5f}, {0.5f, 1.0f}}  /* Top position and texture coordinate */
    };

    GL::Buffer buffer;
    buffer.setData(data);
    _mesh.setPrimitive(GL::MeshPrimitive::Triangles)
        .setCount(3)
        .addVertexBuffer(std::move(buffer), 0,
            Shaders::Flat2D::Position{},
            Shaders::Flat2D::TextureCoordinates{});

    /* Load TGA importer plugin */
    PluginManager::Manager<Trade::AbstractImporter> manager;
    Containers::Pointer<Trade::AbstractImporter> importer =
        manager.loadAndInstantiate("TgaImporter");
    if(!importer) std::exit(1);

    /* Load the texture */
    const Utility::Resource rs{"textured-triangle-data"};
    if(!importer->openData(rs.getRaw("stone.tga")))
        std::exit(2);

    /* Set texture data and parameters */
    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(image);
    _texture.setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        #if !(defined(MAGNUM_TARGET_WEBGL) && defined(MAGNUM_TARGET_GLES2))
        .setStorage(1, GL::TextureFormat::RGB8, image->size())
        #else
        .setStorage(1, GL::TextureFormat::RGB, image->size())
        #endif
        .setSubImage(0, {}, *image);
}

void TexturedTriangleExample::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});
}

void TexturedTriangleExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    using namespace Math::Literals;

    _shader.setTransformationProjectionMatrix({})
        .setColor(0xffb2b2_rgbf)
        .bindTexture(_texture);
    _mesh.draw(_shader);

    swapBuffers();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::TexturedTriangleExample)
