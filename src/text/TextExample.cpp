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

#include <iomanip>
#include <sstream>
#include <PluginManager/Manager.h>
#ifdef CORRADE_TARGET_NACL
#include <Platform/NaClApplication.h>
#elif defined(CORRADE_TARGET_EMSCRIPTEN)
#include <Platform/Sdl2Application.h>
#else
#include <Platform/GlutApplication.h>
#endif
#include <Math/Complex.h>
#include <DefaultFramebuffer.h>
#include <Mesh.h>
#include <Renderer.h>
#include <Shaders/DistanceFieldVector.h>
#include <Text/AbstractFont.h>
#include <Text/GlyphCache.h>
#include <Text/Renderer.h>
#include <Trade/AbstractImporter.h>

#include "configure.h"

#ifdef MAGNUM_BUILD_STATIC
/* Import shader resources in static build */
#include <Shaders/magnumShadersResourceImport.hpp>

/* Import plugins in static build */
static int importStaticPlugins() {
    CORRADE_PLUGIN_IMPORT(MagnumFont)
    CORRADE_PLUGIN_IMPORT(TgaImporter)
    return 0;
} CORRADE_AUTOMATIC_INITIALIZER(importStaticPlugins)
#endif

namespace Magnum { namespace Examples {

class TextExample: public Platform::Application {
    public:
        explicit TextExample(const Arguments& arguments);

    private:
        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;

        void updateText();

        PluginManager::Manager<Trade::AbstractImporter> importerManager;
        PluginManager::Manager<Text::AbstractFont> manager;
        std::unique_ptr<Text::AbstractFont> font;
        std::unique_ptr<Text::GlyphCache> cache;

        Mesh text;
        Buffer vertices, indices;
        std::unique_ptr<Text::Renderer2D> text2;
        Shaders::DistanceFieldVector2D shader;

        Matrix3 transformation;
        Matrix3 projection;
};

TextExample::TextExample(const Arguments& arguments): Platform::Application(arguments, Configuration().setTitle("Magnum Text Example")), importerManager(MAGNUM_PLUGINS_IMPORTER_DIR), manager(MAGNUM_PLUGINS_FONT_DIR), vertices(Buffer::Target::Array), indices(Buffer::Target::ElementArray) {
    /* Load FreeTypeFont plugin */
    if(!(manager.load("MagnumFont") & PluginManager::LoadState::Loaded)) {
        Error() << "Cannot open MagnumFont plugin";
        std::exit(1);
    }
    font = manager.instance("MagnumFont");
    CORRADE_INTERNAL_ASSERT(font);

    /* Open the font and fill glyph cache */
    Utility::Resource rs("fonts");
    if(!font->openData(std::vector<std::pair<std::string, Containers::ArrayReference<const unsigned char>>>{
        {"DejaVuSans.conf", rs.getRaw("DejaVuSans.conf")},
        {"DejaVuSans.tga", rs.getRaw("DejaVuSans.tga")}}, 0.0f))
    {
        Error() << "Cannot open font file";
        std::exit(1);
    }
    cache = font->createGlyphCache();
    CORRADE_INTERNAL_ASSERT(cache);

    std::tie(text, std::ignore) = Text::Renderer2D::render(*font, *cache, 0.1295f,
            "Hello, world!\n"
            "Ahoj, světe!\n"
            "Здравствуй, мир!\n"
            "γεια σου, τον κόσμο!\n"
            "Hej Världen!",
            vertices, indices, BufferUsage::StaticDraw, Text::Alignment::MiddleCenter);

    text2.reset(new Text::Renderer2D(*font, *cache, 0.035f, Text::Alignment::TopRight));
    text2->reserve(40, BufferUsage::DynamicDraw, BufferUsage::StaticDraw);

    Renderer::setFeature(Renderer::Feature::Blending, true);
    Renderer::setBlendFunction(Renderer::BlendFunction::One, Renderer::BlendFunction::OneMinusSourceAlpha);
    Renderer::setBlendEquation(Renderer::BlendEquation::Add, Renderer::BlendEquation::Add);

    transformation = Matrix3::rotation(Deg(-10.0f));
    projection = Matrix3::scaling(Vector2::yScale(Vector2(defaultFramebuffer.viewport().size()).aspectRatio()));
    updateText();
}

void TextExample::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});

    projection = Matrix3::scaling(Vector2::yScale(Vector2(size).aspectRatio()));
}

void TextExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color);

    cache->texture().bind(Shaders::DistanceFieldVector2D::VectorTextureLayer);

    shader.setTransformationProjectionMatrix(projection*transformation)
        .setColor(Color3::fromHSV(Deg(15.0f), 0.9f, 0.4f))
        .setOutlineColor(Color3::fromHSV(Deg(0.0f), 0.5f, 0.75f))
        .setOutlineRange(0.45f, 0.35f)
        .setSmoothness(0.025f/transformation.uniformScaling())
        .use();
    text.draw();

    shader.setTransformationProjectionMatrix(projection*
        Matrix3::translation(1.0f/projection.rotationScaling().diagonal()))
        .setColor(Color4(1.0f, 0.0f))
        .setOutlineRange(0.5f, 1.0f)
        .setSmoothness(0.075f)
        .use();
    text2->mesh().draw();

    swapBuffers();
}

void TextExample::mousePressEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::WheelUp)
        transformation = Matrix3::rotation(Deg(1.0f))*Matrix3::scaling(Vector2(1.1f))*transformation;
    else if(event.button() == MouseEvent::Button::WheelDown)
        transformation = Matrix3::rotation(Deg(-1.0f))*Matrix3::scaling(Vector2(1.0f/1.1f))*transformation;
    else return;

    updateText();

    event.setAccepted();
    redraw();
}

void TextExample::updateText() {
    std::ostringstream out;
    out << std::setprecision(2)
        << "Rotation: "
        #ifndef CORRADE_GCC44_COMPATIBILITY
        << Float(Deg(Complex::fromMatrix(transformation.rotation()).angle()))
        #else
        << Deg(Complex::fromMatrix(transformation.rotation()).angle()).toUnderlyingType()
        #endif
        << "°\nScale: "
        << transformation.uniformScaling();
    text2->render(out.str());
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::TextExample)
