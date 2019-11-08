/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
            Vladimír Vondruš <mosra@centrum.cz>
        2019 — Nghia Truong <nghiatruong.vn@gmail.com>

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

#include "DrawableObjects/ParticleGroup.h"
#include "DrawableObjects/WireframeObjects.h"

#include "SPH/SPHSolver.h"
#include "FluidSimApp.h"

#include <Corrade/Utility/StlMath.h>
#include <Magnum/Animation/Easing.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/PixelFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Image.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Version.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>

static constexpr float s_ParticleRadius = 0.02f;

namespace Magnum { namespace Examples {
using namespace Math::Literals;

/****************************************************************************************************/
FluidSimApp::FluidSimApp(const Arguments& arguments) :
    Platform::Application{arguments, NoCreate} {
    /* Setup window */
    {
        const Vector2 dpiScaling = this->dpiScaling({});
        Configuration conf;
        conf.setTitle("Magnum Fluid Simulation Example")
            .setSize(conf.size(), dpiScaling)
            .setWindowFlags(Configuration::WindowFlag::Resizable);
        GLConfiguration glConf;
        glConf.setSampleCount(dpiScaling.max() < 2.0f ? 8 : 2);
        if(!tryCreate(conf, glConf)) {
            create(conf, glConf.setSampleCount(0));
        }

        GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
        GL::Renderer::setClearColor(Color3 { 0.35f });

        /* Loop at 60 Hz max */
        setSwapInterval(1);
        setMinimalLoopPeriod(16);
    }

    /* Setup ImGui */
    {
#if 0
        ImGui::CreateContext();
        ImGui::GetIO().Fonts->AddFontFromFileTTF(
            "D:/Programming/Simulations/SPH/SimpleSPH/SourceSansPro-Regular.ttf", 18.0f);
        m_ImGuiContext = ImGuiIntegration::Context(*ImGui::GetCurrentContext(),
                                                   Vector2{ windowSize() } /*/ dpiScaling()*/, windowSize(), framebufferSize());
#else
        m_ImGuiContext = ImGuiIntegration::Context(Vector2{ windowSize() } / dpiScaling(), windowSize(), framebufferSize());
#endif
        ImGui::StyleColorsDark();

        /* Setup proper blending to be used by ImGui. There's a great chance
           you'll need this exact behavior for the rest of your scene. If not, set
           this only for the drawFrame() call. */
        GL::Renderer::setBlendEquation(GL::Renderer::BlendEquation::Add,
                                       GL::Renderer::BlendEquation::Add);
        GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::SourceAlpha,
                                       GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    }

    /* Setup scene objects and camera */
    {
        /* Setup scene objects */
        _scene.reset(new Scene3D{});
        _drawableGroup.reset(new SceneGraph::DrawableGroup3D{});

        /* Configure camera */
        _objCamera.reset(new Object3D{ _scene.get() });
        _objCamera->setTransformation(Matrix4::lookAt(Vector3(0, 1.5, 8), Vector3(0, 1, 0), Vector3(0, 1, 0)));

        const auto viewportSize = GL::defaultFramebuffer.viewport().size();
        _camera.reset(new SceneGraph::Camera3D{ *_objCamera });
        _camera->setProjectionMatrix(Matrix4::perspectiveProjection(45.0_degf, Vector2{ viewportSize }.aspectRatio(), 0.01f, 1000.0f))
            .setViewport(viewportSize);

        /* Set default camera parameters */
        _defaultCamPosition = Vector3(1.5f, 3.3f, 6.0f);
        _defaultCamTarget   = Vector3(1.5f, 1.3f, 0.0f);
        _objCamera->setTransformation(Matrix4::lookAt(_defaultCamPosition, _defaultCamTarget, Vector3(0, 1, 0)));

        /* Initialize depth to the value at scene center */
        _lastDepth = ((_camera->projectionMatrix() * _camera->cameraMatrix()).transformPoint({}).z() + 1.0f) * 0.5f;
    }

    /* Setup ground grid */
    {
        _grid.reset(new WireframeGrid(_scene.get(), _drawableGroup.get()));
        _grid->transform(Matrix4::scaling(Vector3(0.5f)) * Matrix4::translation(Vector3(3, 0, -5)));
    }

    /* Setup fluid solver */
    {
        _fluidSolver.reset(new SPHSolver(s_ParticleRadius));

        /* Simulation domain box */
        /* Transform the box to cover the region [0, 0, 0] to [3, 3, 1] */
        _drawableBox.reset(new WireframeBox(_scene.get(), _drawableGroup.get()));
        _drawableBox->transform(Matrix4::scaling(Vector3{ 1.5, 1.5, 0.5 }) * Matrix4::translation(Vector3(1)));
        _drawableBox->setColor(Color3(1, 1, 0));

        /* Drawable particles */
        _drawableParticles.reset(new ParticleGroup(_fluidSolver->particlePositions(), s_ParticleRadius));

        /* Initialize scene particles */
        initializeScene();
    }

    /* Start the timer */
    _timeline.start();
}

/****************************************************************************************************/
void FluidSimApp::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);
    m_ImGuiContext.newFrame();

