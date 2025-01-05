/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>
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

#include <Corrade/Containers/Pointer.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/Utility/StlMath.h>
#include <Magnum/Image.h>
#include <Magnum/Timeline.h>
#include <Magnum/Animation/Easing.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/PixelFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/FunctionsBatch.h>
#include <Magnum/Math/Time.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Version.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>

#include "DrawableObjects/ParticleGroup.h"
#include "DrawableObjects/WireframeObjects.h"
#include "SPH/SPHSolver.h"

#include "configure.h"

#ifdef MAGNUM_FLUIDSIMULATION3D_EXAMPLE_USE_MULTITHREADING
#include <thread>
#endif

namespace Magnum { namespace Examples {

class FluidSimulation3DExample: public Platform::Application {
    public:
        explicit FluidSimulation3DExample(const Arguments& arguments);

    protected:
        void viewportEvent(ViewportEvent& event) override;
        void keyPressEvent(KeyEvent& event) override;
        void keyReleaseEvent(KeyEvent& event) override;
        void pointerPressEvent(PointerEvent& event) override;
        void pointerReleaseEvent(PointerEvent& event) override;
        void pointerMoveEvent(PointerMoveEvent& event) override;
        void scrollEvent(ScrollEvent& event) override;
        void textInputEvent(TextInputEvent& event) override;
        void drawEvent() override;

        /* Helper functions for camera movement */
        Float depthAt(const Vector2& windowPosition);
        Vector3 unproject(const Vector2& windowPosition, Float depth) const;

        /* Fluid simulation helper functions */
        void showMenu();
        void initializeScene();
        void simulationStep();

        /* Window control */
        bool _showMenu = true;
        ImGuiIntegration::Context _imGuiContext{NoCreate};

        /* Scene and drawable group must be constructed before camera and other
        scene objects */
        Containers::Pointer<Scene3D> _scene;
        Containers::Pointer<SceneGraph::DrawableGroup3D> _drawableGroup;

        /* Camera helpers */
        Vector3 _defaultCamPosition{0.0f, 1.5f, 8.0f};
        Vector3 _defaultCamTarget{0.0f, 1.0f, 0.0f};
        Vector2 _prevPointerPosition;
        Vector3  _rotationPoint, _translationPoint;
        Float _lastDepth;
        Containers::Pointer<Object3D> _objCamera;
        Containers::Pointer<SceneGraph::Camera3D> _camera;

        /* Fluid simulation system */
        Containers::Pointer<SPHSolver> _fluidSolver;
        Containers::Pointer<WireframeBox> _drawableBox;
        Int _substeps = 1;
        bool _pausedSimulation = false;
        bool _mousePressed = false;
        bool _dynamicBoundary = true;
        Float _boundaryOffset = 0.0f; /* For boundary animation */

        /* Drawable particles */
        Containers::Pointer<ParticleGroup> _drawableParticles;

        /* Ground grid */
        Containers::Pointer<WireframeGrid> _grid;

        /* Timeline to adjust number of simulation steps per frame */
        Timeline _timeline;
};

using namespace Math::Literals;

namespace {
    constexpr Float ParticleRadius = 0.02f;
}

FluidSimulation3DExample::FluidSimulation3DExample(const Arguments& arguments): Platform::Application{arguments, NoCreate} {
    /* Setup window */
    {
        const Vector2 dpiScaling = this->dpiScaling({});
        Configuration conf;
        conf.setTitle("Magnum 3D Fluid Simulation Example")
            .setSize(conf.size(), dpiScaling)
            .setWindowFlags(Configuration::WindowFlag::Resizable);
        GLConfiguration glConf;
        glConf.setSampleCount(dpiScaling.max() < 2.0f ? 8 : 2);
        if(!tryCreate(conf, glConf)) {
            create(conf, glConf.setSampleCount(0));
        }
    }

    /* Setup ImGui, load a better font */
    {
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImFontConfig fontConfig;
        fontConfig.FontDataOwnedByAtlas = false;
        const Vector2 size = Vector2{windowSize()}/dpiScaling();
        Utility::Resource rs{"data"};
        Containers::ArrayView<const char> font = rs.getRaw("SourceSansPro-Regular.ttf");
        ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
            const_cast<char*>(font.data()), Int(font.size()), 16.0f*framebufferSize().x()/size.x(), &fontConfig);

        _imGuiContext = ImGuiIntegration::Context(*ImGui::GetCurrentContext(),
            Vector2{windowSize()}/dpiScaling(), windowSize(), framebufferSize());

        /* Setup proper blending to be used by ImGui */
        GL::Renderer::setBlendEquation(
            GL::Renderer::BlendEquation::Add, GL::Renderer::BlendEquation::Add);
        GL::Renderer::setBlendFunction(
            GL::Renderer::BlendFunction::SourceAlpha,
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
        _fluidSolver.reset(new SPHSolver{ParticleRadius});

        /* Simulation domain box */
        /* Transform the box to cover the region [0, 0, 0] to [3, 3, 1] */
        _drawableBox.reset(new WireframeBox(_scene.get(), _drawableGroup.get()));
        _drawableBox->transform(Matrix4::scaling(Vector3{ 1.5, 1.5, 0.5 }) * Matrix4::translation(Vector3(1)));
        _drawableBox->setColor(Color3(1, 1, 0));

        /* Drawable particles */
        _drawableParticles.reset(new ParticleGroup{_fluidSolver->particlePositions(), ParticleRadius});

        /* Initialize scene particles */
        initializeScene();
    }

    /* Enable depth test, render particles as sprites */
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::ProgramPointSize);

