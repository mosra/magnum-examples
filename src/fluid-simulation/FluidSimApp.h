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

#pragma once

#include <Corrade/Containers/Pointer.h>
#include <Magnum/ImGuiIntegration/Context.hpp>
#include <Magnum/Math/Color.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/Timeline.h>

namespace Magnum { namespace Examples {
/****************************************************************************************************/
using Object3D = SceneGraph::Object<SceneGraph::MatrixTransformation3D>;
using Scene3D  = SceneGraph::Scene<SceneGraph::MatrixTransformation3D>;

class SPHSolver;
class WireframeGrid;
class WireframeBox;
class ParticleGroup;

/****************************************************************************************************/
class FluidSimApp final : public Platform::Application {
public:
    explicit FluidSimApp(const Arguments& arguments);

protected:
    void viewportEvent(ViewportEvent& event) override;
    void keyPressEvent(KeyEvent& event) override;
    void keyReleaseEvent(KeyEvent& event) override;
    void mousePressEvent(MouseEvent& event) override;
    void mouseReleaseEvent(MouseEvent& event) override;
    void mouseMoveEvent(MouseMoveEvent& event) override;
    void mouseScrollEvent(MouseScrollEvent& event) override;
    void textInputEvent(TextInputEvent& event) override;
    void drawEvent() override;

    /* Helper functions for camera movement */
    float   depthAt(const Vector2i& windowPosition);
    Vector3 unproject(const Vector2i& windowPosition, float depth) const;

    /* Fluid simulation helper functions */
    void showMenu();
    void initializeScene();
    void simulationStep();

    /* Window control */
    bool _bShowMenu { true };
    ImGuiIntegration::Context m_ImGuiContext{ NoCreate };

    /* Scene and drawable group must be constructed before camera and other scene objects */
    Containers::Pointer<Scene3D>                     _scene;
    Containers::Pointer<SceneGraph::DrawableGroup3D> _drawableGroup;

    /* Camera helpers */
    Vector3  _defaultCamPosition { 0.0f, 1.5f, 8.0f };
    Vector3  _defaultCamTarget { 0.0f, 1.0f, 0.0f };
    Vector2i _prevMousePosition;
    Vector3  _rotationPoint, _translationPoint;
    float    _lastDepth;
    Containers::Pointer<Object3D>             _objCamera;
    Containers::Pointer<SceneGraph::Camera3D> _camera;

    /* Fluid simulation system */
    Containers::Pointer<SPHSolver>    _fluidSolver { nullptr };
    Containers::Pointer<WireframeBox> _drawableBox;
    int   _substeps { 1 };
    bool  _bPausedSimulation { false };
    bool  _bMousePressed { false };
    bool  _bDynamicBoundary { true };
    float _boundaryOffset { 0.0f }; /* For boundary animation */

    /* Drawable particles */
    Containers::Pointer<ParticleGroup> _drawableParticles { nullptr };

    /* Ground grid */
    Containers::Pointer<WireframeGrid> _grid;

    /* Timeline to adjust number of simulation steps per frame */
    Timeline _timeline;
};

/****************************************************************************************************/
} } /* namespace Magnum::Examples  */

MAGNUM_APPLICATION_MAIN(Magnum::Examples::FluidSimApp)
