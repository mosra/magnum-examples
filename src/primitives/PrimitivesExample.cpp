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

#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Renderer.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/CompressIndices.h>
#ifdef CORRADE_TARGET_NACL
#include <Magnum/Platform/NaClApplication.h>
#elif defined(CORRADE_TARGET_EMSCRIPTEN)
#include <Magnum/Platform/Sdl2Application.h>
#else
#include <Magnum/Platform/GlutApplication.h>
#endif
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/MeshData3D.h>

#ifdef MAGNUM_BUILD_STATIC
#include <Magnum/Shaders/resourceImport.hpp>
#endif

namespace Magnum { namespace Examples {

class PrimitivesExample: public Platform::Application {
    public:
        explicit PrimitivesExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;

        Buffer indexBuffer, vertexBuffer;
        Mesh mesh;
        Shaders::Phong shader;

        Matrix4 transformation, projection;
        Vector2i previousMousePosition;
        Color3 color;
};

PrimitivesExample::PrimitivesExample(const Arguments& arguments): Platform::Application(arguments, Configuration().setTitle("Magnum Primitives Example")) {
    indexBuffer.setTargetHint(Buffer::Target::ElementArray);

    Renderer::setFeature(Renderer::Feature::DepthTest, true);
    Renderer::setFeature(Renderer::Feature::FaceCulling, true);

    Trade::MeshData3D cube = Primitives::Cube::solid();

    MeshTools::compressIndices(mesh, indexBuffer, BufferUsage::StaticDraw, cube.indices());

    MeshTools::interleave(mesh, vertexBuffer, BufferUsage::StaticDraw,
        cube.positions(0), cube.normals(0));
    mesh.setPrimitive(cube.primitive())
        .addVertexBuffer(vertexBuffer, 0,
            Shaders::Phong::Position(), Shaders::Phong::Normal());

    transformation = Matrix4::rotationX(Deg(30.0f))*
                     Matrix4::rotationY(Deg(40.0f));
    color = Color3::fromHSV(Deg(35.0f), 1.0f, 1.0f);

    projection = Matrix4::perspectiveProjection(Deg(35.0f), Vector2(defaultFramebuffer.viewport().size()).aspectRatio(), 0.01f, 100.0f)*
                 Matrix4::translation(Vector3::zAxis(-10.0f));
}

void PrimitivesExample::drawEvent() {
    defaultFramebuffer.bind(FramebufferTarget::Draw);
    defaultFramebuffer.clear(FramebufferClear::Color|FramebufferClear::Depth);

    shader.setLightPosition({7.0f, 5.0f, 2.5f})
        .setLightColor(Color3(1.0f))
        .setDiffuseColor(color)
        .setAmbientColor(Color3::fromHSV(color.hue(), 1.0f, 0.3f))
        .setTransformationMatrix(transformation)
        .setNormalMatrix(transformation.rotationScaling()) /** @todo better solution? */
        .setProjectionMatrix(projection)
        .use();
    mesh.draw();

    swapBuffers();
}

void PrimitivesExample::mousePressEvent(MouseEvent& event) {
    if(event.button() != MouseEvent::Button::Left) return;

    previousMousePosition = event.position();
    event.setAccepted();
}

void PrimitivesExample::mouseReleaseEvent(MouseEvent& event) {
    color = Color3::fromHSV(color.hue() + Deg(50.0), 1.0f, 1.0f);

    event.setAccepted();
    redraw();
}

void PrimitivesExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    Vector2 delta = 3.0f*Vector2(event.position() - previousMousePosition)/Vector2(defaultFramebuffer.viewport().size());
    transformation =
        Matrix4::rotationX(Rad(delta.y()))*
        transformation*
        Matrix4::rotationY(Rad(delta.x()));

    previousMousePosition = event.position();
    event.setAccepted();
    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::PrimitivesExample)