    /* Start the timer, loop at 60 Hz max */
    setSwapInterval(1);
    setMinimalLoopPeriod(16.0_msec);
    _timeline.start();
}

void FluidSimulation3DExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);
    _imGuiContext.newFrame();

    /* Enable text input, if needed */
    if(ImGui::GetIO().WantTextInput && !isTextInputActive()) {
        startTextInput();
    } else if(!ImGui::GetIO().WantTextInput && isTextInputActive()) {
        stopTextInput();
    }

    /* Pause simulation if the mouse was pressed (camera is moving around).
       This avoid freezing GUI while running the simulation */
    if(!_pausedSimulation && !_mousePressed) {
        /* Adjust the substep number to maximize CPU usage each frame */
        const Float lastAvgStepTime = _timeline.previousFrameDuration()/Float(_substeps);
        const Int newSubsteps = lastAvgStepTime > 0 ? Int(1.0f/60.0f/lastAvgStepTime) + 1 : 1;
        if(Math::abs(newSubsteps - _substeps) > 1) _substeps = newSubsteps;

        for(Int i = 0; i < _substeps; ++i) simulationStep();
    }

    /* Draw objects */
    {
        /* Trigger drawable object to update the particles to the GPU */
        _drawableParticles->setDirty();
        /* Draw particles */
        _drawableParticles->draw(_camera, framebufferSize());

        /* Draw other objects (ground grid) */
        _camera->draw(*_drawableGroup);
    }

    /* Menu for parameters */
    if(_showMenu) showMenu();

    /* Update application cursor */
    _imGuiContext.updateApplicationCursor(*this);

    /* Render ImGui window */
    {
        GL::Renderer::enable(GL::Renderer::Feature::Blending);
        GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
        GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
        GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);

        _imGuiContext.drawFrame();

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

void FluidSimulation3DExample::viewportEvent(ViewportEvent& event) {
    /* Resize the main framebuffer */
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    /* Relayout ImGui */
    _imGuiContext.relayout(Vector2{event.windowSize()}/event.dpiScaling(), event.windowSize(), event.framebufferSize());

    /* Recompute the camera's projection matrix */
    _camera->setViewport(event.framebufferSize());
}

void FluidSimulation3DExample::keyPressEvent(KeyEvent& event) {
    switch(event.key()) {
        case Key::H:
            _showMenu ^= true;
            event.setAccepted(true);
            break;
        case Key::R:
            initializeScene();
            event.setAccepted(true);
            break;
        case Key::Space:
            _pausedSimulation ^= true;
            event.setAccepted(true);
            break;
        default:
            if(_imGuiContext.handleKeyPressEvent(event)) {
                event.setAccepted(true);
            }
    }
}

void FluidSimulation3DExample::keyReleaseEvent(KeyEvent& event) {
    if(_imGuiContext.handleKeyReleaseEvent(event)) {
        event.setAccepted(true);
        return;
    }
}

void FluidSimulation3DExample::pointerPressEvent(PointerEvent& event) {
    if(_imGuiContext.handlePointerPressEvent(event)) {
        event.setAccepted(true);
        return;
    }

    if(!event.isPrimary() ||
       !(event.pointer() & (Pointer::MouseLeft|Pointer::Finger)))
        return;

    /* Update camera */
    {
        _prevPointerPosition = event.position();
        const Float currentDepth = depthAt(event.position());
        const Float depth = currentDepth == 1.0f ? _lastDepth : currentDepth;
        _translationPoint = unproject(event.position(), depth);

        /* Update the rotation point only if we're not zooming against infinite
           depth or if the original rotation point is not yet initialized */
        if(currentDepth != 1.0f || _rotationPoint.isZero()) {
            _rotationPoint = _translationPoint;
            _lastDepth = depth;
        }
    }

    _mousePressed = true;
}

