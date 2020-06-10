/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 —
            Vladimír Vondruš <mosra@centrum.cz>
        2018 — scturtle <scturtle@gmail.com>

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

#include <Magnum/Image.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/PixelFormat.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/FunctionsBatch.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Grid.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/Flat.h>
#include <Magnum/Shaders/VertexColor.h>
#include <Magnum/Trade/MeshData.h>

namespace Magnum { namespace Examples {

using Object3D = SceneGraph::Object<SceneGraph::MatrixTransformation3D>;
using Scene3D = SceneGraph::Scene<SceneGraph::MatrixTransformation3D>;

using namespace Math::Literals;

class MouseInteractionExample: public Platform::Application {
    public:
        explicit MouseInteractionExample(const Arguments& arguments);

    private:
        Float depthAt(const Vector2i& windowPosition);
        Vector3 unproject(const Vector2i& windowPosition, Float depth) const;

        void keyPressEvent(KeyEvent& event) override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void mouseScrollEvent(MouseScrollEvent& event) override;
        void drawEvent() override;

        Shaders::VertexColor3D _vertexColorShader{NoCreate};
        Shaders::Flat3D _flatShader{NoCreate};
        GL::Mesh _mesh{NoCreate}, _grid{NoCreate};

        Scene3D _scene;
        SceneGraph::DrawableGroup3D _drawables;
        Object3D* _cameraObject;
        SceneGraph::Camera3D* _camera;

        Float _lastDepth;
        Vector2i _lastPosition{-1};
        Vector3 _rotationPoint, _translationPoint;
};

class VertexColorDrawable: public SceneGraph::Drawable3D {
    public:
        explicit VertexColorDrawable(Object3D& object, Shaders::VertexColor3D& shader, GL::Mesh& mesh, SceneGraph::DrawableGroup3D& drawables): SceneGraph::Drawable3D{object, &drawables}, _shader(shader), _mesh(mesh) {}

        void draw(const Matrix4& transformation, SceneGraph::Camera3D& camera) {
            _shader
                .setTransformationProjectionMatrix(camera.projectionMatrix()*transformation)
                .draw(_mesh);
        }

    private:
        Shaders::VertexColor3D& _shader;
        GL::Mesh& _mesh;
};

class FlatDrawable: public SceneGraph::Drawable3D {
    public:
        explicit FlatDrawable(Object3D& object, Shaders::Flat3D& shader, GL::Mesh& mesh, SceneGraph::DrawableGroup3D& drawables): SceneGraph::Drawable3D{object, &drawables}, _shader(shader), _mesh(mesh) {}

        void draw(const Matrix4& transformation, SceneGraph::Camera3D& camera) {
            _shader
                .setColor(0x747474_rgbf)
                .setTransformationProjectionMatrix(camera.projectionMatrix()*transformation)
                .draw(_mesh);
        }

    private:
        Shaders::Flat3D& _shader;
        GL::Mesh& _mesh;
};

MouseInteractionExample::MouseInteractionExample(const Arguments& arguments): Platform::Application{arguments, NoCreate} {
    /* Try 8x MSAA, fall back to zero samples if not possible. Enable only 2x
       MSAA if we have enough DPI. */
    {
        const Vector2 dpiScaling = this->dpiScaling({});
        Configuration conf;
        conf.setTitle("Magnum Mouse Interaction Example")
            .setSize(conf.size(), dpiScaling);
        GLConfiguration glConf;
        glConf.setSampleCount(dpiScaling.max() < 2.0f ? 8 : 2);
        if(!tryCreate(conf, glConf))
            create(conf, glConf.setSampleCount(0));
    }

    /* Shaders, renderer setup */
    _vertexColorShader = Shaders::VertexColor3D{};
    _flatShader = Shaders::Flat3D{};
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);

    /* Triangle data */
    const struct {
        Vector3 pos;
        Color3 color;
    } data[]{{{-1.0f, -1.0f, 0.0f}, 0xff0000_rgbf},
             {{ 1.0f, -1.0f, 0.0f}, 0x00ff00_rgbf},
             {{ 0.0f,  1.0f, 0.0f}, 0x0000ff_rgbf}};

    /* Triangle mesh */
    GL::Buffer buffer;
    buffer.setData(data);
    _mesh = GL::Mesh{};
    _mesh.setCount(3)
        .addVertexBuffer(std::move(buffer), 0,
            Shaders::VertexColor3D::Position{},
            Shaders::VertexColor3D::Color3{});

    /* Triangle object */
    auto triangle = new Object3D{&_scene};
    new VertexColorDrawable{*triangle, _vertexColorShader, _mesh, _drawables};

    /* Grid */
    _grid = MeshTools::compile(Primitives::grid3DWireframe({15, 15}));
    auto grid = new Object3D{&_scene};
    (*grid)
        .rotateX(90.0_degf)
        .scale(Vector3{8.0f});
    new FlatDrawable{*grid, _flatShader, _grid, _drawables};

    /* Set up the camera */
    _cameraObject = new Object3D{&_scene};
    (*_cameraObject)
        .translate(Vector3::zAxis(5.0f))
        .rotateX(-15.0_degf)
        .rotateY(30.0_degf);
    _camera = new SceneGraph::Camera3D{*_cameraObject};
    _camera->setProjectionMatrix(Matrix4::perspectiveProjection(
        45.0_degf, Vector2{windowSize()}.aspectRatio(), 0.01f, 100.0f));

