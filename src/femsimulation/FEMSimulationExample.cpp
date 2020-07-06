/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 —
            Vladimír Vondruš <mosra@centrum.cz>
        2020 — Nghia Truong <nghiatruong.vn@gmail.com>

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

#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Pointer.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Platform/Sdl2Application.h>

#include "../arcball/ArcBallCamera.h"
#include "../fluidsimulation3d/DrawableObjects/WireframeObjects.h"
#include "Simulation/Simulator.h"
#include "Simulation/TetMesh.h"

#include <chrono>
#include <string>

namespace Magnum { namespace Examples {
class Timer {
public:
    Timer() = default;
    void tick() { _startTime = std::chrono::high_resolution_clock::now(); }
    Float tock() {
        return std::chrono::duration<Float, std::milli>(
            std::chrono::high_resolution_clock::now() - _startTime).count();
    }

private:
    std::chrono::high_resolution_clock::time_point _startTime;
};

namespace {
constexpr std::size_t FrameTimeHistory { 30 };
}
using namespace Math::Literals;

class FEMSimulationExample : public Platform::Application {
public:
    explicit FEMSimulationExample(const Arguments& arguments);

private:
    void viewportEvent(ViewportEvent& event) override;
    void drawEvent() override;
    void keyPressEvent(KeyEvent& event) override;
    void keyReleaseEvent(KeyEvent& event) override;
    void mousePressEvent(MouseEvent& event) override;
    void mouseReleaseEvent(MouseEvent& event) override;
    void mouseMoveEvent(MouseMoveEvent& event) override;
    void mouseScrollEvent(MouseScrollEvent& event) override;
    void textInputEvent(TextInputEvent& event) override;

    void setupScene(Int sceneId = 1);
    void resetSimulation();
    void showMenu();

    /* Scene objects */
    Scene3D                            _scene;
    SceneGraph::DrawableGroup3D        _drawables;
    ImGuiIntegration::Context          _ImGuiContext{ NoCreate };
    Containers::Pointer<ArcBallCamera> _camera;
    Containers::Pointer<WireframeGrid> _grid;
    bool _showMenu { true };

    /* Simulation */
    Containers::Pointer<Simulator> _simulator;
    Containers::Pointer<TetMesh>   _mesh;
    std::string                    _status { "Status: Paused" };
    Timer _timer;
    bool  _pause { true };

    /* For plotting frame simulation time */
    Float  _frameTime[FrameTimeHistory];
    Float  _lastFrameTime{ 0 };
    size_t _offset { 0 };
};

FEMSimulationExample::FEMSimulationExample(const Arguments& arguments) : Platform::Application{arguments, NoCreate} {
    /* Setup window */
    const Vector2 sdpi = this->dpiScaling({});
    Configuration conf;
    conf.setTitle("Finite Element Simulation Example")
        .setSize(conf.size(), sdpi)
        .setWindowFlags(Configuration::WindowFlag::Resizable);
    GLConfiguration glConf;
    glConf.setSampleCount(sdpi.max() < 2.0f ? 8 : 2);
    if(!tryCreate(conf, glConf)) {
        create(conf, glConf.setSampleCount(0));
    }
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    setSwapInterval(1);
    setMinimalLoopPeriod(16);

    /* Setup ImGui, load a better font */
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImFontConfig fontConfig;
    fontConfig.FontDataOwnedByAtlas = false;
    const Vector2                     size = Vector2{ windowSize() } / sdpi;
    Utility::Resource                 rs{ "data" };
    Containers::ArrayView<const char> font = rs.getRaw("SourceSansPro-Regular.ttf");
    ImGui::GetIO().Fonts->AddFontFromMemoryTTF(
        const_cast<char*>(font.data()), Int(font.size()), 16.0f * framebufferSize().x() / size.x(), &fontConfig);

    _ImGuiContext = ImGuiIntegration::Context(*ImGui::GetCurrentContext(),
                                              Vector2{ windowSize() } / sdpi, windowSize(), framebufferSize());

    /* Setup proper blending to be used by ImGui */
    GL::Renderer::setBlendEquation(
        GL::Renderer::BlendEquation::Add, GL::Renderer::BlendEquation::Add);
    GL::Renderer::setBlendFunction(
        GL::Renderer::BlendFunction::SourceAlpha,
        GL::Renderer::BlendFunction::OneMinusSourceAlpha);

    /* Setup the wireframe grid as floor */
    _grid.emplace(&_scene, &_drawables);
    _grid->transform(Matrix4::translation(Vector3::yAxis(-30)) * Matrix4::scaling(Vector3{ 30 }));

    /* Setup FEM solver, which also setups camera */
    setupScene();
}

void FEMSimulationExample::viewportEvent(ViewportEvent& event) {
    const auto newBufferSize = event.framebufferSize();

    /* Resize the main framebuffer */
    GL::defaultFramebuffer.setViewport({ {}, newBufferSize });

    /* Resize camera */
    _camera->reshape(event.windowSize(), newBufferSize);

    /* Relayout ImGui */
    _ImGuiContext.relayout(Vector2{ event.windowSize() } / event.dpiScaling(),
                           event.windowSize(), event.framebufferSize());
}

void FEMSimulationExample::drawEvent() {
    /* Run the simulation 1 step */
    if(!_pause) {
        _timer.tick();
        _simulator->advanceStep();
        static UnsignedInt count { 0 };
        ++count;
        if(count == 10) { /* Record time every 10 frames */
            _frameTime[_offset] = _timer.tock();
            _lastFrameTime      = _frameTime[_offset];
            _offset = (_offset + 1) % FrameTimeHistory;
            count   = 0;
        }
        char buff[128];
        sprintf(buff, "Running simulation (t = %.2f (s))", _simulator->_generalParams.time);
        _status = std::string(buff);
    }

    /* Draw frame */
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);
    _ImGuiContext.newFrame();
    /* Enable text input, if needed */
    if(ImGui::GetIO().WantTextInput && !isTextInputActive()) {
        startTextInput();
    } else if(!ImGui::GetIO().WantTextInput && isTextInputActive()) {
        stopTextInput();
    }
    _camera->update();
    _camera->draw(_drawables); /* draw the wireframe grid */
    _mesh->draw(_camera.get(), Vector2{ framebufferSize() });
    if(_showMenu) { showMenu(); }