void FluidSimulation3DExample::pointerReleaseEvent(PointerEvent& event) {
    _mousePressed = false;

    if(_imGuiContext.handlePointerReleaseEvent(event)) {
        event.setAccepted(true);
    }
}

void FluidSimulation3DExample::pointerMoveEvent(PointerMoveEvent& event) {
    if(_imGuiContext.handlePointerMoveEvent(event)) {
        event.setAccepted(true);
        return;
    }

    if(!event.isPrimary() ||
       !(event.pointers() & (Pointer::MouseLeft|Pointer::Finger)))
        return;

    const Vector2 delta = 3.0f*Vector2{event.position() - _prevPointerPosition}/Vector2{framebufferSize()};
    _prevPointerPosition = event.position();

    /* Translate */
    if(event.modifiers() & Modifier::Shift) {
        const Vector3 p = unproject(event.position(), _lastDepth);
        _objCamera->translateLocal(_translationPoint - p); /* is Z always 0? */
        _translationPoint = p;

    /* Rotate around rotation point */
    } else {
        _objCamera->transformLocal(
            Matrix4::translation(_rotationPoint)*
            Matrix4::rotationX(-0.51_radf*delta.y())*
            Matrix4::rotationY(-0.51_radf*delta.x())*
            Matrix4::translation(-_rotationPoint));
    }

    event.setAccepted();
}

void FluidSimulation3DExample::scrollEvent(ScrollEvent& event) {
    const Float delta = event.offset().y();
    if(Math::abs(delta) < 1.0e-2f) {
        return;
    }

    if(_imGuiContext.handleScrollEvent(event)) {
        /* Prevent scrolling the page */
        event.setAccepted();
        return;
    }

    const Float currentDepth = depthAt(event.position());
    const Float depth = currentDepth == 1.0f ? _lastDepth : currentDepth;
    const Vector3 p = unproject(event.position(), depth);
    /* Update the rotation point only if we're not zooming against infinite
       depth or if the original rotation point is not yet initialized */
    if(currentDepth != 1.0f || _rotationPoint.isZero()) {
        _rotationPoint = p;
        _lastDepth = depth;
    }

    /* Move towards/backwards the rotation point in cam coords */
    _objCamera->translateLocal(_rotationPoint * delta * 0.1f);
}

void FluidSimulation3DExample::textInputEvent(TextInputEvent& event) {
    if(_imGuiContext.handleTextInputEvent(event)) {
        event.setAccepted(true);
    }
}

Float FluidSimulation3DExample::depthAt(const Vector2& windowPosition) {
    /* First scale the position from being relative to window size to being
       relative to framebuffer size as those two can be different on HiDPI
       systems */
    const Vector2i position = windowPosition*framebufferSize()/Vector2{windowSize()};
    const Vector2i fbPosition{position.x(), GL::defaultFramebuffer.viewport().sizeY() - position.y() - 1};

    GL::defaultFramebuffer.mapForRead(GL::DefaultFramebuffer::ReadAttachment::Front);
    Image2D data = GL::defaultFramebuffer.read(
        Range2Di::fromSize(fbPosition, Vector2i{1}).padded(Vector2i{2}),
        {GL::PixelFormat::DepthComponent, GL::PixelType::Float});

    return Math::min<Float>(Containers::arrayCast<const Float>(data.data()));
}

Vector3 FluidSimulation3DExample::unproject(const Vector2& windowPosition, float depth) const {
    /* We have to take window size, not framebuffer size, since the position is
       in window coordinates and the two can be different on HiDPI systems */
    const Vector2i viewSize = windowSize();
    const Vector2 viewPosition{windowPosition.x(), viewSize.y() - windowPosition.y() - 1};
    const Vector3 in{2.0f*viewPosition/Vector2{viewSize} - Vector2{1.0f}, depth*2.0f - 1.0f};

    return _camera->projectionMatrix().inverted().transformPoint(in);
}

