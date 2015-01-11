/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015
              Vladimír Vondruš <mosra@centrum.cz>

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

#include <Corrade/Utility/utilities.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Renderer.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Icosphere.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/MeshData3D.h>

#include "MotionBlurCamera.h"
#include "Icosphere.h"

namespace Magnum { namespace Examples {

class MotionBlurExample: public Platform::Application {
    public:
        explicit MotionBlurExample(const Arguments& arguments);

    private:
        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;

        Scene3D scene;
        SceneGraph::DrawableGroup3D drawables;
        Object3D* cameraObject;
        SceneGraph::Camera3D* camera;
        Buffer buffer;
        Buffer indexBuffer;
        Mesh mesh;
        Shaders::Phong shader;
        Object3D* spheres[3];
};

MotionBlurExample::MotionBlurExample(const Arguments& arguments): Platform::Application(arguments, Configuration().setTitle("Magnum Motion Blur Example")) {
    (cameraObject = new Object3D(&scene))
        ->translate(Vector3::zAxis(3.0f));
    (camera = new MotionBlurCamera(*cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setPerspective(Deg(35.0f), 1.0f, 0.001f, 100)
        .setViewport(defaultFramebuffer.viewport().size());
    Renderer::setClearColor({0.1f, 0.1f, 0.1f});
    Renderer::enable(Renderer::Feature::DepthTest);
    Renderer::enable(Renderer::Feature::FaceCulling);

    const Trade::MeshData3D data = Primitives::Icosphere::solid(3);

    buffer.setData(MeshTools::interleave(data.positions(0), data.normals(0)), BufferUsage::StaticDraw);

    Containers::Array<char> indexData;
    Mesh::IndexType indexType;
    UnsignedInt indexStart, indexEnd;
    std::tie(indexData, indexType, indexStart, indexEnd) = MeshTools::compressIndices(data.indices());
    indexBuffer.setData(indexData, BufferUsage::StaticDraw);

    mesh.setPrimitive(data.primitive())
        .setCount(data.indices().size())
        .addVertexBuffer(buffer, 0, Shaders::Phong::Position{}, Shaders::Phong::Normal{})
        .setIndexBuffer(indexBuffer, 0, indexType, indexStart, indexEnd);

    /* Add spheres to the scene */
    new Icosphere(&mesh, &shader, {1.0f, 1.0f, 0.0f}, &scene, &drawables);

    spheres[0] = new Object3D(&scene);
    (new Icosphere(&mesh, &shader, {1.0f, 0.0f, 0.0f}, spheres[0], &drawables))
        ->translate(Vector3::yAxis(0.25f));
    (new Icosphere(&mesh, &shader, {1.0f, 0.0f, 0.0f}, spheres[0], &drawables))
        ->translate(Vector3::yAxis(0.25f))
        .rotateZ(Deg(120.0f));
    (new Icosphere(&mesh, &shader, {1.0f, 0.0f, 0.0f}, spheres[0], &drawables))
        ->translate(Vector3::yAxis(0.25f))
        .rotateZ(Deg(240.0f));

    spheres[1] = new Object3D(&scene);
    (new Icosphere(&mesh, &shader, {0.0f, 1.0f, 0.0f}, spheres[1], &drawables))
        ->translate(Vector3::yAxis(0.50f));
    (new Icosphere(&mesh, &shader, {0.0f, 1.0f, 0.0f}, spheres[1], &drawables))
        ->translate(Vector3::yAxis(0.50f))
        .rotateZ(Deg(120.0f));
    (new Icosphere(&mesh, &shader, {0.0f, 1.0f, 0.0f}, spheres[1], &drawables))
        ->translate(Vector3::yAxis(0.50f))
        .rotateZ(Deg(240.0f));

    spheres[2] = new Object3D(&scene);
    (new Icosphere(&mesh, &shader, {0.0f, 0.0f, 1.0f}, spheres[2], &drawables))
        ->translate(Vector3::yAxis(0.75f));
    (new Icosphere(&mesh, &shader, {0.0f, 0.0f, 1.0f}, spheres[2], &drawables))
        ->translate(Vector3::yAxis(0.75f))
        .rotateZ(Deg(120.0f));
    (new Icosphere(&mesh, &shader, {0.0f, 0.0f, 1.0f}, spheres[2], &drawables))
        ->translate(Vector3::yAxis(0.75f))
        .rotateZ(Deg(240.0f));
}

void MotionBlurExample::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});
    camera->setViewport(size);
}

void MotionBlurExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color|FramebufferClear::Depth);
    camera->draw(drawables);
    swapBuffers();

    cameraObject->rotateX(Deg(1.0f));
    spheres[0]->rotateZ(Deg(-2.0f));
    spheres[1]->rotateZ(Deg(1.0f));
    spheres[2]->rotateZ(Deg(-0.5f));
    Utility::sleep(40);
    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::MotionBlurExample)
