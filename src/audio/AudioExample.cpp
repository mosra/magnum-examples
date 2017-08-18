/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017 —
            Vladimír Vondruš <mosra@centrum.cz>
        2015, 2017 — Jonathan Hale <squareys@googlemail.com>

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

#include <Corrade/Containers/Array.h>
#include <Corrade/PluginManager/PluginManager.h>

#include <Magnum/Audio/AbstractImporter.h>
#include <Magnum/Audio/Buffer.h>
#include <Magnum/Audio/Context.h>
#include <Magnum/Audio/Listener.h>
#include <Magnum/Audio/Playable.h>
#include <Magnum/Audio/PlayableGroup.h>
#include <Magnum/Audio/Renderer.h>
#include <Magnum/Audio/Source.h>

#include <Magnum/Shapes/Shape.h>
#include <Magnum/Shapes/ShapeGroup.h>
#include <Magnum/Shapes/Sphere.h>
#include <Magnum/Shapes/Box.h>
#include <Magnum/DebugTools/ResourceManager.h>
#include <Magnum/DebugTools/ShapeRenderer.h>

#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Renderer.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/DualQuaternionTransformation.h>

#include "configure.h"

#ifdef MAGNUM_BUILD_STATIC
/* Import plugins in static build */
static int importStaticPlugins() {
    CORRADE_PLUGIN_IMPORT(WavAudioImporter)
    return 0;
} CORRADE_AUTOMATIC_INITIALIZER(importStaticPlugins)
#endif

namespace Magnum { namespace Examples {

using namespace Magnum::SceneGraph;
using namespace Magnum::Math::Literals;

typedef Scene<DualQuaternionTransformation> Scene3D;
typedef Object<DualQuaternionTransformation> Object3D;

class AudioExample: public Platform::Application {
    public:
        explicit AudioExample(const Arguments& arguments);

    private:
        void drawEvent() override;

        void keyPressEvent(KeyEvent& event) override;

        DebugTools::ResourceManager _manager;

        /* Audio related members */
        Audio::Context _context;
        Audio::Buffer _testBuffer;
        Audio::Source _source;
        Corrade::Containers::Array<char> _bufferData;

        /* Scene objects */
        Scene3D _scene;
        Object3D _sourceRig;
        Object3D _sourceObject;
        Object3D _cameraObject;
        SceneGraph::Camera3D _camera;
        SceneGraph::DrawableGroup3D _drawables;

        Audio::PlayableGroup3D _playables;
        Audio::Playable3D _playable;
        Audio::Listener3D _listener;

        Shapes::ShapeGroup3D _shapes;
};

AudioExample::AudioExample(const Arguments& arguments):
    Platform::Application{arguments, NoCreate},
    /* Create the audio context. Without this, sound will not be initialized:
       Needs to be done before Playables and Sources are initialized. */
    _context(Audio::Context::Configuration().setHrtf(Audio::Context::Configuration::Hrtf::Enabled)),
    _sourceRig(&_scene),
    _sourceObject(&_sourceRig),
    _cameraObject(&_scene),
    _camera(_cameraObject),
    _playable(_sourceObject, &_playables),
    _listener(_scene)
{
    /* Try 16x MSAA */
    Configuration conf;
    conf.setTitle("Magnum Audio Example")
        .setSampleCount(16);
    if(!tryCreateContext(conf))
        createContext(conf.setSampleCount(0));

    /* Setup the camera */
    _camera.setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
           .setProjectionMatrix(Matrix4::perspectiveProjection(Deg(90.0f), 1.0f, 0.001f, 100.0f))
           .setViewport(defaultFramebuffer.viewport().size());

    /* Load WAV importer plugin */
    PluginManager::Manager<Audio::AbstractImporter> audioManager{MAGNUM_PLUGINS_AUDIOIMPORTER_DIR};

    std::unique_ptr<Audio::AbstractImporter> wavImporter = audioManager.loadAndInstantiate("WavAudioImporter");
    if(!wavImporter)
        std::exit(1);

    /* Load wav file  from compiled resources */
    const Utility::Resource rs{"audio-data"};
    if(!wavImporter->openData(rs.getRaw("test.wav")))
        std::exit(2);

    _bufferData = wavImporter->data(); /* Get the data from importer */
    /* Add the sample data to the buffer */
    _testBuffer.setData(wavImporter->format(), _bufferData, wavImporter->frequency());

    /* Make our sound source play the buffer again and again... */
    _playable.source().setBuffer(&_testBuffer);
    _playable.source().setLooping(true);
    _playable.source().play();

    /* Initial offset of the sound source */
    _sourceObject.translate({0.0f, 0.0f, -5.0f});

    /* Camera placement */
    _cameraObject.rotateX(-45.0_degf).rotateY(45.0_degf);
    _cameraObject.translate({8.0f, 10.0f, 8.0f});

    /* Setup simple shape rendering for listener and source */
    _manager.set("pink", DebugTools::ShapeRendererOptions().setColor({1.0f, 0.0f, 1.0f}));
    Shapes::Shape<Shapes::Sphere3D>* sphere = new Shapes::Shape<Shapes::Sphere3D>(_sourceObject, {{}, 0.5f}, &_shapes);
    new DebugTools::ShapeRenderer3D(*sphere, ResourceKey("pink"), &_drawables);

    _manager.set("white", DebugTools::ShapeRendererOptions().setColor({1.0f, 1.0f, 1.0f}));
    Shapes::Shape<Shapes::Box3D>* box = new Shapes::Shape<Shapes::Box3D>(_scene, {Matrix4{}}, &_shapes);
    new DebugTools::ShapeRenderer3D(*box, ResourceKey("white"), &_drawables);

    /* Enable depth testing for correct overlap of shapes */
    Renderer::enable(Renderer::Feature::DepthTest);

    /* Print hrft status information */
    Debug() << "Hrtf status:" << _context.hrtfStatus();
    Debug() << "Hrtf enabled:" << _context.isHrtfEnabled();
    Debug() << "Hrtf specifier:" << _context.hrtfSpecifier();

    /* Loop at 60 Hz max */
    setSwapInterval(1);
    #ifndef CORRADE_TARGET_EMSCRIPTEN
    setMinimalLoopPeriod(16);
    #endif
}

void AudioExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color | FramebufferClear::Depth);

    _shapes.setClean();

    /* Update listener and sound source positions */
    _listener.update({_playables});

    _camera.draw(_drawables);

    swapBuffers();
    redraw();
}

void AudioExample::keyPressEvent(KeyEvent& event) {
    if(event.key() == KeyEvent::Key::Down) {
        _sourceRig.rotateXLocal(Deg(-5.0f));
        _sourceRig.normalizeRotation();
    } else if(event.key() == KeyEvent::Key::Up) {
        _sourceRig.rotateXLocal(Deg(5.0f));
        _sourceRig.normalizeRotation();
    } else if(event.key() == KeyEvent::Key::Left) {
        _sourceRig.rotateY(Deg(5.0f));
        _sourceRig.normalizeRotation();
    } else if(event.key() == KeyEvent::Key::Right) {
        _sourceRig.rotateY(Deg(-5.0f));
        _sourceRig.normalizeRotation();
    } else if(event.key() == KeyEvent::Key::PageUp)
        _sourceObject.translate({0.0f, 0.0f, -.25f});
    else if(event.key() == KeyEvent::Key::PageDown && _sourceObject.transformation().translation().z() < 0.0f)
        _sourceObject.translate({0.0f, 0.0f, .25f});
    else if(event.key() == KeyEvent::Key::Esc)
        this->exit();

    event.setAccepted();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::AudioExample)
