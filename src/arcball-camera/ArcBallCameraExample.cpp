/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
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

#include "ArcBall.h"
#include "ArcBallCamera.h"

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Pointer.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/VertexColor.h>

namespace Magnum { namespace Examples {
using Object3D = SceneGraph::Object<SceneGraph::MatrixTransformation3D>;
using Scene3D  = SceneGraph::Scene<SceneGraph::MatrixTransformation3D>;
using namespace Math::Literals;

class ArcBallCameraExample : public Platform::Application {
public:
    explicit ArcBallCameraExample(const Arguments& arguments);

private:
    void drawEvent() override;
    void viewportEvent(ViewportEvent& event) override;
    void keyPressEvent(KeyEvent& event) override;
    void mousePressEvent(MouseEvent& event) override;
    void mouseMoveEvent(MouseMoveEvent& event) override;
    void mouseScrollEvent(MouseScrollEvent& event) override;

    Scene3D                            _scene;
    SceneGraph::DrawableGroup3D        _drawables;
    GL::Mesh                           _mesh { NoCreate };
    Shaders::VertexColor3D             _shader{ NoCreate };
    Containers::Pointer<ArcBallCamera> _arcballCamera;
};

class VertexColorDrawable : public SceneGraph::Drawable3D {
public:
    explicit VertexColorDrawable(Object3D& object, Shaders::VertexColor3D& shader, GL::Mesh& mesh,
                                 SceneGraph::DrawableGroup3D& drawables) :
        SceneGraph::Drawable3D{object, &drawables}, _shader(shader), _mesh(mesh) {}

    void draw(const Matrix4& transformation, SceneGraph::Camera3D& camera) {
        _shader.setTransformationProjectionMatrix(camera.projectionMatrix() * transformation);
        _mesh.draw(_shader);
    }

private:
    Shaders::VertexColor3D& _shader;
    GL::Mesh&               _mesh;
};

ArcBallCameraExample::ArcBallCameraExample(const Arguments& arguments) :
    Platform::Application{arguments, NoCreate} {
    /* Setup window */
    {
        const Vector2 dpiScaling = this->dpiScaling({});
        Configuration conf;
        conf.setTitle("Magnum ArcBall Camera Example")
            .setSize(conf.size(), dpiScaling)
            .setWindowFlags(Configuration::WindowFlag::Resizable);
        GLConfiguration glConf;
        glConf.setSampleCount(dpiScaling.max() < 2.0f ? 8 : 2);
        if(!tryCreate(conf, glConf)) {
            create(conf, glConf.setSampleCount(0));
        }

        SDL_CaptureMouse(SDL_TRUE);
    }

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    /* Setup the cube with vertex color */
    {
        const Containers::Array<Vector3> cubeVertices {
            Containers::InPlaceInit, {
                // front
                { -1.0, -1.0,  1.0 },
                { 1.0, -1.0,  1.0 },
                { 1.0,  1.0,  1.0 },
                { -1.0,  1.0,  1.0 },
                // back
                { -1.0, -1.0, -1.0 },
                { 1.0, -1.0, -1.0 },
                { 1.0,  1.0, -1.0 },
                { -1.0,  1.0, -1.0 }
            } };

        const Containers::Array<Vector3> cubeColors {
            Containers::InPlaceInit, {
                // front colors
                { 1.0, 0.0, 0.0 },
                { 0.0, 1.0, 0.0 },
                { 0.0, 0.0, 1.0 },
                { 1.0, 1.0, 1.0 },
                // back colors
                { 1.0, 0.0, 0.0 },
                { 0.0, 1.0, 0.0 },
                { 0.0, 0.0, 1.0 },
                { 1.0, 1.0, 1.0 }
            } };

        const Containers::Array<char> cubeIndices {
            Containers::InPlaceInit, {
                // front
                0, 1, 2,
                2, 3, 0,
                // right
                1, 5, 6,
                6, 2, 1,
                // back
                7, 6, 5,
                5, 4, 7,
                // left
                4, 0, 3,
                3, 7, 4,
                // bottom
                4, 5, 1,
                1, 0, 4,
                // top
                3, 2, 6,
                6, 7, 3
            } };

        GL::Buffer vertexBuffer;
        GL::Buffer indexBuffer;
        vertexBuffer.setData(MeshTools::interleave(cubeVertices, cubeColors));
        indexBuffer.setData(cubeIndices);

        _mesh = GL::Mesh{};
        _mesh.setCount(cubeIndices.size())
            .addVertexBuffer(std::move(vertexBuffer), 0,
                             Shaders::VertexColor3D::Position{},
                             Shaders::VertexColor3D::Color3{})
            .setIndexBuffer(std::move(indexBuffer), 0, MeshIndexType::UnsignedByte);

        _shader = Shaders::VertexColor3D{};
        new VertexColorDrawable{ *(new Object3D{ &_scene }), _shader, _mesh, _drawables };
    }

    /* Set up the camera */
    {
        /* Setup the arcball after the camera objects */
        const Vector3 eye(0, 0, -10);
        const Vector3 center(0, 0, 0);
        const Vector3 up(0, 1, 0);
        const Deg     fov { 45.0_degf };
        _arcballCamera.reset(new ArcBallCamera(_scene, eye, center, up, fov,
                                               windowSize(), GL::defaultFramebuffer.viewport().size()));
    }

    /* Start the timer, loop at 60 Hz max */
    setSwapInterval(1);
    setMinimalLoopPeriod(16);
}

void ArcBallCameraExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color | GL::FramebufferClear::Depth);