void FluidSimulation3DExample::showMenu() {
    ImGui::SetNextWindowPos({500.0f, 50.0f}, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.5f);
    ImGui::Begin("Options", nullptr);
    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.6f);

    /* General information */
    ImGui::Text("Hide/show menu: H");
    ImGui::Text("Num. particles: %d", Int(_fluidSolver->numParticles()));
    ImGui::Text("Simulation steps/frame: %d", _substeps);
    #ifndef MAGNUM_FLUIDSIMULATION3D_EXAMPLE_USE_MULTITHREADING
    ImGui::Text("Rendering: %3.2f FPS (1 thread)", Double(ImGui::GetIO().Framerate));
    #else
    ImGui::Text("Rendering: %3.2f FPS (%d threads"
        #ifdef MAGNUM_FLUIDSIMULATION3D_EXAMPLE_USE_TBB
        " + TBB"
        #endif
        ")", Double(ImGui::GetIO().Framerate), Int(std::thread::hardware_concurrency()));
    #endif
    ImGui::Spacing();

    /* Rendering parameters */
    if(ImGui::TreeNodeEx("Particle Rendering", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Particle Rendering");
        {
            constexpr const char* items[] = {"Uniform", "Ramp by ID", "Random"};
            static Int colorMode = 1;
            if(ImGui::Combo("Color Mode", &colorMode, items, 3))
                _drawableParticles->setColorMode(ParticleSphereShader::ColorMode(colorMode));
            if(colorMode == 0) { /* Uniform color */
                static Color3 color = _drawableParticles->diffuseColor();
                if(ImGui::ColorEdit3("Diffuse Color", color.data())) {
                    _drawableParticles->setDiffuseColor(color);
                }
            }
        }
        static Vector3 lightDir = _drawableParticles->lightDirection();
        if(ImGui::InputFloat3("Light Direction", lightDir.data())) {
            _drawableParticles->setLightDirection(lightDir);
        }
        ImGui::PopID();
        ImGui::TreePop();
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    /* Simulation parameters */
    if(ImGui::TreeNodeEx("Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Simulation");
        ImGui::InputFloat("Stiffness", &_fluidSolver->simulationParameters().stiffness);
        ImGui::SliderFloat("Viscosity",   &_fluidSolver->simulationParameters().viscosity,           0.0f, 1.0f);
        ImGui::SliderFloat("Restitution", &_fluidSolver->simulationParameters().boundaryRestitution, 0.0f, 1.0f);
        ImGui::Checkbox("Dynamic Boundary", &_dynamicBoundary);
        ImGui::PopID();
        ImGui::TreePop();
    }
    ImGui::Spacing();
    ImGui::Separator();

    /* Reset */
    ImGui::Spacing();
    if(ImGui::Button(_pausedSimulation ? "Play Sim" : "Pause Sim"))
        _pausedSimulation ^= true;
    ImGui::SameLine();
    if(ImGui::Button("Reset Sim")) {
        _pausedSimulation = false;
        initializeScene();
    }
    ImGui::SameLine();
    if(ImGui::Button("Reset Camera")) {
        _objCamera->setTransformation(Matrix4::lookAt(_defaultCamPosition, _defaultCamTarget, Vector3(0, 1, 0)));
    }
    ImGui::PopItemWidth();
    ImGui::End();
}

void FluidSimulation3DExample::initializeScene() {
    if(_fluidSolver->numParticles() > 0) {
        _fluidSolver->reset();
    } else {
        const Vector3 lowerCorner = Vector3{ParticleRadius*2.0f};
        const Vector3 upperCorner = Vector3{0.5f, 2.0f, 1.0f} - Vector3{ParticleRadius*2.0f};
        const Float spacing = ParticleRadius * 2.0f;
        const Vector3 resolution = (upperCorner - lowerCorner)/spacing;

        std::vector<Vector3> tmp;
        tmp.reserve(std::size_t(resolution.product()));
        for(Int i = 0; i < resolution[0]; ++i) {
            for(Int j = 0; j < resolution[1]; ++j) {
                for(Int k = 0; k < resolution[2]; ++k) {
                    tmp.push_back(Vector3{Vector3i{i, j, k}}*spacing + lowerCorner);
                }
            }
        }

        _fluidSolver->setPositions(tmp);
    }

    /* Reset domain */
    if(_dynamicBoundary) _boundaryOffset = 0.0f;
    _drawableBox->setTransformation(
        Matrix4::scaling(Vector3{1.5f, 1.5f, 0.5f})*
        Matrix4::translation(Vector3(1)));
    _fluidSolver->domainBox().upperDomainBound().x() = 3.0f - ParticleRadius;

    /* Trigger drawable object to upload particles to the GPU */
    _drawableParticles->setDirty();
}

void FluidSimulation3DExample::simulationStep() {
    static Float offset = 0.0f;
    if(_dynamicBoundary) {
        /* Change fluid boundary */
        static Float step = 2.0e-3f;
        if(_boundaryOffset > 1.0f || _boundaryOffset < 0.0f) {
            step *= -1.0f;
        }
        _boundaryOffset += step;
        offset = Math::lerp(0.0f, 0.5f, Animation::Easing::quadraticInOut(_boundaryOffset));
    }

    _drawableBox->setTransformation(
        Matrix4::scaling(Vector3{1.5f - offset, 1.5f, 0.5f})* Matrix4::translation(Vector3{1.0f}));
    _fluidSolver->domainBox().upperDomainBound().x() = 2.0f*(1.5f - offset) - ParticleRadius;

    /* Run simulation one time step */
    _fluidSolver->advance();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::FluidSimulation3DExample)
