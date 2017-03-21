/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017 —
            Vladimír Vondruš <mosra@centrum.cz>
        2015, 2016 — Jonathan Hale <squareys@googlemail.com>

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

#include <memory>
#include <Corrade/Containers/Array.h>
#include <Corrade/Utility/utilities.h>
#include <Magnum/Buffer.h>
#include <Magnum/Context.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Framebuffer.h>
#include <Magnum/Magnum.h>
#include <Magnum/Mesh.h>
#include <Magnum/Renderer.h>
#include <Magnum/Renderbuffer.h>
#include <Magnum/Texture.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/Math/Quaternion.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/MeshData3D.h>
#include <Magnum/OvrIntegration/OvrIntegration.h>
#include <Magnum/OvrIntegration/Context.h>
#include <Magnum/OvrIntegration/Session.h>
#include <Magnum/OvrIntegration/Enums.h>

#include "Types.h"
#include "HmdCamera.h"
#include "CubeDrawable.h"

namespace Magnum { namespace Examples {

class OvrExample: public Platform::Application {
    public:
        explicit OvrExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void keyPressEvent(KeyEvent& event) override;

        OvrIntegration::Context _ovrContext;
        std::unique_ptr<OvrIntegration::Session> _session;

        std::unique_ptr<Buffer> _indexBuffer, _vertexBuffer;
        std::unique_ptr<Mesh> _mesh;
        std::unique_ptr<Shaders::Phong> _shader;

        Scene3D _scene;
        Object3D _cameraObject;
        Object3D _eyes[2];
        SceneGraph::DrawableGroup3D _drawables;
        std::unique_ptr<HmdCamera> _cameras[2];

        std::unique_ptr<Object3D> _cubes[4];
        std::unique_ptr<CubeDrawable> _cubeDrawables[4];

        std::unique_ptr<Framebuffer> _mirrorFramebuffer;
        Texture2D* _mirrorTexture;

        OvrIntegration::LayerEyeFov* _layer;
        OvrIntegration::PerformanceHudMode _curPerfHudMode;
        OvrIntegration::DebugHudStereoMode _curDebugHudStereoMode;

