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
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/PixelFormat.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/FunctionsBatch.h>
#include <Magnum/Math/Time.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Circle.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Timeline.h>
#include <Magnum/Trade/MeshData.h>

#include "DrawableObjects/ParticleGroup2D.h"
#include "DrawableObjects/WireframeObject2D.h"
#include "FluidSolver/ApicSolver2D.h"

namespace Magnum { namespace Examples {

class FluidSimulation2DExample: public Platform::Application {
    public:
        explicit FluidSimulation2DExample(const Arguments& arguments);

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

        /* Fluid simulation helper functions */
        void resetSimulation();
        Vector2 windowPos2WorldPos(const Vector2& winPos);

        /* Window control */
        void showMenu();

        bool _showMenu = true;
        ImGuiIntegration::Context _imGuiContext{ NoCreate };

        /* Scene and drawable group must be constructed before camera and other
           drawble objects */
        Containers::Pointer<Scene2D> _scene;
        Containers::Pointer<SceneGraph::DrawableGroup2D> _drawableGroup;

        /* Camera helpers */
        Containers::Pointer<Object2D> _objCamera;
        Containers::Pointer<SceneGraph::Camera2D> _camera;

        /* Fluid simulation system */
        Containers::Pointer<ApicSolver2D> _fluidSolver;
        Containers::Pointer<ParticleGroup2D> _drawableParticles;
        Containers::Pointer<WireframeObject2D> _drawableBoundary;
        Float _speed = 2.0f;
        Float _evolvedTime = 0.0f;
        Int _numEmission = 0.0f;
        bool _bAutoEmitParticles = true;
        bool _pausedSimulation = false;

        /* Mouse-Fluid interaction */
        Containers::Pointer<WireframeObject2D> _drawablePointer;
        Timeline _timeline;
        Vector2 _lastMousePressedWorldPos;
        Float _mouseInteractionRadius = 5.0f;
        Float _mouseInteractionMagnitude = 5.0f;
        bool _bMouseInteraction = true;
};

namespace {

constexpr Float GridCellLength = 1.0f;      /* length of 1 grid cell */
constexpr Vector2i NumGridCells{100, 100};   /* number of cells */
constexpr Vector2 GridStart{-50.0f, -50.0f}; /* lower corner of the grid */
constexpr Int RadiusCircleBoundary = 45; /* radius of the boundary circle */

/* Viewport will display this window */
constexpr Float ProjectionScale = 1.05f;
const Vector2i DomainDisplaySize = NumGridCells*GridCellLength*ProjectionScale;

Vector2 gridCenter() {
    return Vector2{NumGridCells}*GridCellLength*0.5f + GridStart;
}

}

using namespace Math::Literals;

FluidSimulation2DExample::FluidSimulation2DExample(const Arguments& arguments): Platform::Application{arguments, NoCreate} {

    /* Setup window */
    {
        const Vector2 dpiScaling = this->dpiScaling({});
        Configuration conf;
        conf.setTitle("Magnum 2D Fluid Simulation Example")
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
            const_cast<char*>(font.data()), Int(font.size()),
            16.0f*framebufferSize().x()/size.x(), &fontConfig);

        _imGuiContext = ImGuiIntegration::Context{*ImGui::GetCurrentContext(),
            Vector2{windowSize()}/dpiScaling(), windowSize(), framebufferSize()};

        /* Setup proper blending to be used by ImGui */
        GL::Renderer::setBlendFunction(
            GL::Renderer::BlendFunction::SourceAlpha,
            GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    }

    /* Setup scene objects and camera */
    {
        /* Setup scene objects */
        _scene.emplace();
        _drawableGroup.emplace();

        /* Configure camera */
        _objCamera.emplace(_scene.get());
        _objCamera->setTransformation(Matrix3::translation(gridCenter()));

        _camera.emplace(*_objCamera);
        _camera->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
            .setProjectionMatrix(Matrix3::projection(Vector2{DomainDisplaySize}))
            .setViewport(GL::defaultFramebuffer.viewport().size());
    }

    /* Setup fluid solver */
    {
        SceneObjects* sceneObjs = new SceneObjects;
        sceneObjs->emitterT0 = SDFObject{gridCenter() + Vector2(10.0f, 10.0f), 30.0f, SDFObject::ObjectType::Circle};
        sceneObjs->emitter = SDFObject{gridCenter() + Vector2(15.0f, 20.0f), 15.0f, SDFObject::ObjectType::Circle};
        sceneObjs->boundary = SDFObject{gridCenter(), Float(RadiusCircleBoundary), SDFObject::ObjectType::Circle, false};
        _fluidSolver.emplace(GridStart, GridCellLength, NumGridCells.x(), NumGridCells.y(), sceneObjs);

        /* Drawable particles */
        _drawableParticles.emplace(_fluidSolver->particlePositions(),
                                   _fluidSolver->particleRadius());
        _drawableParticles->setColor(0x55c8f5_rgbf);

        /* Drawable boundary*/
        _drawableBoundary.emplace(_scene.get(), _drawableGroup.get(),
            MeshTools::compile(Primitives::circle2DWireframe(128)));
        _drawableBoundary->setTransformation(Matrix3::scaling(Vector2{RadiusCircleBoundary + _fluidSolver->particleRadius()}));
        _drawableBoundary->setColor(0xffffff_rgbf);

        /* Visualize mouse pointer for mouse-fluid interaction */
        _drawablePointer.emplace(_scene.get(), _drawableGroup.get(),
            MeshTools::compile(Primitives::circle2DWireframe(32)));
        _drawablePointer->setColor(0x00ff00_rgbf);
        _drawablePointer->setEnabled(false);
    }

    /* Enable depth test, render particles as sprites */
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::ProgramPointSize);

