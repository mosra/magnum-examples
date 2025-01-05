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

#include <Corrade/Containers/ArrayView.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Arguments.h>
#include <Magnum/ImageView.h>
#include <Magnum/Timeline.h>
#include <Magnum/Animation/Track.h>
#include <Magnum/Animation/Player.h>
#include <Magnum/Math/Time.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Square.h>
#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData.h>

namespace Magnum { namespace Examples {

using namespace Math::Literals;

class AnimatedGifExample: public Platform::Application {
    public:
        explicit AnimatedGifExample(const Arguments& arguments);

    private:
        void keyPressEvent(KeyEvent& event) override;

        void drawEvent() override;
        void tickEvent() override;

        GL::Mesh _mesh{NoCreate};
        Shaders::FlatGL2D _shader{Shaders::FlatGL2D::Configuration{}
            .setFlags(Shaders::FlatGL2D::Flag::Textured|
                      Shaders::FlatGL2D::Flag::TextureTransformation)};
        GL::Texture2D _texture;
        Vector2i _gridSize, _imageSize;

        Animation::Track<Float, Int> _frames;
        Animation::Player<Float> _player;
        Timeline _timeline;
        Int _currentFrame = ~Int{};
};

AnimatedGifExample::AnimatedGifExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}
        .setTitle("[⏵] Magnum Animated Gif Example")
        .setSize({640, 480})}
{
    Utility::Arguments args;
    args.addArgument("file").setHelp("file", "GIF to load", "file.gif")
        .addOption("importer", "StbImageImporter")
            .setHelp("importer", "importer plugin to use")
        .addSkippedPrefix("magnum", "engine-specific options")
        .setGlobalHelp("Plays back an animated GIF")
        .parse(arguments.argc, arguments.argv);

    /* Load GIF importer plugin */
    PluginManager::Manager<Trade::AbstractImporter> manager;
    Containers::Pointer<Trade::AbstractImporter> importer =
        manager.loadAndInstantiate(args.value("importer"));
    if(!importer || !importer->openFile(args.value("file")))
        std::exit(1);

    /* Get the first image */
    Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
    if(!image) std::exit(2);

    /* Query total image count and allocate a 2D texture */
    const Int imageCount = importer->image2DCount();
    _gridSize.x() = Math::sqrt(imageCount);
    _gridSize.y() = (imageCount + _gridSize.x() - 1)/_gridSize.x();
    _imageSize = image->size();
    _texture.setWrapping(GL::SamplerWrapping::ClampToEdge)
        .setMagnificationFilter(GL::SamplerFilter::Linear)
        .setMinificationFilter(GL::SamplerFilter::Linear)
        .setStorage(1, GL::textureFormat(image->format()), _imageSize*_gridSize)
        .setSubImage(0, {}, *image);

    /* Upload remaining images */
    for(Int i = 1; i != imageCount; ++i) {
        if(!(image = importer->image2D(i))) std::exit(3);
        CORRADE_INTERNAL_ASSERT(image->size() == _imageSize);

        _texture.setSubImage(0,
            Vector2i{i/_gridSize.x(), i%_gridSize.x()}*_imageSize, *image);
    }

    /* Set up a "projector" quad, make its transformation follow the window
       and gif aspect ratio */
    _mesh = MeshTools::compile(
        Primitives::squareSolid(Primitives::SquareFlag::TextureCoordinates));
    _shader.setTransformationProjectionMatrix(
        Matrix3::projection({1.5f*Vector2{windowSize()}.aspectRatio(), 1.5f})*
        Matrix3::scaling(Vector2::yScale(1.0f/
            Vector2{_imageSize}.aspectRatio())));

    /* Set up the animation. Right now, until a better API for video playback
       is in place, StbImageImporter gives the delays as ints in its importer
       state (in case this is an animated GIF). Convert them to floats and set
       up a player to trigger a redraw. */
    if(importer->importerState()) {
        const auto frameDelays = Containers::arrayView(
            reinterpret_cast<const Int*>(importer->importerState()), imageCount);

        Containers::Array<std::pair<Float, Int>> frames{std::size_t(imageCount)};
        Float frameTime = 0.0f;
        for(std::size_t i = 0; i != frames.size(); ++i) {
            frames[i] = {frameTime, i};
            frameTime += frameDelays[i]/1000.0f;
        }
        _frames = Animation::Track<Float, Int>{std::move(frames), Math::select};
        _player.addWithCallbackOnChange(_frames,
            [](Float, const Int& frame, AnimatedGifExample& app) {
                app._shader.setTextureMatrix(
                    Matrix3::scaling(1.0f/Vector2{app._gridSize})*
                    Matrix3::translation(Vector2{Vector2i{frame/
                        app._gridSize.x(), frame%app._gridSize.x()}}));
                app.redraw();
            }, _currentFrame, *this);
    }

    _timeline.start();
    _player
        .setPlayCount(0)
        .play(_timeline.previousFrameTime());

    setMinimalLoopPeriod(16.0_msec);
}

void AnimatedGifExample::keyPressEvent(KeyEvent& event) {
    if(event.key() == Key::Space) {
        if(_player.state() == Animation::State::Playing) {
            _player.pause(_timeline.previousFrameTime());
            setWindowTitle("[⏸] Magnum Animated Gif Example");
        } else {
            _player.play(_timeline.previousFrameTime());
            setWindowTitle("[⏵] Magnum Animated Gif Example");
        }
    } else return;

    event.setAccepted();
    /* Player callback does a redraw(), if needed */
}

void AnimatedGifExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color);

    _shader.bindTexture(_texture)
        .draw(_mesh);

    swapBuffers();
    /* Player callback does a redraw(), if needed */
}

void AnimatedGifExample::tickEvent() {
    _player.advance(_timeline.previousFrameTime());
    _timeline.nextFrame();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::AnimatedGifExample)