        bool _enableMirroring;
};

OvrExample::OvrExample(const Arguments& arguments) : Platform::Application(arguments, nullptr),
    _cameraObject(&_scene), _curPerfHudMode(OvrIntegration::PerformanceHudMode::Off),
    _curDebugHudStereoMode(OvrIntegration::DebugHudStereoMode::Off),
    _enableMirroring(true)
{
    /* Connect to an active Oculus session */
    _session = _ovrContext.createSession();

    if(!_session) {
        Error() << "No HMD connected.";
        exit();
        return;
    }

    /* Get the HMD display resolution */
    const Vector2i resolution = _session->resolution() / 2;

    /* Create a context with the HMD display resolution */
    Configuration conf;
    conf.setTitle("Magnum OculusVR Example")
        .setSize(resolution)
        .setSampleCount(16)
        .setSRGBCapable(true);
    if(!tryCreateContext(conf))
        createContext(conf.setSampleCount(0));

    /* The oculus sdk compositor does some "magic" to reduce latency. For
       that to work, VSync needs to be turned off. */
    if(!setSwapInterval(0))
        Error() << "Could not turn off VSync.";

    Renderer::enable(Renderer::Feature::DepthTest);
    Renderer::enable(Renderer::Feature::FramebufferSRGB);

    _session->configureRendering();

    /* Setup mirroring of oculus sdk compositor results to a texture which can
       later be blitted onto the default framebuffer */
    _mirrorTexture = &_session->createMirrorTexture(resolution);
    _mirrorFramebuffer.reset(new Framebuffer(Range2Di::fromSize({}, resolution)));
    _mirrorFramebuffer->attachTexture(Framebuffer::ColorAttachment(0), *_mirrorTexture, 0)
                      .mapForRead(Framebuffer::ColorAttachment(0));

    /* Setup cube mesh */
    const Trade::MeshData3D cube = Primitives::Cube::solid();

    _vertexBuffer.reset(new Buffer());
    _vertexBuffer->setData(
                MeshTools::interleave(cube.positions(0), cube.normals(0)),
                BufferUsage::StaticDraw);

    Containers::Array<char> indexData;
    Mesh::IndexType indexType;
    UnsignedInt indexStart, indexEnd;
    std::tie(indexData, indexType, indexStart, indexEnd) =
            MeshTools::compressIndices(cube.indices());

    _indexBuffer.reset(new Buffer());
    _indexBuffer->setData(indexData, BufferUsage::StaticDraw);

    _mesh.reset(new Mesh());
    _mesh->setPrimitive(cube.primitive()).setCount(cube.indices().size()).addVertexBuffer(
                *_vertexBuffer, 0, Shaders::Phong::Position {},
                Shaders::Phong::Normal {}).setIndexBuffer(*_indexBuffer, 0, indexType,
                                                          indexStart, indexEnd);

    /* Setup shader */
    _shader.reset(new Shaders::Phong());

    /* Setup scene */
    _cubes[0] = std::unique_ptr<Object3D>(new Object3D(&_scene));
    _cubes[0]->rotateY(Deg(45.0f));
    _cubes[0]->translate({0.0f, 0.0f, -3.0f});
    _cubeDrawables[0] = std::unique_ptr<CubeDrawable>(new CubeDrawable(*_mesh, *_shader, {1.0f, 1.0f, 0.0f}, _cubes[0].get(), &_drawables));

    _cubes[1] = std::unique_ptr<Object3D>(new Object3D(&_scene));
    _cubes[1]->rotateY(Deg(45.0f));
    _cubes[1]->translate({5.0f, 0.0f, 0.0f});
    _cubeDrawables[1] = std::unique_ptr<CubeDrawable>(new CubeDrawable(*_mesh, *_shader, {1.0f, 0.0f, 0.0f}, _cubes[1].get(), &_drawables));

    _cubes[2] = std::unique_ptr<Object3D>(new Object3D(&_scene));
    _cubes[2]->rotateY(Deg(45.0f));
    _cubes[2]->translate({-10.0f, 0.0f, 0.0f});
    _cubeDrawables[2] = std::unique_ptr<CubeDrawable>(new CubeDrawable(*_mesh, *_shader, {0.0f, 0.0f, 1.0f}, _cubes[2].get(), &_drawables));

    _cubes[3] = std::unique_ptr<Object3D>(new Object3D(&_scene));
    _cubes[3]->rotateY(Deg(45.0f));
    _cubes[3]->translate({0.0f, 0.0f, 7.0f});
    _cubeDrawables[3] = std::unique_ptr<CubeDrawable>(new CubeDrawable(*_mesh, *_shader, {0.0f, 1.0f, 1.0f}, _cubes[3].get(), &_drawables));

    /* Setup compositor layers */
    _layer = &_ovrContext.compositor().addLayerEyeFov();
    _layer->setFov(*_session.get());
    _layer->setHighQuality(true);

    /* Setup cameras */
    _eyes[0].setParent(&_cameraObject);
    _eyes[1].setParent(&_cameraObject);

    for(int eye = 0; eye < 2; ++eye) {
        /* Projection matrix is set in the camera, since it requires some
           HMD-specific FoV etc */
        _cameras[eye].reset(new HmdCamera(*_session, eye, _eyes[eye]));

        _layer->setColorTexture(eye, _cameras[eye]->textureSwapChain());
        _layer->setViewport(eye, {{}, _cameras[eye]->viewport()});
    }
}

void OvrExample::drawEvent() {
    /* Get orientation and position of the hmd. */
    const std::array<DualQuaternion, 2> poses = _session->pollEyePoses().eyePoses();

    /* Draw the scene for both cameras */
    for(int eye: {0, 1}) {
        /* set the transformation according to rift trackers */
        _eyes[eye].setTransformation(poses[eye].toMatrix());
        /* render each eye. */
        _cameras[eye]->draw(_drawables);
    }

    /* Set the layers eye poses to the poses chached in the _hmd. */
    _layer->setRenderPoses(*_session.get());

    /* Let the libOVR sdk compositor do its magic! */
    _ovrContext.compositor().submitFrame(*_session.get());

    if(_enableMirroring) {
        /* Blit mirror texture to default framebuffer */
        const Vector2i size = _mirrorTexture->imageSize(0);
        Framebuffer::blit(*_mirrorFramebuffer,
                          defaultFramebuffer,
                          {{0, size.y()}, {size.x(), 0}},
                          {{}, size},
                          FramebufferBlit::Color, FramebufferBlitFilter::Nearest);

        swapBuffers();
    }

    if(_session->isDebugHmd()) {
        /* Provide some rotation, but only without real devices to avoid VR
           sickness ;) */
        _cameraObject.rotateY(Deg(0.1f));
    }

    redraw();
}

void OvrExample::keyPressEvent(KeyEvent& event) {
    /* Toggle through the performance hud modes */
    if(event.key() == KeyEvent::Key::F11) {
        switch(_curPerfHudMode) {
            case OvrIntegration::PerformanceHudMode::Off:
                _curPerfHudMode = OvrIntegration::PerformanceHudMode::LatencyTiming;
                break;
            case OvrIntegration::PerformanceHudMode::LatencyTiming:
                _curPerfHudMode = OvrIntegration::PerformanceHudMode::AppRenderTiming;
                break;
            case OvrIntegration::PerformanceHudMode::AppRenderTiming:
                _curPerfHudMode = OvrIntegration::PerformanceHudMode::CompRenderTiming;
                break;
            case OvrIntegration::PerformanceHudMode::CompRenderTiming:
                _curPerfHudMode = OvrIntegration::PerformanceHudMode::PerfSummary;
                break;
            case OvrIntegration::PerformanceHudMode::PerfSummary:
                _curPerfHudMode = OvrIntegration::PerformanceHudMode::VersionInfo;
                break;
            case OvrIntegration::PerformanceHudMode::VersionInfo:
                _curPerfHudMode = OvrIntegration::PerformanceHudMode::Off;
                break;
        }

        _session->setPerformanceHudMode(_curPerfHudMode);

    /* Toggle through the debug hud stereo modes */
    } else if(event.key() == KeyEvent::Key::F12) {
        switch(_curDebugHudStereoMode) {
            case OvrIntegration::DebugHudStereoMode::Off:
                _curDebugHudStereoMode = OvrIntegration::DebugHudStereoMode::Quad;
                break;
            case OvrIntegration::DebugHudStereoMode::Quad:
                _curDebugHudStereoMode = OvrIntegration::DebugHudStereoMode::QuadWithCrosshair;
                break;
            case OvrIntegration::DebugHudStereoMode::QuadWithCrosshair:
                _curDebugHudStereoMode = OvrIntegration::DebugHudStereoMode::CrosshairAtInfinity;
                break;
            case OvrIntegration::DebugHudStereoMode::CrosshairAtInfinity:
                _curDebugHudStereoMode = OvrIntegration::DebugHudStereoMode::Off;
                break;
        }

        _session->setDebugHudStereoMode(_curDebugHudStereoMode);

    /* Toggle mirroring */
    } else if(event.key() == KeyEvent::Key::M) {
        _enableMirroring = !_enableMirroring;

    /* Exit */
    } else if(event.key() == KeyEvent::Key::Esc) {
        exit();
    }
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::OvrExample)