    /* Enable text input, if needed */
    if(ImGui::GetIO().WantTextInput && !isTextInputActive()) {
        startTextInput();
    } else if(!ImGui::GetIO().WantTextInput && isTextInputActive()) {
        stopTextInput();
    }

    /*
     * Pause simulation if the mouse was pressed (camera is moving around)
       This avoid freezing GUI while running the simulation
     */
    if(!_bPausedSimulation && !_bMousePressed) {
        /* Adjust the substep number to maximize CPU usage each frame */
        const auto lastAvgStepTime = _timeline.previousFrameDuration() / static_cast<float>(_substeps);
        const auto newSubsteps     = lastAvgStepTime > 0 ? static_cast<int>(1.0f / 60.0f / lastAvgStepTime) + 1 : 1;
        if(Math::abs(newSubsteps - _substeps) > 1) {
            _substeps = newSubsteps;
        }

        for(int i = 0; i < _substeps; ++i) {
            simulationStep();
        }
    }

    /* Draw objects */
    {
        /* Draw particles */
        _drawableParticles->setDirty(); /* Trigger drawable object to update the particles to the GPU */
        _drawableParticles->draw(_camera, GL::defaultFramebuffer.viewport().size());

        /* Draw other objects (ground grid) */
        _camera->draw(*_drawableGroup);
    }

    /* Menu for parameters */
    if(_bShowMenu) {
        showMenu();
    }

    /* Render ImGui window */
    {
        /* Set appropriate states. If you only draw imgui UI, it is sufficient to do this once in the constructor. */
        GL::Renderer::enable(GL::Renderer::Feature::Blending);
        GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
        GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
        GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);

        m_ImGuiContext.drawFrame();

        /* Reset state. Only needed if you want to draw something else with different state next frame. */
        GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
        GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
        GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
        GL::Renderer::disable(GL::Renderer::Feature::Blending);
    }

    swapBuffers();
    _timeline.nextFrame();

    /* Run next frame immediately */
    redraw();
}

/****************************************************************************************************/
void FluidSimApp::viewportEvent(ViewportEvent& event) {
    const auto newBufferSize = event.framebufferSize();

    /* Resize the main framebuffer */
    GL::defaultFramebuffer.setViewport({ {}, newBufferSize });

    /* Relayout ImGui */
    m_ImGuiContext.relayout(Vector2{ event.windowSize() } / event.dpiScaling(), event.windowSize(), newBufferSize);

    /* Recompute the camera's projection matrix */
    _camera->setViewport(newBufferSize);
}

/****************************************************************************************************/
void FluidSimApp::keyPressEvent(Platform::Sdl2Application::KeyEvent& event) {
    switch(event.key()) {
        case KeyEvent::Key::H: {
            _bShowMenu ^= true;
            event.setAccepted(true);
        }
        break;
        case KeyEvent::Key::R: {
            initializeScene();
            event.setAccepted(true);
        }
        break;
        case KeyEvent::Key::Space: {
            _bPausedSimulation ^= true;
            event.setAccepted(true);
        }
        break;
        default:
            if(m_ImGuiContext.handleKeyPressEvent(event)) {
                event.setAccepted(true);
            }
    }
}

void FluidSimApp::keyReleaseEvent(KeyEvent& event) {
    if(m_ImGuiContext.handleKeyReleaseEvent(event)) {
        event.setAccepted(true);
        return;
    }
}

/****************************************************************************************************/
void FluidSimApp::mousePressEvent(MouseEvent& event) {
    if(m_ImGuiContext.handleMousePressEvent(event)) {
        event.setAccepted(true);
        return;
    }

    if((event.button() != MouseEvent::Button::Left)
       && (event.button() != MouseEvent::Button::Right)) {
        return;
    }

    /* Update camera */
    {
        _prevMousePosition = event.position();
        const auto currentDepth = depthAt(event.position());
        const auto depth        = currentDepth == 1.0f ? _lastDepth : currentDepth;
        _translationPoint = unproject(event.position(), depth);

        /* Update the rotation point only if we're not zooming against infinite
           depth or if the original rotation point is not yet initialized */
        if(currentDepth != 1.0f || _rotationPoint.isZero()) {
            _rotationPoint = _translationPoint;
            _lastDepth     = depth;
        }
    }
    _bMousePressed = true;
}

