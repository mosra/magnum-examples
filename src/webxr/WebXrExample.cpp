/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 —
            Vladimír Vondruš <mosra@centrum.cz>
        2019 — Jonathan Hale <squareys@googlemail.com>

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
#include <Magnum/Magnum.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Vector4.h>
#include <Magnum/Math/Range.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/EmscriptenApplication.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/UVSphere.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/MeshData.h>

#include <emscripten.h>
#include <emscripten/html5.h>
#include "webxr.h"

namespace Magnum { namespace Examples {

using namespace Math::Literals;

class WebXrExample: public Platform::Application {
    public:
        explicit WebXrExample(const Arguments& arguments);
        ~WebXrExample();

        /* Callbacks for WebXR */
        void onError(int error);
        void drawWebXRFrame(WebXRView* views);
        void sessionStart();
        void sessionEnd();

    private:
        void drawEvent() override;
        void keyPressEvent(KeyEvent& e) override;
        void mousePressEvent(MouseEvent& event) override;

        GL::Mesh _cubeMesh;
        Matrix4 _cubeTransformations[4]{
            Matrix4::translation({ 0.0f,  0.0f, -3.0f})*Matrix4::rotationY(45.0_degf),
            Matrix4::translation({ 5.0f,  0.0f,  0.0f})*Matrix4::rotationY(45.0_degf),
            Matrix4::translation({-10.0f, 0.0f,  0.0f})*Matrix4::rotationY(45.0_degf),
            Matrix4::translation({ 0.0f,  0.0f,  7.0f})*Matrix4::rotationY(45.0_degf)};
        Color3 _cubeColors[4]{
            0xffff00_rgbf, 0xff0000_rgbf, 0x0000ff_rgbf, 0x00ffff_rgbf};

        GL::Mesh _controllerMesh;
        Matrix4 _controllerTransformations[2];
        Color3 _controllerColors[2]{
            {0.0f, 0.0f, 1.0f},
            {1.0f, 0.0f, 0.0f}};

        Shaders::Phong _shader;
        Matrix4 _projectionMatrices[2];
        Matrix4 _viewMatrices[2];
        Range2Di _viewports[2];

        bool _inXR = false;
};

WebXrExample::WebXrExample(const Arguments& arguments):
    Platform::Application(arguments,
        Configuration{},
        GLConfiguration{}.setSampleCount(4))
{
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);

    /* Setup cube mesh */
    _cubeMesh = MeshTools::compile(Primitives::cubeSolid());
    _controllerMesh = MeshTools::compile(Primitives::uvSphereSolid(8, 8));

    /* Left and right controller placeholders */
    _shader.setSpecularColor(Color3(1.0f))
          .setShininess(20);

    webxr_init(
        WEBXR_SESSION_MODE_IMMERSIVE_VR,
        /* Frame callback */
        [](void* userData, int, float[16], WebXRView* views) {
            static_cast<WebXrExample*>(userData)->drawWebXRFrame(views);
        },
        /* Session end callback */
        [](void* userData) {
            static_cast<WebXrExample*>(userData)->sessionStart();
        },
        /* Session end callback */
        [](void* userData) {
            static_cast<WebXrExample*>(userData)->sessionEnd();
        },
        /* Error callback */
        [](void* userData, int error) {
            static_cast<WebXrExample*>(userData)->onError(error);
        },
        /* userData */
        this);

    redraw();
}

WebXrExample::~WebXrExample() {
    if(_inXR) webxr_request_exit();
}

void WebXrExample::drawWebXRFrame(WebXRView* views) {
    int viewIndex = 0;
    for(WebXRView view : {views[0], views[1]}) {
        _viewports[viewIndex] = Range2Di::fromSize(
            {view.viewport[0], view.viewport[1]},
            {view.viewport[2], view.viewport[3]});
        _viewMatrices[viewIndex] = Matrix4::from(view.viewMatrix);
        _projectionMatrices[viewIndex] = Matrix4::from(view.projectionMatrix);

        ++viewIndex;
    }

    WebXRInputSource sources[2];
    int sourcesCount = 0;
    webxr_get_input_sources(sources, 5, &sourcesCount);

    for(int i = 0; i < sourcesCount; ++i) {
        webxr_get_input_pose(&sources[i], _controllerTransformations[i].data());
    }

    drawEvent();
}

void WebXrExample::drawEvent() {
    if(!_inXR) {
        /* Single view */
        _projectionMatrices[0] = Matrix4::perspectiveProjection(90.0_degf,
            Vector2(windowSize()).aspectRatio(), 0.01f, 100.0f);
        _viewMatrices[0] = Matrix4{};
        _viewports[0] = Range2Di{{}, framebufferSize()};

        /* Set some default transformation for the controllers so that they don't
           block view */
        _controllerTransformations[0] = Matrix4::translation({-0.5f, -0.4f, -1.0f});
        _controllerTransformations[1] = Matrix4::translation({0.5f, -0.4f, -1.0f});
    }

    GL::defaultFramebuffer.clear(
        GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    const Vector3 lightPos{3.0f, 3.0f, 3.0f};

    const int viewCount = _inXR ? 2 : 1;
    for(int eye = 0; eye < viewCount; ++eye) {
        GL::defaultFramebuffer.setViewport(_viewports[eye]);

        _shader.setLightPosition(_viewMatrices[eye].transformPoint(lightPos))
               .setProjectionMatrix(_projectionMatrices[eye]);

        /* Draw cubes */
        for(int i = 0; i < 4; ++i) {
            const Matrix4& transformationMatrix =
                _viewMatrices[eye]*_cubeTransformations[i];
            _shader.setTransformationMatrix(transformationMatrix)
                .setNormalMatrix(transformationMatrix.rotationScaling())
                .setDiffuseColor(_cubeColors[i])
                .draw(_cubeMesh);
        }

        /* Draw controller models */
        for(int i = 0; i < 2; ++i) {
            const Matrix4& transformationMatrix =
                _viewMatrices[eye]*_controllerTransformations[i]*Matrix4::scaling(Vector3{0.05f});
            _shader.setTransformationMatrix(transformationMatrix)
                .setNormalMatrix(transformationMatrix.rotationScaling())
                .setDiffuseColor(_controllerColors[i])
                .draw(_controllerMesh);
        }
    }

    /* No need to call redraw() on WebXR, the webxr callback already does this */
}

void WebXrExample::sessionStart() {
    if(_inXR) return;
    _inXR = true;

    Debug{} << "Entered VR";
}

void WebXrExample::sessionEnd() {
    _inXR = false;
    Debug{} << "Exited VR";
    redraw();
}

void WebXrExample::onError(int error) {
    switch(error) {
        case WEBXR_ERR_API_UNSUPPORTED:
             Error{} << "WebXR unsupported in this browser.";
             break;
        case WEBXR_ERR_GL_INCAPABLE:
             Error{} << "GL context cannot be used to render to WebXR";
             break;
        case WEBXR_ERR_SESSION_UNSUPPORTED:
             Error{} << "VR not supported on this device";
             break;
        default:
             Error{} << "Unknown WebXR error with code" << error;
    }
}

void WebXrExample::keyPressEvent(KeyEvent& e) {
    if(e.key() == KeyEvent::Key::Esc && _inXR) {
        webxr_request_exit();
    }
}

void WebXrExample::mousePressEvent(MouseEvent& event) {
    if(event.button() != MouseEvent::Button::Left) return;
    /* Request rendering to the XR device */
    webxr_request_session();
    event.setAccepted();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::WebXrExample)
