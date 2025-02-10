/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>
        2015, 2016, 2018 — Jonathan Hale <squareys@googlemail.com>

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
#include <Magnum/Magnum.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Renderbuffer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/OvrIntegration/OvrIntegration.h>
#include <Magnum/OvrIntegration/Context.h>
#include <Magnum/OvrIntegration/Session.h>
#include <Magnum/OvrIntegration/Enums.h>

namespace Magnum { namespace Examples {

using namespace Math::Literals;

class OvrExample: public Platform::Application {
    public:
        explicit OvrExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void keyPressEvent(KeyEvent& event) override;

        OvrIntegration::Context _ovrContext;
        std::unique_ptr<OvrIntegration::Session> _session;

        GL::Mesh _mesh{NoCreate};
        Shaders::PhongGL _shader{NoCreate};

        enum: std::size_t { CubeCount = 4 };
        Matrix4 _cubeTransforms[CubeCount]{
            Matrix4::rotationY(45.0_degf)*Matrix4::translation({0.0f, 0.0f, -3.0f}),
            Matrix4::rotationY(45.0_degf)*Matrix4::translation({5.0f, 0.0f, 0.0f}),
            Matrix4::rotationY(45.0_degf)*Matrix4::translation({-10.0f, 0.0f, 0.0f}),
            Matrix4::rotationY(45.0_degf)*Matrix4::translation({0.0f, 0.0f, 7.0f})};
        Color3 _cubeColors[CubeCount]{
            0xffff00_rgbf, 0xff0000_rgbf, 0x0000ff_rgbf, 0x00ffff_rgbf};

        GL::Framebuffer _mirrorFramebuffer{NoCreate};
        GL::Texture2D* _mirrorTexture;

        OvrIntegration::LayerEyeFov* _layer;
        OvrIntegration::PerformanceHudMode _curPerfHudMode{
            OvrIntegration::PerformanceHudMode::Off};
        OvrIntegration::DebugHudStereoMode _curDebugHudStereoMode{
            OvrIntegration::DebugHudStereoMode::Off};

        /* Whether to show contents in the window or just on the VR HMD */
        bool _enableMirroring{true};

        /* Per eye view members */
        GL::Texture2D _depth[2]{GL::Texture2D{NoCreate},
                                GL::Texture2D{NoCreate}};
        std::unique_ptr<OvrIntegration::TextureSwapChain> _textureSwapChain[2];
        GL::Framebuffer _framebuffer[2]{GL::Framebuffer{NoCreate},
                                        GL::Framebuffer{NoCreate}};
        Matrix4 _projectionMatrix[2];
        Deg _cameraRotation = 0.0_degf;
};

OvrExample::OvrExample(const Arguments& arguments): Platform::Application(arguments, NoCreate) {
    /* Connect to an active Oculus session */
    _session = _ovrContext.createSession();

    if(!_session) {
        Error() << "No HMD connected.";
        exit();
        return;
    }

    /* Get the HMD display resolution */
    const Vector2i resolution = _session->resolution()/2;

    /* Create a context with the HMD display resolution */
    Configuration conf;
    conf.setTitle("Magnum OculusVR Example")
        .setSize(resolution);
    GLConfiguration glConf;
    glConf.setSampleCount(16)
          .setSrgbCapable(true);
    if(!tryCreate(conf, glConf))
        create(conf, glConf.setSampleCount(0));

    /* The oculus sdk compositor does some "magic" to reduce latency. For
       that to work, VSync needs to be turned off. */
    if(!setSwapInterval(0))
        Error() << "Could not turn off VSync.";

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FramebufferSrgb);

    _session->configureRendering();

    /* Setup mirroring of oculus sdk compositor results to a texture which can
       later be blitted onto the default framebuffer */
    _mirrorTexture = &_session->createMirrorTexture(resolution);
    _mirrorFramebuffer = GL::Framebuffer(Range2Di::fromSize({}, resolution));
    _mirrorFramebuffer.attachTexture(GL::Framebuffer::ColorAttachment(0), *_mirrorTexture, 0)
                      .mapForRead(GL::Framebuffer::ColorAttachment(0));

    /* Setup cube mesh */
    _mesh = MeshTools::compile(Primitives::cubeSolid());