void FluidSimApp::mouseReleaseEvent(MouseEvent& event) {
    _bMousePressed = false;
    if(m_ImGuiContext.handleMouseReleaseEvent(event)) {
        event.setAccepted(true);
    }
}

/****************************************************************************************************/
void FluidSimApp::mouseMoveEvent(MouseMoveEvent& event) {
    if(m_ImGuiContext.handleMouseMoveEvent(event)) {
        event.setAccepted(true);
        return;
    }

    if(!(event.buttons() & MouseMoveEvent::Button::Left)
       && !(event.buttons() & MouseMoveEvent::Button::Right)) {
        return;
    }

    const auto delta = 3.0f * Vector2{ event.position() - _prevMousePosition } /
    Vector2{ GL::defaultFramebuffer.viewport().size() };
    _prevMousePosition = event.position();

    if(event.buttons() & MouseMoveEvent::Button::Left) {
        _objCamera->transformLocal(
            Matrix4::translation(_rotationPoint) *
            Matrix4::rotationX(-0.51_radf * delta.y()) *
            Matrix4::rotationY(-0.51_radf * delta.x()) *
            Matrix4::translation(-_rotationPoint));
    } else {
        const auto p = unproject(event.position(), _lastDepth);
        _objCamera->translateLocal(_translationPoint - p); /* is Z always 0? */
        _translationPoint = p;
    }
    event.setAccepted();
}

/****************************************************************************************************/
void FluidSimApp::mouseScrollEvent(MouseScrollEvent& event) {
    const auto delta = event.offset().y();
    if(std::abs(delta) < 1.0e-2f) {
        return;
    }

    if(m_ImGuiContext.handleMouseScrollEvent(event)) {
        /* Prevent scrolling the page */
        event.setAccepted();
        return;
    }

    const auto    currentDepth = depthAt(event.position());
    const auto    depth        = currentDepth == 1.0f ? _lastDepth : currentDepth;
    const Vector3 p = unproject(event.position(), depth);
    /* Update the rotation point only if we're not zooming against infinite
       depth or if the original rotation point is not yet initialized */
    if(currentDepth != 1.0f || _rotationPoint.isZero()) {
        _rotationPoint = p;
        _lastDepth     = depth;
    }

    /* Move towards/backwards the rotation point in cam coords */
    _objCamera->translateLocal(_rotationPoint * delta * 0.1f);
}

/****************************************************************************************************/
void FluidSimApp::textInputEvent(TextInputEvent& event) {
    if(m_ImGuiContext.handleTextInputEvent(event)) {
        event.setAccepted(true);
    }
}

/****************************************************************************************************/
float FluidSimApp::depthAt(const Vector2i& windowPosition) {
    /* First scale the position from being relative to window size to being
       relative to framebuffer size as those two can be different on HiDPI
       systems */
    const auto position   = windowPosition * Vector2{ framebufferSize() } / Vector2{ windowSize() };
    const auto fbPosition = Vector2i{ position.x(), GL::defaultFramebuffer.viewport().sizeY() - position.y() - 1 };

    GL::defaultFramebuffer.mapForRead(GL::DefaultFramebuffer::ReadAttachment::Front);
    Image2D data = GL::defaultFramebuffer.read(
        Range2Di::fromSize(fbPosition, Vector2i{ 1 }).padded(Vector2i{ 2 }),
        { GL::PixelFormat::DepthComponent, GL::PixelType::Float });

    return Math::min<float>(Containers::arrayCast<const float>(data.data()));
}

/****************************************************************************************************/
Vector3 FluidSimApp::unproject(const Vector2i& windowPosition, float depth) const {
    /* We have to take window size, not framebuffer size, since the position is
       in window coordinates and the two can be different on HiDPI systems */
    const auto viewSize     = windowSize();
    const auto viewPosition = Vector2i{ windowPosition.x(), viewSize.y() - windowPosition.y() - 1 };
    const auto in           = Vector3{ 2 * Vector2{ viewPosition } / Vector2{ viewSize } - Vector2{ 1.0f }, depth*2.0f - 1.0f };

    /*
        Use the following to get global coordinates instead of camera-relative:
        (_cameraObject->absoluteTransformationMatrix()*_camera->projectionMatrix().inverted()).transformPoint(in)
     */
    return _camera->projectionMatrix().inverted().transformPoint(in);
}

