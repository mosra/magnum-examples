/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
            Vladimír Vondruš <mosra@centrum.cz>
        2017, 2018 — Jonathan Hale <squareys@googlemail.com>

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
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Range.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/Platform/EmscriptenApplication.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/MeshData3D.h>

#include <emscripten.h>
#include <emscripten/vr.h>
#include <emscripten/html5.h>

namespace Magnum { namespace Examples {

class WebVrExample: public Platform::Application {
    public:
        explicit WebVrExample(const Arguments& arguments);
        ~WebVrExample();

        void onClick();
        void displayRender();
        void displayPresent();
        void vrReady();

    private:
        void drawEvent() override;
        void keyPressEvent(KeyEvent& e) override;

        GL::Buffer _indexBuffer, _vertexBuffer;
        GL::Mesh _mesh;
        Shaders::Phong _shader;

        Matrix4 _cubeModelMatrices[4];
        Color3 _cubeColors[4];

        bool _vrInitialized = false;

        VRDisplayHandle _displayHandle;
        Vector2i _displayResolution;
};

namespace {
/* Click callback for requesting present */
EM_BOOL clickCallback(int eventType, const EmscriptenMouseEvent* e, void* app) {
    if(!e || eventType != EMSCRIPTEN_EVENT_CLICK) return EM_FALSE;

    static_cast<WebVrExample*>(app)->onClick();
    return EM_FALSE;
}

/* Touch callback for requesting present on mobile */
EM_BOOL touchCallback(int eventType, const EmscriptenTouchEvent* e, void* app) {
    if(!e || eventType != EMSCRIPTEN_EVENT_TOUCHEND) return EM_FALSE;

    static_cast<WebVrExample*>(app)->onClick();
    return EM_FALSE;
}

/* Render loop callback for the VR display */
void displayRenderCallback(void* app) {
    static_cast<WebVrExample*>(app)->displayRender();
}

void displayPresentCallback(void* app) {
    static_cast<WebVrExample*>(app)->displayPresent();
}

}

WebVrExample::WebVrExample(const Arguments& arguments):
    Platform::Application(arguments,
        Configuration{}.setSize({640, 320}),
        GLConfiguration{}.setSampleCount(4))
{
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);

    /* Setup cube mesh */
    const Trade::MeshData3D cube = Primitives::cubeSolid();
    _vertexBuffer.setData(MeshTools::interleave(cube.positions(0), cube.normals(0)), GL::BufferUsage::StaticDraw);

    Containers::Array<char> indexData;
    MeshIndexType indexType;
    UnsignedInt indexStart, indexEnd;
    std::tie(indexData, indexType, indexStart, indexEnd) = MeshTools::compressIndices(cube.indices());

    _indexBuffer.setTargetHint(GL::Buffer::TargetHint::ElementArray);
    _indexBuffer.setData(indexData, GL::BufferUsage::StaticDraw);

    _mesh.setPrimitive(cube.primitive())
        .setCount(cube.indices().size())
        .addVertexBuffer(_vertexBuffer, 0, Shaders::Phong::Position{}, Shaders::Phong::Normal{})
        .setIndexBuffer(_indexBuffer, 0, indexType, indexStart, indexEnd);

    /* Setup scene */
    _cubeModelMatrices[0] = Matrix4::translation({ 0.0f,  0.0f, -3.0f})*Matrix4::rotationY(Deg(45.0f));
    _cubeModelMatrices[1] = Matrix4::translation({ 5.0f,  0.0f,  0.0f})*Matrix4::rotationY(Deg(45.0f));
    _cubeModelMatrices[2] = Matrix4::translation({-10.0f, 0.0f,  0.0f})*Matrix4::rotationY(Deg(45.0f));
    _cubeModelMatrices[3] = Matrix4::translation({ 0.0f,  0.0f,  7.0f})*Matrix4::rotationY(Deg(45.0f));

    _cubeColors[0] = {1.0f, 1.0f, 0.0f};
    _cubeColors[1] = {1.0f, 0.0f, 0.0f};
    _cubeColors[2] = {0.0f, 0.0f, 1.0f};
    _cubeColors[3] = {0.0f, 1.0f, 1.0f};

    _shader.setSpecularColor(Color3(1.0f))
          .setShininess(20);

    /* Initialize the VR API of emscripten */
#if EMSCRIPTEN_VR_API_VERSION >= 10101
    /* This version of the emscripten WebVR API uses callbacks
     * instead of busy polling of emscripten_vr_ready() to notify
     * when the API has finished initializing. */
    emscripten_vr_init([](void* app) {
        static_cast<WebVrExample*>(app)->vrReady();
    }, this);
#else
    emscripten_vr_init();
#endif