    /* Call arcball update in every frame
     * This will do nothing if the camera has not been changed
     * Otherwise, camera transformation will be propagated into the camera objects
     */

    bool bCamChanged = _arcballCamera->update();
    _arcballCamera->draw(_drawables);
    swapBuffers();
    if(bCamChanged) {
        redraw();
    }
}

void ArcBallCameraExample::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({ {}, event.framebufferSize() });
    _arcballCamera->reshape(windowSize(), GL::defaultFramebuffer.viewport().size());
}

void ArcBallCameraExample::keyPressEvent(KeyEvent& event) {
    switch(event.key()) {
        case KeyEvent::Key::Plus:
        case KeyEvent::Key::NumAdd:
            _arcballCamera->zoom(1.0f);
            break;
        case KeyEvent::Key::Minus:
        case KeyEvent::Key::NumSubtract:
            _arcballCamera->zoom(-1.0f);
            break;
        case KeyEvent::Key::Left:
            _arcballCamera->translateDelta(Vector2{ -0.1f, 0 });
            break;
        case KeyEvent::Key::Right:
            _arcballCamera->translateDelta(Vector2{ 0.1f, 0 });
            break;
        case KeyEvent::Key::Up:
            _arcballCamera->translateDelta(Vector2{ 0, 0.1f });
            break;
        case KeyEvent::Key::Down:
            _arcballCamera->translateDelta(Vector2{ 0, -0.1f });
            break;

        case KeyEvent::Key::L:
            _arcballCamera->setLagging(_arcballCamera->lagging() > 0 ? 0.0f : 0.85f);
            break;
        case KeyEvent::Key::R:
            _arcballCamera->reset();
            break;
        default:
            return;
    }
    event.setAccepted(true);
    redraw(); /* camera has changed, redraw! */
}

void ArcBallCameraExample::mousePressEvent(MouseEvent& event) {
    if(!(event.button() == MouseEvent::Button::Left ||
         event.button() == MouseEvent::Button::Right)) { return; }
    _arcballCamera->initTransformation(event.position());
    event.setAccepted();
}

void ArcBallCameraExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(event.buttons() & MouseMoveEvent::Button::Left) {
        _arcballCamera->rotate(event.position());
    } else if(event.buttons() & MouseMoveEvent::Button::Right) {
        _arcballCamera->translate(event.position());
    } else {
        return;
    }
    event.setAccepted();
    redraw(); /* camera has changed, redraw! */
}

void ArcBallCameraExample::mouseScrollEvent(MouseScrollEvent& event) {
    const Float delta = event.offset().y();
    if(Math::abs(delta) < 1.0e-2f) {
        return;
    }
    _arcballCamera->zoom(delta);
    event.setAccepted();
    redraw(); /* camera has changed, redraw! */
}
} }

MAGNUM_APPLICATION_MAIN(Magnum::Examples::ArcBallCameraExample)