/****************************************************************************************************/
void FluidSimApp::showMenu() {
    ImGui::SetNextWindowBgAlpha(0.5f);
    ImGui::Begin("Options", nullptr);
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.6f);

    /* General information */
    ImGui::Text("Hide/show menu: H");
    ImGui::Text("Num. particles: %d",         static_cast<int>(_fluidSolver->numParticles()));
    ImGui::Text("Simulation steps/frame: %d", _substeps);
    ImGui::Text("Rendering: %3.2f FPS",       static_cast<double>(ImGui::GetIO().Framerate));
    ImGui::Spacing();

    /* Rendering parameters */
    if(ImGui::CollapsingHeader("Particle Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Particle Rendering");
        {
            const char* items[]   = { "Uniform", "Ramp by ID", "Random" };
            static int  colorMode = 1;
            if(ImGui::Combo("Color Mode", &colorMode, items, 3)) { _drawableParticles->setColorMode(colorMode); }
            if(colorMode == 0) { /* Uniform color */
                ImGui::ColorEdit3("Diffuse Color", _drawableParticles->diffuseColor().data());
            }
        }
        ImGui::InputFloat3("Light Direction", _drawableParticles->lightDirection().data());
        ImGui::PopID();
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    /* Simulation parameters */
    if(ImGui::CollapsingHeader("Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Simulation");
        ImGui::InputFloat("Stiffness", &_fluidSolver->simulationParameters().stiffness);
        ImGui::SliderFloat("Viscosity",   &_fluidSolver->simulationParameters().viscosity,           0.0f, 1.0f);
        ImGui::SliderFloat("Restitution", &_fluidSolver->simulationParameters().boundaryRestitution, 0.0f, 1.0f);
        ImGui::Checkbox("Dynamic Boundary", &_bDynamicBoundary);
        ImGui::PopID();
    }
    ImGui::Spacing();
    ImGui::Separator();

    /* Reset */
    ImGui::Spacing();
    if(ImGui::Button("Reset Simulation")) { initializeScene(); } ImGui::SameLine();
    if(ImGui::Button("Reset Camera")) {
        _objCamera->setTransformation(Matrix4::lookAt(_defaultCamPosition, _defaultCamTarget, Vector3(0, 1, 0)));
    }
    ImGui::PopItemWidth();
    ImGui::End();
}

/****************************************************************************************************/
void FluidSimApp::initializeScene() {
    if(_fluidSolver->numParticles() > 0) {
        _fluidSolver->reset();
    } else {
        const auto lowerCorner = Vector3(s_ParticleRadius * 2.0f);
        const auto upperCorner = Vector3(0.5, 2, 1) - Vector3(s_ParticleRadius * 2.0f);
        const auto spacing     = s_ParticleRadius * 2.0f;
        const auto resolution { (upperCorner - lowerCorner) / spacing };

        std::vector<Vector3> tmp;
        tmp.reserve(static_cast<size_t>(resolution.x() * resolution.y() * resolution.z()));
        for(int i = 0; i < resolution[0]; ++i) {
            for(int j = 0; j < resolution[1]; ++j) {
                for(int k = 0; k < resolution[2]; ++k) {
                    tmp.push_back(Vector3(i, j, k) * spacing + lowerCorner);
                }
            }
        }
        _fluidSolver->setPositions(tmp);
    }

    /* Reset domain */
    if(_bDynamicBoundary) { _boundaryOffset = 0.0f; }
    _drawableBox->setTransformation(Matrix4::scaling(Vector3{ 1.5f, 1.5f, 0.5f }) * Matrix4::translation(Vector3(1)));
    _fluidSolver->domainBox().upperDomainBound().x() = 3.0f - s_ParticleRadius;

    /* Trigger drawable object to upload particles to the GPU */
    _drawableParticles->setDirty();
}

/****************************************************************************************************/
void FluidSimApp::simulationStep() {
    static float offset { 0.0f };
    if(_bDynamicBoundary) {
        /* Change fluid boundary */
        static float step = 2.0e-3f;
        if(_boundaryOffset > 1.0f || _boundaryOffset < 0.0f) {
            step *= -1.0f;
        }
        _boundaryOffset += step;
        offset           = Math::lerp(0.0f, 0.5f, Animation::Easing::quadraticInOut(_boundaryOffset));
    }

    _drawableBox->setTransformation(Matrix4::scaling(Vector3{ 1.5f - offset, 1.5f, 0.5f }) * Matrix4::translation(Vector3(1)));
    _fluidSolver->domainBox().upperDomainBound().x() = 2.0f * (1.5f - offset) - s_ParticleRadius;

    /* Run simulation one time step */
    _fluidSolver->advance();
}

/****************************************************************************************************/
} } /* namespace Magnum::Examples  */