    /* Start the timer, loop at 60 Hz max */
    setSwapInterval(1);
    setMinimalLoopPeriod(16.0_msec);
    _timeline.start();
}

void FluidSimulation2DExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);
    _imGuiContext.newFrame();

    /* Enable text input, if needed */
    if(ImGui::GetIO().WantTextInput && !isTextInputActive())
        startTextInput();
    else if(!ImGui::GetIO().WantTextInput && isTextInputActive())
        stopTextInput();

    /* Draw objects */
    {
        /* Trigger drawable object to update the particles to the GPU */
        _drawableParticles->setDirty();
        _drawableParticles->draw(_camera, GL::defaultFramebuffer.viewport().size().y(), DomainDisplaySize.y());

        /* Draw other objects (boundary mesh, pointer mesh) */
        _camera->draw(*_drawableGroup);
    }

    if(!_pausedSimulation) {
        constexpr Float frameTime = 1.0f/60.0f;
        /* pause for a while before starting simulation */
        if(_evolvedTime > 1.0f) _fluidSolver->advanceFrame(frameTime*_speed);
        _evolvedTime += frameTime;

        /* Emit particles automatically */
        if(_bAutoEmitParticles && _evolvedTime > 10.0f) {
            static Float lastTime = _evolvedTime;
            if(_evolvedTime - lastTime > 1.5f && /* emit every 1.5 second */
               _numEmission < 5)                 /* emit 5 times */
            {
                _fluidSolver->emitParticles();
                lastTime = _evolvedTime;
                ++_numEmission;
            }
        }
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

    /* Run next frame immediately */
    redraw();
}

void FluidSimulation2DExample::viewportEvent(ViewportEvent& event) {
    /* Resize the main framebuffer */
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    /* Relayout ImGui */
    _imGuiContext.relayout(Vector2{event.windowSize()}/event.dpiScaling(), event.windowSize(), event.framebufferSize());

    /* Recompute the camera's projection matrix */
    _camera->setViewport(event.framebufferSize());
}

void FluidSimulation2DExample::keyPressEvent(KeyEvent& event) {
    switch(event.key()) {
        case Key::E:
            _fluidSolver->emitParticles();
            break;
        case Key::H:
            _showMenu ^= true;
            event.setAccepted(true);
            break;
        case Key::R:
            resetSimulation();
            event.setAccepted(true);
            break;
        case Key::Space:
            _pausedSimulation ^= true;
            event.setAccepted(true);
            break;
        default:
            if(_imGuiContext.handleKeyPressEvent(event))
                event.setAccepted(true);
    }
}

void FluidSimulation2DExample::keyReleaseEvent(KeyEvent& event) {
    if(_imGuiContext.handleKeyReleaseEvent(event)) {
        event.setAccepted(true);
        return;
    }
}

void FluidSimulation2DExample::pointerPressEvent(PointerEvent& event) {
    if(_imGuiContext.handlePointerPressEvent(event)) {
        event.setAccepted(true);
        return;
    }

    if(!event.isPrimary() ||
       !(event.pointer() & (Pointer::MouseLeft|Pointer::Finger)))
        return;

    _lastMousePressedWorldPos = windowPos2WorldPos(event.position());
    if(_bMouseInteraction) {
        _timeline.nextFrame();
        _drawablePointer->setEnabled(true);
        _drawablePointer->setTransformation(
            Matrix3::translation(_lastMousePressedWorldPos)*
            Matrix3::scaling(Vector2{_mouseInteractionRadius}));
        event.setAccepted();
    }
}

void FluidSimulation2DExample::pointerReleaseEvent(PointerEvent& event) {
    if(_imGuiContext.handlePointerReleaseEvent(event))
        event.setAccepted(true);

    if(!event.isPrimary() ||
       !(event.pointer() & (Pointer::MouseLeft|Pointer::Finger)))
        return;

    if(_bMouseInteraction) {
        _drawablePointer->setEnabled(false);
        event.setAccepted();
    }
}

