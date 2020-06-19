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

#include <Corrade/Containers/Optional.h>
#include <Magnum/DebugTools/ColorMap.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/MeshVisualizer.h>
#include <Magnum/Trade/MeshData.h>

#include "ArcBall.h"
#include "ArcBallCamera.h"

namespace Magnum { namespace Examples {

using Object3D = SceneGraph::Object<SceneGraph::MatrixTransformation3D>;
using Scene3D = SceneGraph::Scene<SceneGraph::MatrixTransformation3D>;

using namespace Math::Literals;

class ArcBallExample: public Platform::Application {
    public:
        explicit ArcBallExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void viewportEvent(ViewportEvent& event) override;
        void keyPressEvent(KeyEvent& event) override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void mouseScrollEvent(MouseScrollEvent& event) override;

        Scene3D _scene;
        SceneGraph::DrawableGroup3D _drawables;
        GL::Mesh _mesh{NoCreate};
        Containers::Optional<ArcBallCamera> _arcballCamera;

        /* Stuff for visualizing the cube */
        Shaders::MeshVisualizer3D _shader{NoCreate};
        GL::Texture2D _colormap{NoCreate};
};

class VisualizationDrawable: public SceneGraph::Drawable3D {
    public:
        explicit VisualizationDrawable(Object3D& object,
            Shaders::MeshVisualizer3D& shader, GL::Mesh& mesh,
            SceneGraph::DrawableGroup3D& drawables):
                SceneGraph::Drawable3D{object, &drawables}, _shader(shader),
                _mesh(mesh) {}

        void draw(const Matrix4& transformation, SceneGraph::Camera3D& camera) {
            _shader
                .setTransformationMatrix(transformation)
                .setProjectionMatrix(camera.projectionMatrix())
                .draw(_mesh);
        }

    private:
        Shaders::MeshVisualizer3D& _shader;
        GL::Mesh& _mesh;
};

ArcBallExample::ArcBallExample(const Arguments& arguments) :
    Platform::Application{arguments, NoCreate}
{
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
    }

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    /* Setup a cube with vertex ID and wireframe visualized */
    {
        const Trade::MeshData cube = Primitives::cubeSolid();
        _mesh = MeshTools::compile(cube);

        const auto map = DebugTools::ColorMap::turbo();
        const Vector2i size{Int(map.size()), 1};
        _colormap = GL::Texture2D{};
        _colormap
            .setMinificationFilter(SamplerFilter::Linear)
            .setMagnificationFilter(SamplerFilter::Linear)
            .setWrapping(SamplerWrapping::ClampToEdge)
            .setStorage(1, GL::TextureFormat::RGB8, size)
            .setSubImage(0, {}, ImageView2D{PixelFormat::RGB8Unorm, size, map});

        _shader = Shaders::MeshVisualizer3D{
            Shaders::MeshVisualizer3D::Flag::Wireframe|
            Shaders::MeshVisualizer3D::Flag::VertexId};
        _shader
            .setViewportSize(Vector2{framebufferSize()})
            .setColor(0xffffff_rgbf)
            .setWireframeColor(0xffffff_rgbf)
            .setWireframeWidth(2.0f)
            .setColorMapTransformation(0.0f, 1.0f/cube.vertexCount())
            .bindColorMapTexture(_colormap);

        auto object = new Object3D{&_scene};
        (*object)
            .rotateY(40.0_degf)
            .rotateX(-30.0_degf)
            ;
        new VisualizationDrawable{*object, _shader, _mesh, _drawables};
    }

    /* Set up the camera */
    {
        /* Setup the arcball after the camera objects */
        const Vector3 eye = Vector3::zAxis(-10.0f);
        const Vector3 center{};
        const Vector3 up = Vector3::yAxis();
        _arcballCamera.emplace(_scene, eye, center, up, 45.0_degf,
            windowSize(), framebufferSize());
    }

    /* Loop at 60 Hz max */
    setSwapInterval(1);
    setMinimalLoopPeriod(16);
}

void ArcBallExample::drawEvent() {
    GL::defaultFramebuffer.clear(
        GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    /* Call arcball update in every frame. This will do nothing if the camera
       has not been changed. Otherwise, camera transformation will be
       propagated into the camera objects. */
    bool camChanged = _arcballCamera->update();
    _arcballCamera->draw(_drawables);
    swapBuffers();

    if(camChanged) redraw();
}

void ArcBallExample::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    _arcballCamera->reshape(event.windowSize(), event.framebufferSize());
    _shader.setViewportSize(Vector2{framebufferSize()});
}

void ArcBallExample::keyPressEvent(KeyEvent& event) {
    switch(event.key()) {
        case KeyEvent::Key::L:
            if(_arcballCamera->lagging() > 0.0f) {
                Debug{} << "Lagging disabled";
                _arcballCamera->setLagging(0.0f);
            } else {
                Debug{} << "Lagging enabled";
                _arcballCamera->setLagging(0.85f);
            }
            break;
        case KeyEvent::Key::R:
            _arcballCamera->reset();
            break;

        default: return;
    }

    event.setAccepted();
    redraw(); /* camera has changed, redraw! */
}

void ArcBallExample::mousePressEvent(MouseEvent& event) {
    /* Enable mouse capture so the mouse can drag outside of the window */
    /** @todo replace once https://github.com/mosra/magnum/pull/419 is in */
    SDL_CaptureMouse(SDL_TRUE);

    _arcballCamera->initTransformation(event.position());

    event.setAccepted();
    redraw(); /* camera has changed, redraw! */
}

void ArcBallExample::mouseReleaseEvent(MouseEvent&) {
    /* Disable mouse capture again */
    /** @todo replace once https://github.com/mosra/magnum/pull/419 is in */
    SDL_CaptureMouse(SDL_FALSE);
}

void ArcBallExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!event.buttons()) return;

    if(event.modifiers() & MouseMoveEvent::Modifier::Shift)
        _arcballCamera->translate(event.position());
    else _arcballCamera->rotate(event.position());

    event.setAccepted();
    redraw(); /* camera has changed, redraw! */
}

void ArcBallExample::mouseScrollEvent(MouseScrollEvent& event) {
    const Float delta = event.offset().y();
    if(Math::abs(delta) < 1.0e-2f) return;

    _arcballCamera->zoom(delta);

    event.setAccepted();
    redraw(); /* camera has changed, redraw! */
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::ArcBallExample)
