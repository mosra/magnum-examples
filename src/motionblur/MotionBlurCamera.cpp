/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017 —
            Vladimír Vondruš <mosra@centrum.cz>

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

#include "MotionBlurCamera.h"

#include <sstream>
#include <Corrade/Containers/Array.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Shader.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/Version.h>

namespace Magnum { namespace Examples {

MotionBlurCamera::MotionBlurCamera(SceneGraph::AbstractObject3D& object): SceneGraph::Camera3D(object), framebuffer(PixelFormat::RGB, PixelType::UnsignedByte), currentFrame(0), canvas(frames) {
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
    Containers::Array<UnsignedByte> texture{Containers::ValueInit, textureSize};
    framebuffer.setData(PixelFormat::RGB, PixelType::UnsignedByte, size, texture, BufferUsage::DynamicDraw);

    for(Int i = 0; i != FrameCount; ++i)
        frames[i]->setImage(0, TextureFormat::RGB8, framebuffer);
}

void MotionBlurCamera::draw(SceneGraph::DrawableGroup3D& group) {
    SceneGraph::Camera3D::draw(group);

    defaultFramebuffer.read({{}, viewport()}, framebuffer, BufferUsage::DynamicDraw);

    frames[currentFrame]->setImage(0, TextureFormat::RGB8, framebuffer);

    canvas.draw(currentFrame);
    currentFrame = (currentFrame+1)%FrameCount;
}

MotionBlurCamera::MotionBlurShader::MotionBlurShader() {
    Utility::Resource rs("shaders");

    Shader vert(Version::GL330, Shader::Type::Vertex);
    Shader frag(Version::GL330, Shader::Type::Fragment);

    vert.addSource(rs.get("MotionBlurShader.vert"));
    frag.addSource(rs.get("MotionBlurShader.frag"));

    CORRADE_INTERNAL_ASSERT_OUTPUT(Shader::compile({vert, frag}));

    attachShaders({vert, frag});

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
    mesh.setPrimitive(MeshPrimitive::TriangleStrip)
        .setCount(4)
        .addVertexBuffer(buffer, 0, MotionBlurShader::Position());
}

void MotionBlurCamera::MotionBlurCanvas::draw(std::size_t currentFrame) {
    for(Int i = 0; i != MotionBlurCamera::FrameCount; ++i)
        frames[i]->bind((i+currentFrame)%MotionBlurCamera::FrameCount);
    mesh.draw(shader);
}

}}
