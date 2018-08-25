/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 —
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
#include <Magnum/GL/Renderer.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/VertexColor.h>

namespace Magnum { namespace Examples {

using Object3D = SceneGraph::Object<SceneGraph::MatrixTransformation3D>;
using Scene3D = SceneGraph::Scene<SceneGraph::MatrixTransformation3D>;

using namespace Math::Literals;

class MouseInteractionExample: public Platform::Application {
    public:
        explicit MouseInteractionExample(const Arguments &arguments);

    private:
        Float depthAt(const Vector2i& position);
        Vector3 unproject(const Vector2i& position, Float depth);

        void mousePressEvent(MouseEvent &event) override;
        void mouseMoveEvent(MouseMoveEvent &event) override;
        void mouseScrollEvent(MouseScrollEvent &event) override;
        void drawEvent() override;

        Shaders::VertexColor3D _shader;
        GL::Mesh _mesh;

        Scene3D _scene;
        SceneGraph::DrawableGroup3D _drawables;
        Object3D* _cameraObject;
        SceneGraph::Camera3D* _camera;

        Float _lastDepth{0.8f};
        Vector2i _lastPosition{-1};
        Vector3 _rotationPoint, _translationPoint;
};

class Drawable: public SceneGraph::Drawable3D {
    public:
        explicit Drawable(Object3D& object, Shaders::VertexColor3D& shader, GL::Mesh& mesh, SceneGraph::DrawableGroup3D& drawables): SceneGraph::Drawable3D{object, &drawables}, _shader(shader), _mesh(mesh) {}

        void draw(const Matrix4& transformation, SceneGraph::Camera3D& camera) {
            _shader.setTransformationProjectionMatrix(camera.projectionMatrix()*transformation);
            _mesh.draw(_shader);
        }

    private:
        Shaders::VertexColor3D& _shader;
        GL::Mesh& _mesh;
};

MouseInteractionExample::MouseInteractionExample(const Arguments &arguments):
    Platform::Application{arguments, Configuration{}
        .setTitle("Magnum Mouse Interaction Example")}
{
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
    _mesh.setCount(3)
        .addVertexBuffer(std::move(buffer), 0,
            Shaders::VertexColor3D::Position{},
            Shaders::VertexColor3D::Color3{});

    /* Triangle object */
    auto triangle = new Object3D{&_scene};
    new Drawable{*triangle, _shader, _mesh, _drawables};

    /* Set up the camera */
    _cameraObject = new Object3D{&_scene};
    _cameraObject->setTransformation(Matrix4::lookAt(Vector3::zAxis(5), {}, Vector3::yAxis()));
    _camera = new SceneGraph::Camera3D{*_cameraObject};
    _camera->setProjectionMatrix(Matrix4::perspectiveProjection(
        45.0_degf, Vector2{windowSize()}.aspectRatio(), 0.01f, 100.0f));
}

Float MouseInteractionExample::depthAt(const Vector2i& position) {
    const Vector2i fbPosition{position.x(), GL::defaultFramebuffer.viewport().sizeY() - position.y() - 1};

    GL::defaultFramebuffer.mapForRead(GL::DefaultFramebuffer::ReadAttachment::Front);
    Image2D data = GL::defaultFramebuffer.read(
        Range2Di::fromSize(fbPosition, Vector2i{1}).padded(Vector2i{2}),
        {GL::PixelFormat::DepthComponent, GL::PixelType::Float});

    return Math::min(Containers::arrayCast<const Float>(data.data()));
}

Vector3 MouseInteractionExample::unproject(const Vector2i& position, Float depth) {
    const Range2Di view = GL::defaultFramebuffer.viewport();
    const Vector2i fbPosition{position.x(), view.sizeY() - position.y() - 1};
    const Vector3 in{2*Vector2{fbPosition - view.min()}/Vector2{view.size()} - Vector2{1.0f}, depth*2.0f - 1.0f};

    /*
        Use the following to get global coordinates instead of camera-relative:

        (_cameraObject->absoluteTransformationMatrix()*_camera->projectionMatrix().inverted()).transformPoint(in)
    */
    return _camera->projectionMatrix().inverted().transformPoint(in);
}

void MouseInteractionExample::mousePressEvent(MouseEvent& event) {
    const Float currentDepth = depthAt(event.position());
    const Float depth = currentDepth == 1.0f ? _lastDepth : currentDepth;
    _translationPoint = unproject(event.position(), depth);
    if(currentDepth != 1.0f) {
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

void MouseInteractionExample::mouseScrollEvent(MouseScrollEvent &event) {
    const Float currentDepth = depthAt(event.position());
    const Float depth = currentDepth == 1.0f ? _lastDepth : currentDepth;
    const Vector3 p = unproject(event.position(), depth);
    if(currentDepth != 1.0f) {
        _rotationPoint = p;
        _lastDepth = depth;
    }

    const Int direction = event.offset().y();
    if(!direction) return;

    /* Move towards/backwards the rotation point in cam coords */
    _cameraObject->translateLocal(_rotationPoint*(direction < 0 ? -1.0f : 1.0f)*0.1f);

    redraw();
}

void MouseInteractionExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    _camera->draw(_drawables);

    swapBuffers();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::MouseInteractionExample)
