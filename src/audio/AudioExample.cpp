/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>
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
#include <Corrade/Containers/Reference.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/Utility/DebugStl.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/Audio/AbstractImporter.h>
#include <Magnum/Audio/Buffer.h>
#include <Magnum/Audio/Context.h>
#include <Magnum/Audio/Listener.h>
#include <Magnum/Audio/Playable.h>
#include <Magnum/Audio/PlayableGroup.h>
#include <Magnum/Audio/Source.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Time.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cone.h>
#include <Magnum/Primitives/Cylinder.h>
#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/DualQuaternionTransformation.h>
#include <Magnum/Trade/MeshData.h>

namespace Magnum { namespace Examples {

using namespace Math::Literals;

typedef SceneGraph::Scene<SceneGraph::DualQuaternionTransformation> Scene3D;
typedef SceneGraph::Object<SceneGraph::DualQuaternionTransformation> Object3D;

class AudioExample: public Platform::Application {
    public:
        explicit AudioExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void keyPressEvent(KeyEvent& event) override;

        Shaders::FlatGL3D _shader{NoCreate};

        /* Audio context and buffer */
        Audio::Context _context;
        Containers::Array<char> _bufferData;
        Audio::Buffer _buffer;

        /* Scene */
        Scene3D _scene;
        Object3D _sourceRig, _sourceObject, _cameraObject;
        SceneGraph::Camera3D _camera;
        SceneGraph::DrawableGroup3D _drawables;
        Audio::Listener3D _listener;
        Audio::PlayableGroup3D _playables;
};

AudioExample::AudioExample(const Arguments& arguments):
    Platform::Application{arguments, NoCreate},
    /* Create the audio context. Without this, sound will not be initialized:
       Needs to be done before Playables and Sources are initialized. */
    _context{
        Audio::Context::Configuration{}
            .setHrtf(Audio::Context::Configuration::Hrtf::Enabled),
        arguments.argc, arguments.argv
    },
    _sourceRig(&_scene),
    _sourceObject(&_sourceRig),
    _cameraObject(&_scene),
    _camera(_cameraObject),
    _listener{_scene}
{
    /* Try 8x MSAA, fall back to zero samples if not possible. Enable only 2x
       MSAA if we have enough DPI. */
    {
        const Vector2 dpiScaling = this->dpiScaling({});
        Configuration conf;
        conf.setTitle("Magnum Audio Example")
            .setSize(conf.size(), dpiScaling);
        GLConfiguration glConf;
        glConf.setSampleCount(dpiScaling.max() < 2.0f ? 8 : 2);
        if(!tryCreate(conf, glConf))
            create(conf, glConf.setSampleCount(0));
    }

    /* Load audio file from compiled resources */
    {
        /* Load importer plugin */
        PluginManager::Manager<Audio::AbstractImporter> manager;
        Containers::Pointer<Audio::AbstractImporter> importer = manager.loadAndInstantiate("StbVorbisAudioImporter");
        if(!importer)
            std::exit(1);

        const Utility::Resource rs{"audio-data"};
        if(!importer->openData(rs.getRaw("chimes.ogg")))
            std::exit(2);

        /* Get the data from importer and add them to the buffer. Be sure to
           keep a copy to avoid dangling reference. */
        _bufferData = importer->data();
        _buffer.setData(importer->format(), _bufferData, importer->frequency());
    }

    /* Set up rendering and a simple debug drawable. Takes ownership of passed
       mesh and draws it with given color. */
    _shader = Shaders::FlatGL3D{};
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    class WireframeDrawable: public SceneGraph::Drawable3D {
        public:
            explicit WireframeDrawable(Object3D& object, const Color4& color, Shaders::FlatGL3D& shader, GL::Mesh&& mesh, SceneGraph::DrawableGroup3D* drawables): SceneGraph::Drawable3D{object, drawables}, _color{color}, _shader(shader), _mesh{std::move(mesh)} {}

        private:
            void draw(const Matrix4& transformation, SceneGraph::Camera3D& camera) override {
                _shader
                    .setColor(_color)
                    .setTransformationProjectionMatrix(camera.projectionMatrix()*transformation)
                    .draw(_mesh);
            }

            Color4 _color;
            Shaders::FlatGL3D& _shader;
            GL::Mesh _mesh;
    };

    /* Set up the camera */
    _cameraObject
        .translate(Vector3::zAxis(6.5f))
        .rotateX(-45.0_degf);
    _camera
        .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(90.0_degf, 1.0f, 0.001f, 100.0f))
        .setViewport(GL::defaultFramebuffer.viewport().size());

    /* Audio playable. Make it directional and start playing on repeat.
       Visualize as a pink cone and direct it on the listener. */
    (new Audio::Playable3D{_sourceObject, -Vector3::yAxis(), &_playables})->source()
        .setInnerConeAngle(60.0_degf)
        .setOuterConeAngle(90.0_degf)
        .setBuffer(&_buffer)
        .setLooping(true)
        .play();
    new WireframeDrawable{_sourceObject, 0xff00ff_rgbf, _shader,
        MeshTools::compile(Primitives::coneWireframe(32, 0.5f)), &_drawables};
    _sourceObject.translate(Vector3::yAxis(5.0f));
    _sourceRig.rotateX(-60.0_degf).rotateY(-45.0_degf);

    /* Listener is at the center of the scene. Draw it as a cylinder. */
    new WireframeDrawable{_scene, 0xffffff_rgbf, _shader,
        MeshTools::compile(Primitives::cylinderWireframe(1, 32, 1.0f)), &_drawables};

    /* Print HRTF status information */
    Debug{} << "HRTF status:" << _context.hrtfStatus();
    Debug{} << "HRTF enabled:" << _context.isHrtfEnabled();
    Debug{} << "HRTF specifier:" << _context.hrtfSpecifierString();

    /* Loop at 60 Hz max */
    setSwapInterval(1);
    setMinimalLoopPeriod(16.0_msec);
}

void AudioExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    /* Update listener and sound source positions and draw them */
    _listener.update({_playables});
    _camera.draw(_drawables);

    swapBuffers();
    redraw();
}

void AudioExample::keyPressEvent(KeyEvent& event) {
    if(event.key() == Key::Down)
        _sourceRig.rotateXLocal(-5.0_degf);
    else if(event.key() == Key::Up)
        _sourceRig.rotateXLocal(5.0_degf);
    else if(event.key() == Key::Left)
        _sourceRig.rotateY(5.0_degf);
    else if(event.key() == Key::Right)
        _sourceRig.rotateY(-5.0_degf);
    else if(event.key() == Key::PageUp)
        _sourceObject.translate(Vector3::zAxis(-0.25f));
    else if(event.key() == Key::PageDown &&
            _sourceObject.transformation().translation().z() < 0.0f)
        _sourceObject.translate(Vector3::zAxis(0.25f));
    else return;

    _sourceRig.normalizeRotation();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::AudioExample)
