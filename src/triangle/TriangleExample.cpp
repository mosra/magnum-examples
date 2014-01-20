/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014
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

#include <Magnum/Buffer.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Mesh.h>
#include <Magnum/Math/Vector3.h>
#ifdef CORRADE_TARGET_NACL
#include <Magnum/Platform/NaClApplication.h>
#elif defined(CORRADE_TARGET_EMSCRIPTEN)
#include <Magnum/Platform/Sdl2Application.h>
#else
#include <Magnum/Platform/GlutApplication.h>
#endif
#include <Magnum/Shaders/VertexColor.h>

#ifdef MAGNUM_BUILD_STATIC
#include <Magnum/Shaders/resourceImport.hpp>
#endif

namespace Magnum { namespace Examples {

class TriangleExample: public Platform::Application {
    public:
        explicit TriangleExample(const Arguments& arguments);

    private:
        void drawEvent() override;

        Buffer buffer;
        Mesh mesh;
        Shaders::VertexColor3D shader;
};

TriangleExample::TriangleExample(const Arguments& arguments): Platform::Application(arguments, Configuration().setTitle("Magnum Triangle Example")) {
    constexpr static Vector3 data[] = {
        {-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, /* Left vertex, red color */
        { 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, /* Right vertex, green color */
        { 0.0f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}  /* Top vertex, blue color */
    };

    buffer.setData(data, BufferUsage::StaticDraw);
    mesh.setPrimitive(MeshPrimitive::Triangles)
        .setVertexCount(3)
        .addVertexBuffer(buffer, 0,
            Shaders::VertexColor3D::Position(),
            Shaders::VertexColor3D::Color());
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
