/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015
              Vladimír Vondruš <mosra@centrum.cz>
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

#include <Magnum/Buffer.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Renderer.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Shaders/Flat.h>
#include <Magnum/Trade/MeshData2D.h>
#include <Magnum/Primitives/Plane.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/Drawable.h>

#include "Types.h"

namespace Magnum {namespace Primitives { namespace Plane2D {

Trade::MeshData2D solid(const Plane::TextureCoords textureCoords) {
    std::vector<std::vector<Vector2>> coords;
    if(textureCoords == Plane::TextureCoords::Generate) coords.push_back({
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 0.0f},
        {0.0f, 1.0f}
    });

    return Trade::MeshData2D(MeshPrimitive::TriangleStrip, {}, {{
        {1.0f, -1.0f},
        {1.0f, 1.0f},
        {-1.0f, -1.0f},
        {-1.0f, 1.0f}
    }}, std::move(coords));
}

}}}

namespace Magnum {namespace Examples {

class AudioExample: public Platform::Application {
    public:
        explicit AudioExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;

        Buffer _indexBuffer, _vertexBuffer;
        Mesh _mesh;
        Shaders::Flat2D _shader;

        Scene2D _scene;
        SceneGraph::Camera2D _camera;
        SceneGraph::DrawableGroup2D _drawables;

        Vector2i _previousMousePosition;
};

AudioExample::AudioExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}.setTitle("Magnum Audio Example")},
    _scene(),
    _camera(_scene)
{

    const Trade::MeshData2D plane = Primitives::Plane2D::solid(Primitives::Plane::TextureCoords::Generate);

    _vertexBuffer.setData(plane.positions(0), BufferUsage::StaticDraw);
    _mesh.setPrimitive(plane.primitive())
        .addVertexBuffer(_vertexBuffer, 0, Shaders::Flat2D::Position{});

    _camera.setProjectionMatrix(Matrix3::projection(Vector2{defaultFramebuffer.viewport().size()}));
}

void AudioExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color);

    _camera.draw(_drawables);

    swapBuffers();
}

void AudioExample::mousePressEvent(MouseEvent& event) {
    if(event.button() != MouseEvent::Button::Left) return;

    _previousMousePosition = event.position();
    event.setAccepted();
}

void AudioExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    event.setAccepted();
    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::AudioExample)
