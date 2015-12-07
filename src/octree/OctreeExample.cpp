/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015
              Vladimír Vondruš <mosra@centrum.cz>
    Copyright © 2015 Andrea Capobianco <andrea.c.2205@gmail.com>
    Copyright © 2015 Jonathan Hale <squareys@googlemail.com>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/DebugTools/ShapeRenderer.h>
#include <Magnum/DebugTools/ResourceManager.h>
#include <Magnum/Renderer.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/OctreeDrawableGroup.h>
#include <Magnum/SceneGraph/OctreeDrawableGroup.hpp>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/Shapes/Shape.h>
#include <Magnum/Shapes/ShapeGroup.h>
#include <Magnum/Shapes/AxisAlignedBox.h>
#include <Magnum/Shapes/Box.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Buffer.h>
#include <Magnum/Mesh.h>
#include <Magnum/Shaders/Flat.h>


namespace Magnum { namespace Examples {

using namespace SceneGraph;
using namespace DebugTools;

typedef Scene<MatrixTransformation3D> Scene3D;
typedef Object<MatrixTransformation3D> Object3D;

class FrustumDrawable: public Object3D, public SceneGraph::Drawable3D {
    public:
        explicit FrustumDrawable(Object3D* parent, SceneGraph::DrawableGroup3D* group, Mesh& mesh): Object3D{parent}, SceneGraph::Drawable3D{*this, group}, _mesh(mesh) {
        }
    private:
        void draw(const Matrix4& transformationMatrix, Camera3D& camera) override {
            /* The scaling in the TransformationProjectionMatrix for reflection, because of use of DirectX matrices instead of OpenGL ones */
            _shader.setColor(Color3::fromHSV(216.0_degf, 0.85f, 1.0f))
                    .setTransformationProjectionMatrix(camera.projectionMatrix() * transformationMatrix * Matrix4::scaling(Vector3{1, 1, -1}));
            _mesh.draw(_shader);
        }
        Mesh& _mesh;
        Shaders::Flat3D _shader;
};

class OctreeExample: public Platform::Application {
    public:
        explicit OctreeExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void addBox(Shapes::AxisAlignedBox3D& box);
        Vector3 calculateIntersection(const Vector4& v1, const Vector4& v2, const Vector4& v3);

        DebugTools::ResourceManager _manager;

        Scene3D _scene;
        Object3D _cameraObject;
        Object3D _viewerObject;
        Camera3D _camera; /* camera which will be culled for */
        Camera3D _viewer; /* camera from which we will render */

        OctreeDrawableGroup<Float> _culledDrawables;
        ShapeGroup<3> _shapes;

        DrawableGroup3D _drawables;