void FluidSimulation2DExample::pointerMoveEvent(PointerMoveEvent& event) {
    if(_imGuiContext.handlePointerMoveEvent(event)) {
        event.setAccepted(true);
        return;
    }

    if(!event.isPrimary() ||
       !(event.pointers() & (Pointer::MouseLeft|Pointer::Finger)))
        return;

    const Vector2 currentPos = windowPos2WorldPos(event.position());
    if(_bMouseInteraction) {
        _timeline.nextFrame();
        const auto dt = _timeline.previousFrameDuration();
        if(dt > 1e-4f) {
            _fluidSolver->addRepulsiveVelocity(_lastMousePressedWorldPos,
                currentPos, dt, _mouseInteractionRadius,
                _mouseInteractionMagnitude*0.01f);
            _drawablePointer->setTransformation(
                Matrix3::translation(currentPos)*
                Matrix3::scaling(Vector2{_mouseInteractionRadius}));
            event.setAccepted();
        }
        _lastMousePressedWorldPos = currentPos;
    }
}

void FluidSimulation2DExample::scrollEvent(ScrollEvent& event) {
    const Float delta = event.offset().y();
    if(Math::abs(delta) < 1.0e-2f)
        return;

    if(_imGuiContext.handleScrollEvent(event)) {
        /* Prevent scrolling the page */
        event.setAccepted();
        return;
    }
}

void FluidSimulation2DExample::textInputEvent(TextInputEvent& event) {
    if(_imGuiContext.handleTextInputEvent(event))
        event.setAccepted(true);
}

void FluidSimulation2DExample::showMenu() {
    ImGui::SetNextWindowPos({ 10.0f, 10.0f }, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowBgAlpha(0.5f);
    ImGui::Begin("Options", nullptr);

    /* General information */
    ImGui::Text("Hide/show menu: H");
    ImGui::Text("Num. particles: %d",   Int(_fluidSolver->numParticles()));
    ImGui::Text("Rendering: %3.2f FPS", Double(ImGui::GetIO().Framerate));
    ImGui::Spacing();

    /* Rendering parameters */
    if(ImGui::TreeNode("Particle Rendering")) {
        ImGui::PushID("Particle Rendering");
        {
            constexpr const char* items[] = { "Uniform", "Ramp by ID" };
            static Int colorMode = 1;
            ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);
            if(ImGui::Combo("Color Mode", &colorMode, items, 2)) {
                _drawableParticles->setColorMode(ParticleSphereShader2D::ColorMode(colorMode));
            }
            ImGui::PopItemWidth();
            if(colorMode == 0) { /* Uniform color */
                static Color3 color = _drawableParticles->color();
                if(ImGui::ColorEdit3("Color", color.data())) {
                    _drawableParticles->setColor(color);
                }
            }
        }
        ImGui::PopID();
        ImGui::TreePop();
    }
    ImGui::Spacing();
    ImGui::Separator();

    /* Simulation parameters */
    if(ImGui::TreeNodeEx("Simulation", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Simulation");
        ImGui::PushItemWidth(ImGui::GetWindowWidth()*0.3f);
        ImGui::InputFloat("Speed", &_speed);
        ImGui::Checkbox("Auto emit particles 5 times", &_bAutoEmitParticles);
        ImGui::PopItemWidth();
        ImGui::BeginGroup();
        ImGui::Checkbox("Mouse interaction", &_bMouseInteraction);
        if(_bMouseInteraction) {
            ImGui::PushItemWidth(ImGui::GetWindowWidth()*0.5f);
            ImGui::SliderFloat("Radius", &_mouseInteractionRadius, 1.0f, 10.0f);
            ImGui::SliderFloat("Magnitude", &_mouseInteractionMagnitude, 1.0f, 10.0f);
            ImGui::PopItemWidth();
        }
        ImGui::EndGroup();
        ImGui::PopID();
        ImGui::TreePop();
    }
    ImGui::Spacing();
    ImGui::Separator();

    /* Reset */
    ImGui::Spacing();
    if(ImGui::Button("Emit Particles")) {
        _fluidSolver->emitParticles();
    }
    ImGui::SameLine();
    if(ImGui::Button(_pausedSimulation ? "Play Sim" : "Pause Sim")) {
        _pausedSimulation ^= true;
    }
    ImGui::SameLine();
    if(ImGui::Button("Reset Sim")) {
        resetSimulation();
    }
    ImGui::End();
}

void FluidSimulation2DExample::resetSimulation() {
    _fluidSolver->reset();
    _pausedSimulation = false;
    _evolvedTime = 0.0f;
    _numEmission = 0;
}

Vector2 FluidSimulation2DExample::windowPos2WorldPos(const Vector2& windowPosition) {
    /* First scale the position from being relative to window size to being
       relative to framebuffer size as those two can be different on HiDPI
       systems */
    const Vector2 position = windowPosition*Vector2{framebufferSize()}/Vector2{windowSize()};

    /* Compute inverted model view projection matrix */
    const Matrix3 invViewProjMat = (_camera->projectionMatrix()*_camera->cameraMatrix()).inverted();

    /* Compute the world coordinate from window coordinate */
    const Vector2 flippedPos{position.x(), framebufferSize().y() - position.y()};
    const Vector2 ndcPos = flippedPos/Vector2(framebufferSize())*2.0f
        - Vector2{1.0f};
    return invViewProjMat.transformPoint(ndcPos);
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::FluidSimulation2DExample)