    Debug{} << "Browser is running WebVR version"
            << emscripten_vr_version_major()
            << Debug::nospace << "." << Debug::nospace
            << emscripten_vr_version_minor();
}

WebVrExample::~WebVrExample() {
    emscripten_vr_cancel_display_render_loop(_displayHandle);

#if EMSCRIPTEN_VR_API_VERSION >= 10101
    emscripten_vr_deinit();
#endif
}

void WebVrExample::vrReady() {
    Debug{} << "";

    const int numDisplays = emscripten_vr_count_displays();
    if(numDisplays <= 0) {
        Fatal{} << "No VR displays found, exiting.";
        exit();
    }

    Debug{} << "Found" << numDisplays << "VR displays.";

    /* Use first of the available display */
    _displayHandle = emscripten_vr_get_display_handle(0);

    const char* devName = emscripten_vr_get_display_name(_displayHandle);
    Debug{} << "Using VRDisplay with name: " << devName;

    if(!emscripten_vr_set_display_render_loop_arg(_displayHandle, displayRenderCallback, this)) {
        Fatal{} << "Failed to set display render loop for VR display!";
        exit();
    }

    /* Set callbacks for getting present permission for the VR display */
    emscripten_set_click_callback("#canvas", this, true, clickCallback);
    emscripten_set_touchend_callback("#canvas", this, true, touchCallback);

    _vrInitialized = true;
}

void WebVrExample::displayPresent() {
    Debug{} << "Presenting to VR display.";

    VREyeParameters eyeLeft, eyeRight;
    emscripten_vr_get_eye_parameters(_displayHandle, VREyeLeft, &eyeLeft);
    emscripten_vr_get_eye_parameters(_displayHandle, VREyeRight, &eyeRight);

    _displayResolution = {2*int(Math::max(eyeLeft.renderWidth, eyeRight.renderWidth)), int(eyeLeft.renderHeight)};
   if(emscripten_set_canvas_element_size("#canvas", _displayResolution.x(), _displayResolution.y()) != EMSCRIPTEN_RESULT_SUCCESS) {
       Fatal{} << "Could not set canvas element size to " << _displayResolution;
       exit();
   }

    Debug{} << "Set canvas size to" << _displayResolution;
}

void WebVrExample::drawEvent() {
#if EMSCRIPTEN_VR_API_VERSION < 10101
    /* Only initialize once */
    if(_vrInitialized) return;

    /* Has navigator.getVRDisplays returned yet? */
    if(!emscripten_vr_ready()) {
        /* Cannot initialize yet */
        redraw();
        return;
    }

    vrReady();
#endif
}

void WebVrExample::displayRender() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    const Vector3 lightPos{3.0f, 3.0f, 3.0f};

    /* Get matrices and poses for this frame */
    VRFrameData fd;
    emscripten_vr_get_frame_data(_displayHandle, &fd);

    const bool inVR = emscripten_vr_display_presenting(_displayHandle);

    const Matrix4 viewMatrix[2] = {
        Matrix4::from(fd.leftViewMatrix),
        Matrix4::from(fd.rightViewMatrix)};
    const Matrix4 projMatrix[2] = {
        inVR ? Matrix4::from(fd.leftProjectionMatrix) : Matrix4::perspectiveProjection(Deg(90.0f), 2.0f, 0.001, 100.0f),
        Matrix4::from(fd.rightProjectionMatrix)};

    const int halfWidth = 0.5f*_displayResolution.x();
    Range2Di viewport[2] = {
        {{}, inVR ? Vector2i{halfWidth, _displayResolution.y()} : _displayResolution},
        {{halfWidth, 0}, _displayResolution}
    };

    const int views = inVR ? 2 : 1;
    for (int eye = 0; eye < views; ++eye) {
        _shader.setLightPosition(viewMatrix[eye].transformPoint(lightPos))
               .setProjectionMatrix(projMatrix[eye]);

        /* Set viewport to left/right half of canvas */
        GL::defaultFramebuffer.setViewport(viewport[eye]);

        /* Draw all four cubes for this eye */
        for (int i = 0; i < 4; ++i) {
            const Matrix4& transformationMatrix = _cubeModelMatrices[i];
            _shader.setTransformationMatrix(viewMatrix[eye]*transformationMatrix)
                .setNormalMatrix((viewMatrix[eye]*transformationMatrix).rotationScaling())
                .setDiffuseColor(_cubeColors[i]);
            _mesh.draw(_shader);
        }
    }

    if(inVR) emscripten_vr_submit_frame(_displayHandle);
}

void WebVrExample::keyPressEvent(KeyEvent& e) {
    if(e.key() == KeyEvent::Key::Esc &&
       emscripten_vr_display_presenting(_displayHandle))
    {
        emscripten_vr_exit_present(_displayHandle);
    }
}

void WebVrExample::onClick() {
    if(emscripten_vr_display_presenting(_displayHandle)) return;

    Debug{} << "Requesting present for VR display";

    /* Request present on emscripten default canvas */
    VRLayerInit init = {0, VR_LAYER_DEFAULT_LEFT_BOUNDS, VR_LAYER_DEFAULT_RIGHT_BOUNDS};

    if (!emscripten_vr_request_present(_displayHandle, &init, 1, displayPresentCallback, this)) {
        Error{} << "Error while requesting present permission for VR display.";
        exit();
    }
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::WebVrExample)
