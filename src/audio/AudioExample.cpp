/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015
              Vladimír Vondruš <mosra@centrum.cz>
    Copyright © 2015 Jonathan Hale <squareys@googlemail.com>

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

#include <Corrade/PluginManager/PluginManager.h>

#include <Magnum/Audio/AbstractImporter.h>
#include <Magnum/Audio/Buffer.h>
#include <Magnum/Audio/Context.h>
#include <Magnum/Audio/Renderer.h>
#include <Magnum/Audio/Source.h>
#include <Magnum/Buffer.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Math/Range.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Renderer.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/Shaders/Flat.h>
#include <Magnum/Texture.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>

#include "configure.h"
#include "TexturedDrawable2D.h"
#include "Types.h"

namespace Magnum {namespace Examples {

class AudioExample: public Platform::Application {
    public:
        explicit AudioExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void updateSourceTranslation(const Vector2i& mousePos);

        Buffer _vertexBuffer;
        Mesh _mesh;
        Shaders::Flat2D _shader;

        /* Scene objects */
        Scene2D _scene;
        SceneGraph::Camera2D _camera;
        Object2D _sourceTopObject;
        Object2D _sourceFrontObject;

        /* Drawable groups, separated per viewport */
        SceneGraph::DrawableGroup2D _drawablesFront;
        SceneGraph::DrawableGroup2D _drawablesTop;

        /* Drawables */
        std::unique_ptr<TexturedDrawable2D> _listenerFront;
        std::unique_ptr<TexturedDrawable2D> _listenerTop;
        std::unique_ptr<TexturedDrawable2D> _sourceFront;
        std::unique_ptr<TexturedDrawable2D> _sourceTop;

        /* Textures */
        Texture2D _textureListenerTop;
        Texture2D _textureListenerFront;
        Texture2D _textureSource;

        /* Viewports ranges */
        Range2Di _leftViewport;
        Range2Di _rightViewport;