        Buffer _buffer;
        Mesh _mesh;

};

OctreeExample::OctreeExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}.setTitle("Magnum Octree View Frustrum Culling Example")},
    _cameraObject(&_scene),
    _viewerObject(&_scene),
    _camera(_cameraObject),
    _viewer(_viewerObject)
{
    Renderer::enable(Renderer::Feature::DepthTest);

    _camera.setProjectionMatrix(Matrix4::perspectiveProjection(75.0_degf, Vector2{defaultFramebuffer.viewport().size()}.aspectRatio(), 0.01f, 75.0f));
    _viewer.setProjectionMatrix(Matrix4::perspectiveProjection(75.0_degf, Vector2{defaultFramebuffer.viewport().size()}.aspectRatio(), 0.01f, 150.0f));

    _manager.set("pink", DebugTools::ShapeRendererOptions().setColor({1.0f, 0.0f, 1.0f}));

    //Setup _camera and _viewer transformation
    _viewerObject.translate({0.0f, 10.0f, 50.0f});
    _viewerObject.rotateXLocal(Deg(-10.0f));

    //create visualisation of _camera view frustum
    const Matrix4 projection = Matrix4(_camera.projectionMatrix()).transposed();
    const Vector4 left{projection[3] + projection[0]};
    const Vector4 right{projection[3] - projection[0]};
    const Vector4 bottom{projection[3] + projection[1]};
    const Vector4 top{projection[3] - projection[1]};
    const Vector4 near{projection[2]};
    const Vector4 far{projection[3] - projection[2]};

    const Vector3 rbn = calculateIntersection(right, bottom, near);
    const Vector3 lbn = calculateIntersection(left, bottom, near);
    const Vector3 rtn = calculateIntersection(right, top, near);
    const Vector3 ltn = calculateIntersection(left, top, near);
    const Vector3 rbf = calculateIntersection(right, bottom, far);
    const Vector3 lbf = calculateIntersection(left, bottom, far);
    const Vector3 rtf = calculateIntersection(right, top, far);
    const Vector3 ltf = calculateIntersection(left, top, far);

    const Vector3 data[] = {
        rbn, lbn, ltn, rtn,
        rbn, rbf, rtf, rtn,
        rtf, ltf, ltn, ltf,
        lbf, lbn, lbf, rbf
    };

    _buffer.setData(data, BufferUsage::StaticDraw);
    _mesh.setPrimitive(MeshPrimitive::LineStrip)
         .setCount(16)
         .addVertexBuffer(_buffer, 0, Shaders::Flat3D::Position{});

    new FrustumDrawable(&_cameraObject, &_drawables, _mesh);

    // TODO: add more boxes
    // TODO: Do it with a loop and random values
    auto rightBox = Shapes::AxisAlignedBox3D(Vector3{10.0, -0.5, -0.5}, Vector3{11.0, 0.5, 0.5});
    //doesn't break the culling
    //auto rightBox = Shapes::AxisAlignedBox3D(Vector3{10.0, -0.5, -10.5}, Vector3{11.0, 0.5, 0.5});
    //breaks the culling
    //auto rightBox = Shapes::AxisAlignedBox3D(Vector3{10.0, -0.5, -15.5}, Vector3{11.0, 0.5, 0.5});
    addBox(rightBox);

    auto leftBox = Shapes::AxisAlignedBox3D(Vector3{-11.0, -0.5, -0.5}, Vector3{-10.0, 0.5, 0.5});
    addBox(leftBox);

    auto nearBox = Shapes::AxisAlignedBox3D(Vector3{-0.5, -0.5, 10.0}, Vector3{0.5, 0.5, 11.0});
    addBox(nearBox);

    auto farBox = Shapes::AxisAlignedBox3D(Vector3{-0.5, -0.5, -11.0}, Vector3{0.5, 0.5, -10.0});
    addBox(farBox);

    auto originBox = Shapes::AxisAlignedBox3D(Vector3{-0.5}, Vector3{0.5});
    addBox(originBox);

    //breaks the culling
    //auto shape5 = Shapes::AxisAlignedBox3D(Vector3{-25.5, 5.5, -46.5}, Vector3{-24.5, 6.5, -45.5});
    //addBox(shape5);

}

Vector3 OctreeExample::calculateIntersection(const Vector4 &v1, const Vector4 &v2, const Vector4 &v3) {

    const Float det = Matrix3(v1.xyz(), v2.xyz(), v3.xyz()).transposed().determinant();

    const Float x = Matrix3({v1.w(), v2.w(), v3.w()}, {v1.y(), v2.y(), v3.y()}, {v1.z(), v2.z(), v3.z()}).determinant() / det;
    const Float y = Matrix3({v1.x(), v2.x(), v3.x()}, {v1.w(), v2.w(), v3.w()}, {v1.z(), v2.z(), v3.z()}).determinant() / det;
    const Float z = Matrix3({v1.x(), v2.x(), v3.x()}, {v1.y(), v2.y(), v3.y()}, {v1.w(), v2.w(), v3.w()}).determinant() / det;

    return Vector3{x, y, z};
}

void OctreeExample::addBox(Shapes::AxisAlignedBox3D& aabox) {

    const Vector3 s = aabox.max() - aabox.min();
    const Vector3 t = aabox.min() + s/2;
    const Matrix4 transformationMatrix = Matrix4::translation(t) * Matrix4::scaling(s);

    Shapes::Shape<Shapes::Box3D>* box = new Shapes::Shape<Shapes::Box3D>(_scene, transformationMatrix, &_shapes);
    new DebugTools::ShapeRenderer3D(*box, ResourceKey("pink"), &_culledDrawables);

    // TODO: maybe randomize the color? *later*
    ShapeRenderer<3>* renderer = new ShapeRenderer<3>(*box, ResourceKey("pink"));

    _culledDrawables.add(*renderer, aabox);
}

void OctreeExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color | FramebufferClear::Depth);

    // cull for _camera
    _culledDrawables.buildOctree();
    _culledDrawables.cull(_camera);

    _viewer.draw(_culledDrawables);
    _viewer.draw(_drawables);
    swapBuffers();

    // rotate _camera to change cull result
    _cameraObject.rotateYLocal(Deg(0.5f));

    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::OctreeExample)