    /* Update cursor */
    _ImGuiContext.updateApplicationCursor(*this);

    /* Set appropriate states. If you only draw imgui UI, it is sufficient to do this once in the constructor. */
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::ScissorTest);

    _ImGuiContext.drawFrame();

    /* Reset state. Only needed if you want to draw something else with different state next frame. */
    GL::Renderer::disable(GL::Renderer::Feature::ScissorTest);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);

    swapBuffers();
    redraw();
}

void FEMSimulationExample::keyPressEvent(KeyEvent& event) {
    if(_ImGuiContext.handleKeyPressEvent(event)) {
        event.setAccepted(true);
        return;
    }
    switch(event.key()) {
        case KeyEvent::Key::H:
            _showMenu ^= true;
            event.setAccepted(true);
            break;
        case KeyEvent::Key::S:
            if(_camera->lagging() > 0.0f) {
                Debug{} << "Disable camera smooth navigation";
                _camera->setLagging(0.0f);
            } else {
                Debug{} << "Enable camera smooth navigation";
                _camera->setLagging(0.85f);
            }
            break;
        case KeyEvent::Key::R:
            _camera->reset();
            break;
        case KeyEvent::Key::Space:
            _pause ^= true;
            if(_pause) { _status = "Simulation paused"; }
            break;
        case KeyEvent::Key::F5:
            resetSimulation();
            break;
    }
}

void FEMSimulationExample::keyReleaseEvent(KeyEvent& event) {
    if(_ImGuiContext.handleKeyReleaseEvent(event)) {
        event.setAccepted(true);
    }
}

void FEMSimulationExample::mousePressEvent(MouseEvent& event) {
    /* Enable mouse capture so the mouse can drag outside of the window */
    /** @todo replace once https://github.com/mosra/magnum/pull/419 is in */
    SDL_CaptureMouse(SDL_TRUE);

    if(_ImGuiContext.handleMousePressEvent(event)) {
        event.setAccepted(true);
    } else {
        _camera->initTransformation(event.position());
    }
}

void FEMSimulationExample::mouseReleaseEvent(MouseEvent& event) {
    /* Disable mouse capture again */
    /** @todo replace once https://github.com/mosra/magnum/pull/419 is in */
    SDL_CaptureMouse(SDL_FALSE);

    if(_ImGuiContext.handleMouseReleaseEvent(event)) {
        event.setAccepted(true);
    }
}

void FEMSimulationExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(_ImGuiContext.handleMouseMoveEvent(event)) {
        event.setAccepted(true);
        return;
    }
    if(!event.buttons()) { return; }
    if(event.modifiers() & MouseMoveEvent::Modifier::Shift) {
        _camera->translate(event.position());
    } else { _camera->rotate(event.position()); }
    event.setAccepted();
}

