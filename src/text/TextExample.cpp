/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>

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

#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Format.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Complex.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Shaders/DistanceFieldVectorGL.h>
#include <Magnum/Text/AbstractFont.h>
#include <Magnum/Text/AbstractShaper.h>
#include <Magnum/Text/Alignment.h>
#include <Magnum/Text/DistanceFieldGlyphCacheGL.h>
#include <Magnum/Text/RendererGL.h>

namespace Magnum { namespace Examples {

using namespace Math::Literals;

class TextExample: public Platform::Application {
    public:
        explicit TextExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void scrollEvent(ScrollEvent& event) override;

        void updateText();

        PluginManager::Manager<Text::AbstractFont> _manager;
        Containers::Pointer<Text::AbstractFont> _font;

        /* The distance field cache rasterizes the font 4x larger */
        Text::DistanceFieldGlyphCacheGL _cache{Vector2i{2048}, Vector2i{512}, 22};
        Text::RendererGL _rotatingText{_cache};
        Text::RendererGL _dynamicText{_cache};

        Shaders::DistanceFieldVectorGL2D _shader;

        Matrix3 _transformationRotatingText;
};

TextExample::TextExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}
        .setTitle("Magnum Text Example")
        .setWindowFlags(Configuration::WindowFlag::Resizable)}
{
    /* Load a TrueTypeFont plugin and open the font */
    Utility::Resource rs("fonts");
    _font = _manager.loadAndInstantiate("TrueTypeFont");
    if(!_font || !_font->openData(rs.getRaw("SourceSansPro-Regular.ttf"), 180.0f))
        Fatal{} << "Cannot open font file";

    /* Glyphs we need to render everything */
    _font->fillGlyphCache(_cache,
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789:-+,.!°ěäПривітСΓειασουκόμ ");

    /* Text that rotates and scales using mouse wheel. Size relative to the
       window size (1/10 of it) -- if you resize the window, it gets bigger */
    _rotatingText
        .setAlignment(Text::Alignment::MiddleCenter)
        .render(*_font->createShaper(), 0.2f,
            "Hello, world!\n"
            "Ahoj, světe!\n"
            "Привіт Світ!\n"
            "Γεια σου κόσμε!\n"
            "Hej Världen!");
    _transformationRotatingText = Matrix3::rotation(-10.0_degf);

    /* Dynamically updated text that shows rotation/zoom of the other.
       Positioned ten pixels from the top right corner of the window, gets
       filled inside updateText() below. */
    _dynamicText
        .setAlignment(Text::Alignment::TopRightGlyphBounds)
        .setCursor(Vector2{windowSize()}*0.5f - Vector2{10.0f});

    /* Set up alpha blending so overlapping glyph quads don't cut into each
       other */
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(
        GL::Renderer::BlendFunction::One,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    updateText();
}

void TextExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    _shader.bindVectorTexture(_cache.texture());

    _shader
        .setTransformationProjectionMatrix(
            Matrix3::projection(Vector2::xScale(Vector2{windowSize()}.aspectRatio()))*
            _transformationRotatingText)
        .setColor(0x2f83cc_rgbf)
        .setOutlineColor(0xdcdcdc_rgbf)
        .setOutlineRange(0.45f, 0.35f)
        .setSmoothness(0.025f/_transformationRotatingText.uniformScaling())
        .draw(_rotatingText.mesh());

    _shader
        .setTransformationProjectionMatrix(Matrix3::projection(Vector2{windowSize()}))
        .setColor(0xffffff_rgbf)
        .setOutlineRange(0.5f, 1.0f)
        .setSmoothness(0.075f)
        .draw(_dynamicText.mesh());

    swapBuffers();
}

void TextExample::scrollEvent(ScrollEvent& event) {
    if(!event.offset().y())
        return;

    if(event.offset().y() > 0)
        _transformationRotatingText =
            Matrix3::rotation(1.0_degf)*
            Matrix3::scaling(Vector2{1.1f})*
            _transformationRotatingText;
    else
        _transformationRotatingText =
            Matrix3::rotation(-1.0_degf)*
            Matrix3::scaling(Vector2{1.0f/1.1f})*
            _transformationRotatingText;

    updateText();

    event.setAccepted();
    redraw();
}

void TextExample::updateText() {
    /* Replace the existing text, if any, with a new one */
    _dynamicText
        .clear()
        .render(*_font->createShaper(), 32.0f, Utility::format(
            "Rotation: {:.2}°\nScale: {:.2}",
            Float(Deg(Complex::fromMatrix(_transformationRotatingText.rotation())
                .angle())),
            _transformationRotatingText.uniformScaling()));
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::TextExample)
