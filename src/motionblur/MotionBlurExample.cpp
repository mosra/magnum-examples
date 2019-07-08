/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
            Vladimír Vondruš <mosra@centrum.cz>
        2019 — Winfried Baumann <winfried.baumann@tum.de>

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

#include <Corrade/Utility/utilities.h>
#include <Magnum/Mesh.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/MeshTools/Compile.h>
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
        void viewportEvent(ViewportEvent& event) override;
        void drawEvent() override;

        Scene3D scene;
        SceneGraph::DrawableGroup3D drawables;
        Object3D* cameraObject;
        MotionBlurCamera* camera;
        GL::Mesh mesh;
        Shaders::Phong shader;
        Object3D* spheres[3];
};

MotionBlurExample::MotionBlurExample(const Arguments& arguments): Platform::Application(arguments, Configuration().setTitle("Magnum Motion Blur Example")) {
    (cameraObject = new Object3D(&scene))
        ->translate(Vector3::zAxis(3.0f));
    (camera = new MotionBlurCamera(*cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(Deg(35.0f), 1.0f, 0.001f, 100))
        .setViewport(GL::defaultFramebuffer.viewport().size());
    GL::Renderer::setClearColor({0.1f, 0.1f, 0.1f});
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    mesh = MeshTools::compile(Primitives::icosphereSolid(3));

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

    setSwapInterval(16);
    setMinimalLoopPeriod(40);
}

void MotionBlurExample::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});
    camera->setViewport(event.windowSize());
}

void MotionBlurExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);
    camera->draw(drawables);
    swapBuffers();

    cameraObject->rotateX(Deg(0.5f));
    spheres[0]->rotateZ(Deg(-1.0f));
    spheres[1]->rotateZ(Deg(0.5f));
    spheres[2]->rotateZ(Deg(-0.25f));
    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::MotionBlurExample)