void FEMSimulationExample::mouseScrollEvent(MouseScrollEvent& event) {
    if(_ImGuiContext.handleMouseScrollEvent(event)) {
        /* Prevent scrolling the page */
        event.setAccepted(true);
        return;
    }
    const Float delta = event.offset().y();
    if(std::abs(delta) < 1.0e-2f) { return; }
    _camera->zoom(delta * 5); /* zooming speed in this application is faster due to mesh large scale */
    event.setAccepted();
}

void FEMSimulationExample::textInputEvent(TextInputEvent& event) {
    if(_ImGuiContext.handleTextInputEvent(event)) {
        event.setAccepted(true);
    }
}

void FEMSimulationExample::setupScene(Int sceneId) {
    if(sceneId == 0) {
        /* Configure camera */
        const Vector3 eye = Vector3{ -40, 20, 65 };
        const Vector3 viewCenter { -10, -7, 0 };
        const Vector3 up  = Vector3::yAxis();
        const Deg     fov = 45.0_degf;
        _camera.emplace(_scene, eye, viewCenter, up, fov, windowSize(), framebufferSize(), 0.01f, 1000.0f);
        _camera->setLagging(0.85f);

        _mesh.emplace("longbar.mesh");

        /* Setup fixed vertices */
        /* Find the maximum x value */
        Float max_x = -1e10f;
        for(UnsignedInt idx = 0; idx < _mesh->_numVerts; ++idx) {
            const EgVec3f& v = _mesh->_positions_t0.block3(idx);
            if(max_x < v.x()) { max_x = v.x(); }
        }

        /* Fix the vertices that have x ~~ max_x */
        for(UnsignedInt idx = 0; idx < _mesh->_numVerts; ++idx) {
            const EgVec3f& v = _mesh->_positions_t0.block3(idx);
            if(std::abs(max_x - v.x()) < 1e-4f) {
                arrayAppend(_mesh->_fixedVerts, idx);
            }
        }
    } else {
        /* Configure camera */
        const Vector3 eye = Vector3{ -20, 0, 65 };
        const Vector3 viewCenter { -10, -3, 0 };
        const Vector3 up  = Vector3::yAxis();
        const Deg     fov = 45.0_degf;
        _camera.emplace(_scene, eye, viewCenter, up, fov, windowSize(), framebufferSize(), 0.01f, 1000.0f);
        _camera->setLagging(0.85f);

        _mesh.emplace("squirrel.mesh");

        /*
         * Transform the squirrel mesh, since the original mesh is small
         * Firstly compute the squirrel bounding box, then rescale/translate
         */
        EgVec3f lower(1e10, 1e10, 1e10);
        EgVec3f upper(-1e10, -1e10, -1e10);
        for(UnsignedInt idx = 0; idx < _mesh->_numVerts; ++idx) {
            const EgVec3f& v = _mesh->_positions_t0.block3(idx);
            for(Int i = 0; i < 3; ++i) {
                if(lower[i] > v[i]) { lower[i] = v[i]; }
                if(upper[i] < v[i]) { upper[i] = v[i]; }
            }
        }

        const EgVec3f center  = (lower + upper) * 0.5f;
        const Float   maxSize = Math::max(Math::max(upper[0] - lower[0],
                                                    upper[1] - lower[1]),
                                          upper[2] - lower[2]);
        const Float scaling = 25.0f / maxSize;

        /* Fix the vertices that have y near max_y */
        for(UnsignedInt idx = 0; idx < _mesh->_numVerts; ++idx) {
            const EgVec3f& v = _mesh->_positions_t0.block3(idx);
            if(std::abs(v.y() - upper.y()) < 0.05f * maxSize) {
                arrayAppend(_mesh->_fixedVerts, idx);
            }
        }

        /* Transform the mesh */
        for(UnsignedInt idx = 0; idx < _mesh->_numVerts; ++idx) {
            EgVec3f v = _mesh->_positions_t0.block3(idx);
            v = (v - center) * scaling;
            _mesh->_positions_t0.block3(idx) = v;
        }
    }
    _mesh->reset();
    _simulator.emplace(_mesh.get());

    /* The default simulation parameters are turned to work well with the long bar
     * Thus we need to change the simulation paramteres for the squirrel */
    if(sceneId == 1) {
        _simulator->_generalParams.dt       = 0.02f;
        _simulator->_generalParams.subSteps = 3;
        _simulator->_generalParams.damping  = 0.003f;
        _simulator->_windParams.magnitude   = 7;
        _simulator->_materialParams.type    = FEMConstraint::Material::StVK;
        _simulator->_materialParams.mu      = 3;
        _simulator->_materialParams.lambda  = 3;
        _simulator->updateConstraintParameters();
    }
}