        /* Audio related members */
        Audio::Context _context;
        Audio::Buffer _testBuffer;
        Audio::Source _source;
        Corrade::Containers::Array<char> _bufferData;
};

AudioExample::AudioExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}.setTitle("Magnum Audio Example")},
    _shader(Shaders::Flat2D::Flag::Textured),
    _scene(),
    _camera(_scene),
    _sourceTopObject(&_scene),
    _sourceFrontObject(&_scene),
    _leftViewport(),
    _rightViewport(),
    _context(), /* Create the audio context. Without this, sound will not be initialized. */
    _testBuffer(),
    _source()
{
    /* Create a 2D plane for rendering all the sprites. */
    _vertexBuffer.setData(MeshTools::interleave(std::vector<Vector2>{
                 {1.0f, -1.0f},
                 {1.0f, 1.0f},
                 {-1.0f, -1.0f},
                 {-1.0f, 1.0f}
             }, std::vector<Vector2>{
                 {1.0f, 0.0f},
                 {1.0f, 1.0f},
                 {0.0f, 0.0f},
                 {0.0f, 1.0f}
             }), BufferUsage::StaticDraw);

    _mesh.setPrimitive(MeshPrimitive::TriangleStrip)
        .setCount(4)
        .addVertexBuffer(_vertexBuffer, 0, Shaders::Flat2D::Position{}, Shaders::Flat2D::TextureCoordinates{});

    /* Prepare viewports for top view (on the right) and front view (on the left) */
    const Vector2i halfViewport = defaultFramebuffer.viewport().size()*Vector2::xScale(0.5f);

    _leftViewport = Range2Di::fromSize({}, halfViewport);
    _rightViewport = Range2Di::fromSize({halfViewport.x(), 0}, halfViewport);

    /* Setup camera to scale down pixel measure to -1.0;1.0 on x and y according to aspect ratio for both viewports */
    _camera.setProjectionMatrix(Matrix3::projection({1.0f, 1.f/Vector2{halfViewport}.aspectRatio()}));

    /* Load TGA importer plugin */
    PluginManager::Manager<Trade::AbstractImporter> imageManager{MAGNUM_PLUGINS_IMPORTER_DIR};
    PluginManager::Manager<Audio::AbstractImporter> audioManager{MAGNUM_PLUGINS_AUDIOIMPORTER_DIR};

    std::unique_ptr<Trade::AbstractImporter> importer = imageManager.loadAndInstantiate("TgaImporter");
    if(!importer)
        std::exit(1);

    std::unique_ptr<Audio::AbstractImporter> wavImporter = audioManager.loadAndInstantiate("WavAudioImporter");
    if(!wavImporter)
        std::exit(1);

    /* Load the textures from compiled resources */
    const Utility::Resource rs{"audio-data"};
    if(!importer->openData(rs.getRaw("source.tga")))
        std::exit(2);

    std::optional<Trade::ImageData2D> image = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(image);
    _textureSource.setWrapping(Sampler::Wrapping::ClampToEdge)
        .setMagnificationFilter(Sampler::Filter::Linear)
        .setMinificationFilter(Sampler::Filter::Linear)
        .setStorage(1, TextureFormat::RGBA8, image->size())
        .setSubImage(0, {}, *image);

    if(!importer->openData(rs.getRaw("listener_top.tga")))
        std::exit(2);

    image = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(image);
    _textureListenerTop.setWrapping(Sampler::Wrapping::ClampToEdge)
        .setMagnificationFilter(Sampler::Filter::Linear)
        .setMinificationFilter(Sampler::Filter::Linear)
        .setStorage(1, TextureFormat::RGBA8, image->size())
        .setSubImage(0, {}, *image);

    if(!importer->openData(rs.getRaw("listener_front.tga")))
        std::exit(2);

   image = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(image);
    _textureListenerFront.setWrapping(Sampler::Wrapping::ClampToEdge)
        .setMagnificationFilter(Sampler::Filter::Linear)
        .setMinificationFilter(Sampler::Filter::Linear)
        .setStorage(1, TextureFormat::RGBA8, image->size())
        .setSubImage(0, {}, *image);

    /* Load wav file */
    if(!wavImporter->openData(rs.getRaw("test.wav")))
        std::exit(2);

    _bufferData = wavImporter->data(); /* Get the data from importer */
    /* Add the sample data to the buffer */
    _testBuffer.setData(wavImporter->format(), _bufferData, wavImporter->frequency());

    /* Make our sound source play the buffer again and again... */
    _source.setBuffer(&_testBuffer);
    _source.setLooping(true);

    _source.play();

    /* Create and add drawables to scene */
    _listenerFront.reset(new TexturedDrawable2D(_mesh, _shader, _textureListenerFront, _scene, _drawablesFront));
    _listenerTop.reset(new TexturedDrawable2D(_mesh, _shader, _textureListenerTop, _scene, _drawablesTop));
    _sourceFront.reset(new TexturedDrawable2D(_mesh, _shader, _textureSource, _sourceFrontObject, _drawablesFront));
    _sourceTop.reset(new TexturedDrawable2D(_mesh, _shader, _textureSource, _sourceTopObject, _drawablesTop));

    /* We do not need depth test, drawing order is fine. */
    Renderer::disable(Renderer::Feature::DepthTest);

    /* Enable blending for alpha channel in textures. */
    Renderer::enable(Renderer::Feature::Blending);
    Renderer::setBlendFunction(Renderer::BlendFunction::SourceAlpha, Renderer::BlendFunction::OneMinusSourceAlpha);

    /* setup image scaling since the plane is currently larger than the screen. */
    _sourceFront->scale({0.1, 0.1});
    _sourceTop->scale({0.1, 0.1});
    _listenerFront->scale({0.1, 0.1});
    _listenerTop->scale({0.1, 0.05});

    /* initial positioning of sound source */
    _sourceTopObject.translate({0.5, 0.3});
    _sourceFrontObject.translate({0.5, 0.3});

    /* set initial sound source position to match the visualization */
    _source.setPosition(Vector3i{5, 3, 3});
}

void AudioExample::updateSourceTranslation(const Vector2i& mousePos) {
    const Vector2 normalized = Vector2{mousePos}/Vector2{_leftViewport.size()} - Vector2{0.5f};
    const Vector2 position = normalized*Vector2::yScale(-1.0f)*_camera.projectionSize();

    Vector2 newTop = {position.x(), _sourceTopObject.transformation().translation().y()};
    Vector2 newFront = {position.x(), _sourceFrontObject.transformation().translation().y()};

    if (normalized.x() > 0.5f) {
        /* clicked on the right viewport/"top view" */
        newTop.y() = position.y();

        newFront.x() -= 1.0f;
        newTop.x() -= 1.0f;
    } else {
        /* clicked on the left viewport/"front view" */
        newFront.y() = position.y();
    }

    _sourceTopObject.setTransformation(Matrix3::translation(newTop));
    _sourceFrontObject.setTransformation(Matrix3::translation(newFront));

    /* Update sound source position to new input */
    _source.setPosition(Vector3i{Int(newTop.x()*10), Int(newFront.y()*10), Int(newTop.y()*10)});
}

void AudioExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color);

    defaultFramebuffer.setViewport(_leftViewport);
    _camera.draw(_drawablesFront);

    defaultFramebuffer.setViewport(_rightViewport);
    _camera.draw(_drawablesTop);

    swapBuffers();
}

void AudioExample::mousePressEvent(MouseEvent& event) {
    if(event.button() != MouseEvent::Button::Left) return;

    updateSourceTranslation(event.position());

    event.setAccepted();
    redraw();
}

void AudioExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    updateSourceTranslation(event.position());

    event.setAccepted();
    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::AudioExample)