    /* Setup shader */
    _shader = Shaders::PhongGL{};
    _shader.setShininess(20)
           .setLightPositions({{3.0f, 3.0f, 3.0f, 0.0f}});

    /* Setup compositor layers */
    _layer = &_ovrContext.compositor().addLayerEyeFov();
    _layer->setFov(*_session.get())
           .setHighQuality(true);

    /* Setup per-eye views */
    for(Int eye: {0, 1}) {
        _projectionMatrix[eye] = _session->projectionMatrix(eye, 0.001f, 100.0f);

        const Vector2i textureSize = _session->fovTextureSize(eye);
        _textureSwapChain[eye] =_session->createTextureSwapChain(textureSize);

        /* Create the framebuffer which will be used to render to the current
        texture of the texture set later. */
        _framebuffer[eye] = GL::Framebuffer{{{}, textureSize}};
        _framebuffer[eye].mapForDraw(GL::Framebuffer::ColorAttachment(0));

        /* Setup depth attachment */
        _depth[eye] = GL::Texture2D{};
        _depth[eye].setMinificationFilter(GL::SamplerFilter::Linear)
                   .setWrapping(GL::SamplerWrapping::ClampToEdge)
                   .setStorage(1, GL::TextureFormat::DepthComponent32F, textureSize);

        _layer->setColorTexture(eye, *_textureSwapChain[eye])
               .setViewport(eye, {{}, textureSize});
    }
}

void OvrExample::drawEvent() {
    /* Get orientation and position of the hmd. */
    const std::array<DualQuaternion, 2> poses = _session->pollEyePoses().eyePoses();

    /* Draw the scene for both eyes */
    for(Int eye: {0, 1}) {
        /* Switch to eye render target and bind render textures */
        _framebuffer[eye]
            .attachTexture(GL::Framebuffer::ColorAttachment(0), _textureSwapChain[eye]->activeTexture(), 0)
            .attachTexture(GL::Framebuffer::BufferAttachment::Depth, _depth[eye], 0)
            /* Clear with the standard gray so that at least that will be visible in
            case the scene is not correctly set up */
            .clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth)
            .bind();

        /* Render scene */
        const Matrix4 viewProjMatrix = _projectionMatrix[eye]*poses[eye].inverted().toMatrix()*Matrix4::rotationY(_cameraRotation);
        for(Int cubeIndex = 0; cubeIndex < CubeCount; ++cubeIndex) {
            _shader.setDiffuseColor(_cubeColors[cubeIndex])
                .setTransformationMatrix(_cubeTransforms[cubeIndex])
                .setNormalMatrix(_cubeTransforms[cubeIndex].normalMatrix())
                .setProjectionMatrix(viewProjMatrix)
                .draw(_mesh);
        }

        /* Commit changes and use next texture in chain */
        _textureSwapChain[eye]->commit();

        /* Reasoning for the next two lines, taken from the Oculus SDK examples
           code: Without this, [during the next frame, this method] would bind a
           framebuffer with an invalid COLOR_ATTACHMENT0 because the texture ID
           associated with COLOR_ATTACHMENT0 had been unlocked by calling
           wglDXUnlockObjectsNV(). */
        _framebuffer[eye].detach(GL::Framebuffer::ColorAttachment(0))
                         .detach(GL::Framebuffer::BufferAttachment::Depth);
    }

    /* Set the layers eye poses to the poses chached in the _hmd. */
    _layer->setRenderPoses(*_session.get());

    /* Let the libOVR sdk compositor do its magic! */
    _ovrContext.compositor().submitFrame(*_session.get());

    if(_enableMirroring) {
        /* Blit mirror texture to default framebuffer */
        const Vector2i size = _mirrorTexture->imageSize(0);
        GL::Framebuffer::blit(_mirrorFramebuffer,
            GL::defaultFramebuffer,
            {{0, size.y()}, {size.x(), 0}},
            {{}, size},
            GL::FramebufferBlit::Color, GL::FramebufferBlitFilter::Nearest);

        swapBuffers();
    }

    /* Provide some rotation, but only without real devices to avoid VR sickness ;) */
    if(_session->isDebugHmd()) _cameraRotation += 0.1_degf;

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