void FEMSimulationExample::resetSimulation() {
    _pause  = true;
    _status = "Simulation paused";
    _simulator->reset();
}

void FEMSimulationExample::showMenu() {
    ImGui::SetNextWindowBgAlpha(0.5f);
    ImGui::Begin("Options", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("Hide/show menu: H");
    ImGui::Text("%3.2f FPS", static_cast<double>(ImGui::GetIO().Framerate));
    ImGui::Spacing();
    ImGui::PushItemWidth(120);
    if(ImGui::CollapsingHeader("General Simulation Parameters")) {
        ImGui::PushID("General-Parameters");
        ImGui::InputFloat("Damping", &_simulator->_generalParams.damping, 0.0f, 0.0f, "%.6g");
        if(ImGui::InputFloat("Attachement Stiffness", &_simulator->_generalParams.attachmentStiffness)) {
            _simulator->updateConstraintParameters();
        }
        ImGui::Spacing();
        ImGui::InputFloat("Frame Time", &_simulator->_generalParams.dt, 0.0f, 0.0f, "%.6g");
        ImGui::InputInt("Substeps", &_simulator->_generalParams.subSteps);
        ImGui::PopID();
    }
    ImGui::Spacing();

    if(ImGui::CollapsingHeader("Wind")) {
        ImGui::PushID("Wind");
        ImGui::Checkbox("Enable", &_simulator->_windParams.enable);
        ImGui::InputFloat("Time enable", &_simulator->_windParams.timeEnable);
        ImGui::InputFloat("Magnitude",   &_simulator->_windParams.magnitude);
        ImGui::InputFloat("Frequency",   &_simulator->_windParams.frequency);
        ImGui::PopID();
    }
    ImGui::Spacing();

    if(ImGui::CollapsingHeader("FEM Material")) {
        ImGui::PushID("FEM-Material");
        const char* items[] = { "Corotational", "StVK", "NeoHookean-ExtendLog" };
        ImGui::PopItemWidth();
        ImGui::PushItemWidth(200);
        if(ImGui::Combo("Material", &_simulator->_materialParams.type, items, IM_ARRAYSIZE(items))) {
            _simulator->updateConstraintParameters();
        }
        ImGui::PopItemWidth();
        ImGui::PushItemWidth(120);
        if(ImGui::InputFloat("mu", &_simulator->_materialParams.mu)
           || ImGui::InputFloat("lambda", &_simulator->_materialParams.lambda)
           || (_simulator->_materialParams.type == FEMConstraint::Material::StVK
               && ImGui::InputFloat("kappa", &_simulator->_materialParams.kappa))) {
            _simulator->updateConstraintParameters();
        }
        ImGui::PopID();
    }

    ImGui::Spacing();
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(200);

    if(ImGui::CollapsingHeader("Timing", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID("Timing");
        char buff[32];
        sprintf(buff, "Frame Time:\n%5.2f (ms)", _lastFrameTime);
        Float minVal = 1e10;
        Float maxVal = -1e10;
        for(size_t i = 0; i < FrameTimeHistory; ++i) {
            if(minVal > _frameTime[i]) { minVal = _frameTime[i]; }
            if(maxVal < _frameTime[i]) { maxVal = _frameTime[i]; }
        }
        ImGui::PlotLines(buff, _frameTime, IM_ARRAYSIZE(_frameTime), _offset, "",
                         minVal - 0.1f, maxVal + 0.1f, ImVec2(0, 80.0f));
        ImGui::PopID();
    }

    ImGui::Text(_status.c_str());
    ImGui::Separator();

    const char* items[] = { "Long Bar", "Squirrel Head" };
    static int  sceneId { 1 };
    if(ImGui::Combo("Scene", &sceneId, items, IM_ARRAYSIZE(items))) {
        _pause = true;
        setupScene(sceneId);
    }
    ImGui::ColorEdit3("Mesh Color", _mesh->_meshColor.data());
    ImGui::Separator();

    if(ImGui::Button("Reset Camera")) { _camera->reset(); }
    ImGui::SameLine();
    if(ImGui::Button("Reset Simulation")) { resetSimulation(); }
    ImGui::SameLine();
    if((_pause && ImGui::Button("Resume Simulation"))
       || (!_pause && ImGui::Button("Pause Simulation"))) {
        _pause ^= true;
        if(_pause) { _status = "Status: Paused"; }
    }
    ImGui::PopItemWidth();
}
} }

MAGNUM_APPLICATION_MAIN(Magnum::Examples::FEMSimulationExample)
