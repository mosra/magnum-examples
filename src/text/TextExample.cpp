/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025, 2026
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

#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/StringStl.h> /** @todo drop once file callbacks are STL-free */
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Format.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Complex.h>
#include <Magnum/Math/Matrix3.h>
#ifdef CORRADE_TARGET_EMSCRIPTEN
#include <Magnum/Platform/EmscriptenApplication.h>
#else
#include <Magnum/Platform/Sdl2Application.h>
#endif
#include <Magnum/Platform/Gesture.h>
#include <Magnum/Shaders/DistanceFieldVectorGL.h>
#include <Magnum/Trade/AbstractImporter.h>
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
        void viewportEvent(ViewportEvent& event) override;
        void drawEvent() override;
        void pointerPressEvent(PointerEvent& event) override;
        void pointerReleaseEvent(PointerEvent& event) override;
        void pointerMoveEvent(PointerMoveEvent& event) override;
        void scrollEvent(ScrollEvent& event) override;

        void updateText();

        PluginManager::Manager<Trade::AbstractImporter> _importerManager;
        PluginManager::Manager<Text::AbstractFont> _manager;
        Containers::Pointer<Text::AbstractFont> _font;

        Containers::Pointer<Text::GlyphCacheGL> _cache;
        Text::RendererGL _rotatingText{NoCreate};
        Text::RendererGL _dynamicText{NoCreate};

        Shaders::DistanceFieldVectorGL2D _shader;

        Matrix3 _transformationRotatingText;

        /* Pinch to zoom and rotate */
        Platform::TwoFingerGesture _gesture;
};

TextExample::TextExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}
        .setTitle("Magnum Text Example")
        .setWindowFlags(Configuration::WindowFlag::Resizable)}
{
    /* Make the font plugin manager aware of the importer manager for
       dependencies, load MagnumFont plugin */
    _manager.registerExternalManager(_importerManager);
    _font = _manager.loadAndInstantiate("MagnumFont");
    if(!_font) std::exit(1);

    /* Open the font and fill glyph cache */
    Utility::Resource rs("fonts");
    /* Load MagnumFont plugin */
    _font = _manager.loadAndInstantiate("MagnumFont");
    if(!_font) std::exit(1);
    _font->setFileCallback([](const std::string& filename,
        InputFileCallbackPolicy, void*) {
            Utility::Resource rs("fonts");
            return Containers::optional(rs.getRaw(filename));
        });

    /* Open the font and fill glyph cache */
    if(!_font->openFile("SourceSansPro.conf", 0.0f)) {
        Error() << "Cannot open font file";
        std::exit(1);
    }
    /* We know it's Text::GlyphCache, so cast it. Sigh, this is awful. */
    _cache = Containers::pointerCast<Text::GlyphCacheGL>(_font->createGlyphCache());
    CORRADE_INTERNAL_ASSERT(_cache);
    _rotatingText = Text::RendererGL{*_cache};
    _dynamicText = Text::RendererGL{*_cache};

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
       Positioned ten pixels from the top right corner of the window (with the
       window size applied via projection), gets filled inside updateText()
       below. */
    _dynamicText
        .setAlignment(Text::Alignment::TopRightGlyphBounds)
        .setCursor(Vector2{-10.0f});

    /* Set up alpha blending so overlapping glyph quads don't cut into each
       other */
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(
        GL::Renderer::BlendFunction::One,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    updateText();
}

void TextExample::viewportEvent(ViewportEvent& event) {
   GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});
}

void TextExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    _shader.bindVectorTexture(_cache->texture());

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
        .setTransformationProjectionMatrix(
            Matrix3::projection(Vector2{windowSize()})*
            Matrix3::translation(Vector2{windowSize()}*0.5f))
        .setColor(0xffffff_rgbf)
        .setOutlineRange(0.5f, 1.0f)
        .setSmoothness(0.075f)
        .draw(_dynamicText.mesh());

    swapBuffers();
}

void TextExample::pointerPressEvent(PointerEvent& event) {
    _gesture.pressEvent(event);
}

void TextExample::pointerReleaseEvent(PointerEvent& event) {
    _gesture.releaseEvent(event);
}

void TextExample::pointerMoveEvent(PointerMoveEvent& event) {
    _gesture.moveEvent(event);

    if(_gesture) {
        _transformationRotatingText =
            /* Event coordinates are Y down, invert the rotation to match the
               Y up used for transformations */
            Matrix3::from(_gesture.relativeRotation().inverted().toMatrix(), {})*
            Matrix3::scaling(Vector2{_gesture.relativeScaling()})*
            _transformationRotatingText;

        updateText();

        event.setAccepted();
        redraw();
        return;
    }
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
