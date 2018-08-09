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

#include <algorithm>

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Image.h>
#include <Magnum/Magnum.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/VertexColor.h>

#include <SDL_mouse.h>

namespace Magnum {
namespace Examples {

using Object3D =
    Magnum::SceneGraph::Object<Magnum::SceneGraph::MatrixTransformation3D>;
using Scene3D =
    Magnum::SceneGraph::Scene<Magnum::SceneGraph::MatrixTransformation3D>;

class InteractiveApplication : public Magnum::Platform::Sdl2Application {
  public:
    explicit InteractiveApplication(const Arguments &arguments,
                                    Magnum::NoCreateT)
        : Magnum::Platform::Sdl2Application{arguments, Magnum::NoCreate} {}
    explicit InteractiveApplication(const Arguments &arguments,
                                    const Configuration &conf)
        : Magnum::Platform::Sdl2Application{arguments, conf} {}

  protected:
    float depth_at(int x, int y);
    Magnum::Vector3 unproject(float x, float y, float z,
                              bool cam_coords = false);
    void mousePressEvent(MouseEvent &event) override;
    void mouseMoveEvent(MouseMoveEvent &event) override;
    void mouseScrollEvent(MouseScrollEvent &event) override;

    Object3D *_camera_object;
    Magnum::SceneGraph::Camera3D *_camera;
    float last_z = 0.8;
    Magnum::Vector3 rp, tp;
};

void InteractiveApplication::mousePressEvent(MouseEvent &event) {
    int x = event.position().x();
    int y = event.position().y();
    float orig_z = depth_at(x, y);
    float z = orig_z == 1.f ? last_z : orig_z;
    tp = unproject(x, y, z, /*cam_coords=*/true);
    if (orig_z != 1.f)
        rp = tp, last_z = z;
}

void InteractiveApplication::mouseMoveEvent(MouseMoveEvent &event) {
    Vector2i pos = event.position();
    int x = pos.x(), y = pos.y();
    static Vector2i last_pos = pos;
    Vector2i delta = pos - last_pos;
    last_pos = pos;
    if (event.buttons() & MouseMoveEvent::Button::Left) {
        Vector3 p = unproject(x, y, last_z, true);
        _camera_object->translateLocal({tp[0] - p[0], tp[1] - p[1], 0});
        tp = p;
        redraw();
    } else if (event.buttons() & MouseMoveEvent::Button::Right) {
        float rx = -0.01f * delta.y(), ry = -0.01f * delta.x();
        Matrix4 t = Matrix4::translation(rp) * Matrix4::rotationX(Rad(rx)) *
                    Matrix4::rotationY(Rad(ry)) * Matrix4::translation(-rp);
        _camera_object->transformLocal(t);
        redraw();
    }
}

void InteractiveApplication::mouseScrollEvent(MouseScrollEvent &event) {
    int x, y;
    SDL_GetMouseState(&x, &y);
    float orig_z = depth_at(x, y);
    float z = orig_z == 1.f ? last_z : orig_z;
    Vector3 p = unproject(x, y, z, /*cam_coords=*/true);
    if (orig_z != 1.f)
        rp = p, last_z = z;
    int dir = event.offset().y();
    if (!dir)
        return;
    Vector3 t = rp * (dir < 0 ? -1 : 1) * 0.1f;
    // move towards/backwards that point in cam coords
    _camera_object->translateLocal(t);
    redraw();
}

float InteractiveApplication::depth_at(int x, int y) {
    Range2Di view = GL::defaultFramebuffer.viewport();
    y = view.sizeY() - y;
    GL::defaultFramebuffer.mapForRead(
        GL::DefaultFramebuffer::ReadAttachment::Front);
    constexpr int w = 2;
    Image2D data = GL::defaultFramebuffer.read(
        Range2Di::fromSize({x - w, y - w}, {2 * w + 1, 2 * w + 1}),
        {GL::PixelFormat::DepthComponent, GL::PixelType::Float});
    return *std::min_element(data.data<Float>(),
                             data.data<float>() + data.pixelSize());
}

Vector3 InteractiveApplication::unproject(float x, float y, float z,
                                          bool cam_coords) {
    Range2Di view = GL::defaultFramebuffer.viewport();
    y = view.sizeY() - y;
    Vector4 in = {(x - view.min().x()) / view.sizeX() * 2 - 1,
                  (y - view.min().y()) / view.sizeY() * 2 - 1, z * 2 - 1, 1};
    Vector4 out = _camera->projectionMatrix().inverted() * in;
    if (!cam_coords)
        out = _camera->cameraMatrix().invertedRigid() * out;
    out /= out[3];
    return out.xyz();
}

template <typename Shader>
class Drawable : public Object3D, SceneGraph::Drawable3D {
  public:
    Drawable(Shader &&shader, Object3D &parent,
             SceneGraph::DrawableGroup3D &drawables)
        : Object3D{&parent}, SceneGraph::Drawable3D{*this, &drawables},
          shader(std::move(shader)) {}

    void draw(const Magnum::Matrix4 &transformation,
              SceneGraph::Camera3D &camera) {
        shader.setTransformationProjectionMatrix(camera.projectionMatrix() *
                                                 transformation);
        mesh.draw(shader);
    }

    Shader shader;
    GL::Mesh mesh;
    GL::Buffer buffer;
};

class InteractionExample : public InteractiveApplication {
  public:
    explicit InteractionExample(const Arguments &arguments);

  private:
    void drawEvent() override;

    Scene3D _scene;
    SceneGraph::DrawableGroup3D _drawables;
};

InteractionExample::InteractionExample(const Arguments &arguments)
    : InteractiveApplication{arguments, NoCreate} {
    using namespace Math::Literals;

    create(Configuration{}.setTitle("interaction"),
           GLConfiguration{}.setSampleCount(4));

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::setClearColor(0x222222_rgbf);

    struct {
        Vector3 pos;
        Color3 color;
    } data[4] = {{{-1.0f, -1.0f, 0.0f}, 0xff0000_rgbf},
                 {{1.0f, -1.0f, 0.0f}, 0x00ff00_rgbf},
                 {{0.0f, 1.0f, 0.0f}, 0x0000ff_rgbf}};

    auto tri = new Drawable<Shaders::VertexColor3D>(Shaders::VertexColor3D{},
                                                    _scene, _drawables);
    tri->buffer.setData(data);
    tri->mesh.setPrimitive(GL::MeshPrimitive::TriangleStrip)
        .setCount(3)
        .addVertexBuffer(tri->buffer, 0, Shaders::VertexColor3D::Position{},
                         Shaders::VertexColor3D::Color3{});

    Vector2i view_size = GL::defaultFramebuffer.viewport().size();
    _camera_object = new Object3D{&_scene};
    _camera_object->setTransformation(
        Matrix4::lookAt({0, 0, 5}, {0, 0, 0}, {0, 1, 0}));
    _camera = new SceneGraph::Camera3D{*_camera_object};
    _camera->setProjectionMatrix(Matrix4::perspectiveProjection(
        45.0_degf, Vector2{view_size}.aspectRatio(), 0.01f, 100.0f));
}

void InteractionExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color |
                                 GL::FramebufferClear::Depth);
    _camera->draw(_drawables);
    swapBuffers();
}

} // namespace Examples
} // namespace Magnum

MAGNUM_APPLICATION_MAIN(Magnum::Examples::InteractionExample)
