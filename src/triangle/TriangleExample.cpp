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

#include <Math/Vector3.h>
#include <Buffer.h>
#include <DefaultFramebuffer.h>
#include <Mesh.h>
#include <Platform/GlutApplication.h>
#include <Shaders/VertexColor.h>

namespace Magnum { namespace Examples {

class TriangleExample: public Platform::GlutApplication {
    public:
        explicit TriangleExample(const Arguments& arguments);

    protected:
        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;

    private:
        Buffer buffer;
        Mesh mesh;
        Shaders::VertexColor3D shader;
};

TriangleExample::TriangleExample(const Arguments& arguments): Platform::GlutApplication(arguments, (new Configuration)->setTitle("Triangle example")) {
    constexpr static Vector3 data[] = {
        {-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, /* Left vertex, red color */
        { 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, /* Right vertex, green color */
        { 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}  /* Top vertex, blue color */
    };

    buffer.setData(data, Buffer::Usage::StaticDraw);
    mesh.setPrimitive(Mesh::Primitive::Triangles)
        ->setVertexCount(3)
        ->addInterleavedVertexBuffer(&buffer, 0,
            Shaders::VertexColor3D::Position(),
            Shaders::VertexColor3D::Color());
}

void TriangleExample::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});
}

void TriangleExample::drawEvent() {
    defaultFramebuffer.bind(FramebufferTarget::Draw);
    defaultFramebuffer.clear(FramebufferClear::Color);

    shader.use();
    mesh.draw();

    swapBuffers();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::TriangleExample)
