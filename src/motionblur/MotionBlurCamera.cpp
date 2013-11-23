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

#include "MotionBlurCamera.h"

#include <sstream>
#include <Utility/Resource.h>
#include <ColorFormat.h>
#include <DefaultFramebuffer.h>
#include <Shader.h>
#include <TextureFormat.h>

namespace Magnum { namespace Examples {

MotionBlurCamera::MotionBlurCamera(SceneGraph::AbstractObject3D& object): SceneGraph::Camera3D(object), framebuffer(ColorFormat::RGB, ColorType::UnsignedByte), currentFrame(0), canvas(frames) {
    for(Int i = 0; i != FrameCount; ++i) {
        (frames[i] = new Texture2D)
            ->setWrapping(Sampler::Wrapping::ClampToEdge)
            .setMinificationFilter(Sampler::Filter::Nearest)
            .setMagnificationFilter(Sampler::Filter::Nearest);
    }
}

MotionBlurCamera::~MotionBlurCamera() {
    for(Int i = 0; i != FrameCount; ++i)
        delete frames[i];
}

void MotionBlurCamera::setViewport(const Vector2i& size) {
    SceneGraph::Camera3D::setViewport(size);

    /* Initialize previous frames with black color */
    std::size_t textureSize = size.product()*framebuffer.pixelSize();
    UnsignedByte* texture = new UnsignedByte[textureSize]();
    framebuffer.setData(size, ColorFormat::RGB, ColorType::UnsignedByte, nullptr, BufferUsage::DynamicDraw);
    delete texture;

    Buffer::unbind(Buffer::Target::PixelPack);
    for(Int i = 0; i != FrameCount; ++i)
        frames[i]->setImage(0, TextureFormat::RGB8, framebuffer);
}

void MotionBlurCamera::draw(SceneGraph::DrawableGroup3D& group) {
    SceneGraph::Camera3D::draw(group);

    defaultFramebuffer.read({0, 0}, viewport(), framebuffer, BufferUsage::DynamicDraw);

    frames[currentFrame]->setImage(0, TextureFormat::RGB8, framebuffer);

    canvas.draw(currentFrame);
    currentFrame = (currentFrame+1)%FrameCount;
}

MotionBlurCamera::MotionBlurShader::MotionBlurShader() {
    Utility::Resource rs("shaders");

    Shader vert(Version::GL330, Shader::Type::Vertex);
    vert.addSource(rs.get("MotionBlurShader.vert"));
    CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile());
    attachShader(vert);

    Shader frag(Version::GL330, Shader::Type::Fragment);
    frag.addSource(rs.get("MotionBlurShader.frag"));
    CORRADE_INTERNAL_ASSERT_OUTPUT(frag.compile());
    attachShader(frag);

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    std::stringstream ss;
    for(Int i = 0; i != MotionBlurCamera::FrameCount; ++i) {
        ss.str("");
        ss << "frame[" << i << "]";
        setUniform(uniformLocation(ss.str()), i);
    }
}

MotionBlurCamera::MotionBlurCanvas::MotionBlurCanvas(Texture2D** frames, Object3D* parent): Object3D(parent), frames(frames) {
    const Vector2 vertices[] = {
        {1.0f, -1.0f},
        {1.0f, 1.0f},
        {0.0f, -1.0f},
        {0.0f, 1.0f}
    };

    buffer.setData(vertices, BufferUsage::StaticDraw);
    mesh.setPrimitive(Mesh::Primitive::TriangleStrip)
        .setVertexCount(4)
        .addVertexBuffer(buffer, 0, MotionBlurShader::Position());
}

void MotionBlurCamera::MotionBlurCanvas::draw(std::size_t currentFrame) {
    shader.use();
    for(Int i = 0; i != MotionBlurCamera::FrameCount; ++i)
        frames[i]->bind((i+currentFrame)%MotionBlurCamera::FrameCount);
    mesh.draw();
}

}}
