/*
    Copyright © 2010, 2011, 2012 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Magnum.

    Magnum is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Magnum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

#include "Framebuffer.h"
#include "SceneGraph/Scene.h"
#include "Contexts/GlutContext.h"
#include "MeshTools/CompressIndices.h"
#include "MeshTools/Interleave.h"
#include "Primitives/Icosphere.h"
#include "Shaders/PhongShader.h"

#include "MotionBlurCamera.h"
#include "IndexedMesh.h"
#include "Icosphere.h"

using namespace Corrade;
using namespace Magnum::Shaders;

namespace Magnum { namespace Examples {

class MotionBlurExample: public Contexts::GlutContext {
    public:
        MotionBlurExample(int& argc, char** argv): GlutContext(argc, argv, "Motion blur example") {
            camera = new MotionBlurCamera(&scene);
            camera->setAspectRatioPolicy(SceneGraph::Camera3D::AspectRatioPolicy::Extend);
            camera->setPerspective(deg(35.0f), 0.001f, 100);
            camera->translate(Vector3::zAxis(3.0f));
            Framebuffer::setClearColor({0.1f, 0.1f, 0.1f});
            Framebuffer::setFeature(Framebuffer::Feature::DepthTest, true);
            Framebuffer::setFeature(Framebuffer::Feature::FaceCulling, true);

            Primitives::Icosphere<3> data;
            MeshTools::compressIndices(&mesh, Buffer::Usage::StaticDraw, *data.indices());
            Buffer* buffer = mesh.addBuffer(Mesh::BufferType::Interleaved);
            MeshTools::interleave(&mesh, buffer, Buffer::Usage::StaticDraw, *data.vertices(0), *data.normals(0));
            mesh.bindAttribute<PhongShader::Vertex>(buffer);
            mesh.bindAttribute<PhongShader::Normal>(buffer);

            /* Add spheres to the scene */
            Icosphere* i = new Icosphere(&mesh, &shader, {1.0f, 1.0f, 0.0f}, &scene);

            spheres[0] = new SceneGraph::Object3D(&scene);
            i = new Icosphere(&mesh, &shader, {1.0f, 0.0f, 0.0f}, spheres[0]);
            i->translate(Vector3::yAxis(0.25f));
            i = new Icosphere(&mesh, &shader, {1.0f, 0.0f, 0.0f}, spheres[0]);
            i->translate(Vector3::yAxis(0.25f));
            i->rotate(deg(120.0f), Vector3::zAxis());
            i = new Icosphere(&mesh, &shader, {1.0f, 0.0f, 0.0f}, spheres[0]);
            i->translate(Vector3::yAxis(0.25f));
            i->rotate(deg(240.0f), Vector3::zAxis());

            spheres[1] = new SceneGraph::Object3D(&scene);
            i = new Icosphere(&mesh, &shader, {0.0f, 1.0f, 0.0f}, spheres[1]);
            i->translate(Vector3::yAxis(0.50f));
            i = new Icosphere(&mesh, &shader, {0.0f, 1.0f, 0.0f}, spheres[1]);
            i->translate(Vector3::yAxis(0.50f));
            i->rotate(deg(120.0f), Vector3::zAxis());
            i = new Icosphere(&mesh, &shader, {0.0f, 1.0f, 0.0f}, spheres[1]);
            i->translate(Vector3::yAxis(0.50f));
            i->rotate(deg(240.0f), Vector3::zAxis());

            spheres[2] = new SceneGraph::Object3D(&scene);
            i = new Icosphere(&mesh, &shader, {0.0f, 0.0f, 1.0f}, spheres[2]);
            i->translate(Vector3::yAxis(0.75f));
            i = new Icosphere(&mesh, &shader, {0.0f, 0.0f, 1.0f}, spheres[2]);
            i->translate(Vector3::yAxis(0.75f));
            i->rotate(deg(120.0f), Vector3::zAxis());
            i = new Icosphere(&mesh, &shader, {0.0f, 0.0f, 1.0f}, spheres[2]);
            i->translate(Vector3::yAxis(0.75f));
            i->rotate(deg(240.0f), Vector3::zAxis());
        }

    protected:
        inline void viewportEvent(const Math::Vector2<GLsizei>& size) {
            Framebuffer::setViewport({0, 0}, size);

            camera->setViewport(size);
        }

        void drawEvent() {
            Framebuffer::clear(Framebuffer::Clear::Color|Framebuffer::Clear::Depth);
            camera->draw();
            swapBuffers();

            camera->rotate(deg(1.0f), Vector3::yAxis());
            spheres[0]->rotate(deg(-2.0f), Vector3::zAxis());
            spheres[1]->rotate(deg(1.0f), Vector3::zAxis());
            spheres[2]->rotate(deg(-0.5f), Vector3::zAxis());
            Utility::sleep(40);
            redraw();
        }

    private:
        SceneGraph::Scene3D scene;
        SceneGraph::Camera3D* camera;
        IndexedMesh mesh;
        PhongShader shader;
        SceneGraph::Object3D* spheres[3];
};

}}

int main(int argc, char** argv) {
    Magnum::Examples::MotionBlurExample e(argc, argv);
    return e.exec();
}
