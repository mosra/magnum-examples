/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013 Vladimír Vondruš <mosra@centrum.cz>

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

#include <DefaultFramebuffer.h>
#include <Renderer.h>
#include <MeshTools/Interleave.h>
#include <MeshTools/CompressIndices.h>
#include <Platform/GlutApplication.h>
#include <Primitives/Cube.h>
#include <Shaders/PhongShader.h>
#include <Trade/MeshData3D.h>

namespace Magnum { namespace Examples {

class PrimitivesExample: public Platform::GlutApplication {
    public:
        explicit PrimitivesExample(int& argc, char** argv);

        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;

    private:
        Buffer indexBuffer, vertexBuffer;
        Mesh mesh;
        Shaders::PhongShader shader;

        Matrix4 transformation, projection;
        Vector2i previousMousePosition;
        Color3<> color;
};

PrimitivesExample::PrimitivesExample(int& argc, char** argv): GlutApplication(argc, argv, (new Configuration())->setTitle("Primitives example")) {
    Renderer::setFeature(Renderer::Feature::FaceCulling, true);
    Renderer::setFeature(Renderer::Feature::DepthTest, true);
    Renderer::setClearColor(Color3<>(0.125f));

    Trade::MeshData3D cube = Primitives::Cube::solid();

    MeshTools::interleave(&mesh, &vertexBuffer, Buffer::Usage::StaticDraw,
                          *cube.positions(0), *cube.normals(0));
    MeshTools::compressIndices(&mesh, &indexBuffer, Buffer::Usage::StaticDraw,
                               *cube.indices());

    mesh.setPrimitive(Mesh::Primitive::Triangles)
        ->addInterleavedVertexBuffer(&vertexBuffer, 0,
            Shaders::PhongShader::Position(),
            Shaders::PhongShader::Normal());

    transformation = Matrix4::rotationX(Deg(30.0f))*
                     Matrix4::rotationY(Deg(40.0f));

    color = Color3<>::fromHSV(Deg(35.0f), 1.0f, 1.0f);
}

void PrimitivesExample::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});

    projection = Matrix4::perspectiveProjection(Deg(35.0f), Float(size.x())/size.y(), 0.01f, 100.0f)*
                 Matrix4::translation(Vector3::zAxis(-10.0f));
}

void PrimitivesExample::drawEvent() {
    defaultFramebuffer.bind(DefaultFramebuffer::Target::Draw);
    defaultFramebuffer.clear(DefaultFramebuffer::Clear::Color|DefaultFramebuffer::Clear::Depth);

    shader.setLightPosition({7.0f, 5.0f, 2.5f})
        ->setLightColor(Color3<>(1.0f))
        ->setDiffuseColor(color)
        ->setAmbientColor(Color3<>::fromHSV(color.hue(), 1.0f, 0.3f))
        ->setTransformationMatrix(transformation)
        ->setProjectionMatrix(projection)
        ->use();
    mesh.draw();

    swapBuffers();
}

void PrimitivesExample::mousePressEvent(MouseEvent& event) {
    if(event.button() != MouseEvent::Button::Left) return;

    previousMousePosition = event.position();

    event.setAccepted();
}

void PrimitivesExample::mouseReleaseEvent(MouseEvent& event) {
    color = Color3<>::fromHSV(color.hue() + Deg(50.0), 1.0f, 1.0f);

    event.setAccepted();
    redraw();
}

void PrimitivesExample::mouseMoveEvent(MouseMoveEvent& event) {
    Vector2 delta = 3.0f*Vector2(event.position() - previousMousePosition)/defaultFramebuffer.viewport().size();

    transformation = Matrix4::rotationX(Rad(delta.y()))*transformation*Matrix4::rotationY(Rad(delta.x()));
    previousMousePosition = event.position();

    event.setAccepted();
    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::PrimitivesExample)
