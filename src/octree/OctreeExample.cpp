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
#include <Magnum/Platform/Sdl2Application.h>


namespace Magnum { namespace Examples {

using namespace SceneGraph;
using namespace DebugTools;

typedef Scene<MatrixTransformation3D> Scene3D;
typedef Object<MatrixTransformation3D> Object3D;

class OctreeExample: public Platform::Application {
    public:
        explicit OctreeExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void addBox(Shapes::AxisAlignedBox3D& box);

        DebugTools::ResourceManager _manager;

        Scene3D _scene;
        Camera3D _camera; /* camera which will be culled for */
        Camera3D _viewer; /* camera from which we will render */

        OctreeDrawableGroup<Float> _drawables;
        ShapeGroup<3> _shapes;
};

OctreeExample::OctreeExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}.setTitle("Magnum Octree View Frustrum Culling Example")},
    _camera(_scene),
    _viewer(_scene)
{
    Renderer::enable(Renderer::Feature::DepthTest);

    _camera.setProjectionMatrix(Matrix4::perspectiveProjection(75.0_degf, Vector2{defaultFramebuffer.viewport().size()}.aspectRatio(), 0.01f, 100.0f));

    // TODO: Add more colors
    _manager.set("pink", DebugTools::ShapeRendererOptions().setColor({1.0f, 0.0f, 1.0f}));

    // TODO: Setup _camera and _viewer transformation

    // TODO: add more boxes
    auto shape = Shapes::AxisAlignedBox3D(Vector3{-0.5}, Vector3{0.5});
    addBox(shape);
}

void OctreeExample::addBox(Shapes::AxisAlignedBox3D& box) {
    Shapes::Shape<Shapes::AxisAlignedBox3D>* theShape = new Shapes::Shape<Shapes::AxisAlignedBox3D>(*(new Object3D(&_scene)), box, &_shapes);

    // TODO: maybe randomize the color? *later*
    ShapeRenderer<3>* renderer = new ShapeRenderer<3>(*theShape, ResourceKey("pink"));

    _drawables.add(*renderer, box);
}

void OctreeExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color | FramebufferClear::Depth);

    // TODO: cull for _camera

    _viewer.draw(_drawables);

    swapBuffers();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::OctreeExample)