    /* Initialize initial depth to the value at scene center */
    _lastDepth = ((_camera->projectionMatrix()*_camera->cameraMatrix()).transformPoint({}).z() + 1.0f)*0.5f;
}

Float MouseInteractionExample::depthAt(const Vector2i& windowPosition) {
    /* First scale the position from being relative to window size to being
       relative to framebuffer size as those two can be different on HiDPI
       systems */
    const Vector2i position = windowPosition*Vector2{framebufferSize()}/Vector2{windowSize()};
    const Vector2i fbPosition{position.x(), GL::defaultFramebuffer.viewport().sizeY() - position.y() - 1};

    GL::defaultFramebuffer.mapForRead(GL::DefaultFramebuffer::ReadAttachment::Front);
    Image2D data = GL::defaultFramebuffer.read(
        Range2Di::fromSize(fbPosition, Vector2i{1}).padded(Vector2i{2}),
        {GL::PixelFormat::DepthComponent, GL::PixelType::Float});

    /* TODO: change to just Math::min<Float>(data.pixels<Float>() when the
       batch functions in Math can handle 2D views */
    return Math::min<Float>(data.pixels<Float>().asContiguous());
}

Vector3 MouseInteractionExample::unproject(const Vector2i& windowPosition, Float depth) const {
    /* We have to take window size, not framebuffer size, since the position is
       in window coordinates and the two can be different on HiDPI systems */
    const Vector2i viewSize = windowSize();
    const Vector2i viewPosition{windowPosition.x(), viewSize.y() - windowPosition.y() - 1};
    const Vector3 in{2*Vector2{viewPosition}/Vector2{viewSize} - Vector2{1.0f}, depth*2.0f - 1.0f};

    /*
        Use the following to get global coordinates instead of camera-relative:

        (_cameraObject->absoluteTransformationMatrix()*_camera->projectionMatrix().inverted()).transformPoint(in)
    */
    return _camera->projectionMatrix().inverted().transformPoint(in);
}

void MouseInteractionExample::keyPressEvent(KeyEvent& event) {
    /* Reset the transformation to the original view */
    if(event.key() == KeyEvent::Key::NumZero) {
        (*_cameraObject)
            .resetTransformation()
            .translate(Vector3::zAxis(5.0f))
            .rotateX(-15.0_degf)
            .rotateY(30.0_degf);
        redraw();
        return;

    /* Axis-aligned view */
    } else if(event.key() == KeyEvent::Key::NumOne ||
              event.key() == KeyEvent::Key::NumThree ||
              event.key() == KeyEvent::Key::NumSeven)
    {
        /* Start with current camera translation with the rotation inverted */
        const Vector3 viewTranslation = _cameraObject->transformation().rotationScaling().inverted()*_cameraObject->transformation().translation();

        /* Front/back */
        const Float multiplier = event.modifiers() & KeyEvent::Modifier::Ctrl ? -1.0f : 1.0f;

        Matrix4 transformation;
        if(event.key() == KeyEvent::Key::NumSeven) /* Top/bottom */
            transformation = Matrix4::rotationX(-90.0_degf*multiplier);
        else if(event.key() == KeyEvent::Key::NumOne) /* Front/back */
            transformation = Matrix4::rotationY(90.0_degf - 90.0_degf*multiplier);
        else if(event.key() == KeyEvent::Key::NumThree) /* Right/left */
            transformation = Matrix4::rotationY(90.0_degf*multiplier);
        else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

        _cameraObject->setTransformation(transformation*Matrix4::translation(viewTranslation));
        redraw();
    }
}

void MouseInteractionExample::mousePressEvent(MouseEvent& event) {
    /* Due to compatibility reasons, scroll is also reported as a press event,
       so filter that out. Could be removed once MouseEvent::Button::Wheel is
       gone from Magnum. */
    if(event.button() != MouseEvent::Button::Left &&
       event.button() != MouseEvent::Button::Middle)
        return;

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

void MouseInteractionExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(_lastPosition == Vector2i{-1}) _lastPosition = event.position();
    const Vector2i delta = event.position() - _lastPosition;
    _lastPosition = event.position();

    if(!event.buttons()) return;

    /* Translate */
    if(event.modifiers() & MouseMoveEvent::Modifier::Shift) {
        const Vector3 p = unproject(event.position(), _lastDepth);
        _cameraObject->translateLocal(_translationPoint - p); /* is Z always 0? */
        _translationPoint = p;

    /* Rotate around rotation point */
    } else _cameraObject->transformLocal(
        Matrix4::translation(_rotationPoint)*
        Matrix4::rotationX(-0.01_radf*delta.y())*
        Matrix4::rotationY(-0.01_radf*delta.x())*
        Matrix4::translation(-_rotationPoint));

    redraw();
}

void MouseInteractionExample::mouseScrollEvent(MouseScrollEvent& event) {
    const Float currentDepth = depthAt(event.position());
    const Float depth = currentDepth == 1.0f ? _lastDepth : currentDepth;
    const Vector3 p = unproject(event.position(), depth);
    /* Update the rotation point only if we're not zooming against infinite
       depth or if the original rotation point is not yet initialized */
    if(currentDepth != 1.0f || _rotationPoint.isZero()) {
        _rotationPoint = p;
        _lastDepth = depth;
    }

    const Float direction = event.offset().y();
    if(!direction) return;

    /* Move towards/backwards the rotation point in cam coords */
    _cameraObject->translateLocal(_rotationPoint*direction*0.1f);

    event.setAccepted();
    redraw();
}

void MouseInteractionExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    _camera->draw(_drawables);

    swapBuffers();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::MouseInteractionExample)
