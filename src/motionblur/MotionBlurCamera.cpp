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

#include "MotionBlurCamera.h"

#include <sstream>

#include "Utility/Resource.h"
#include "Framebuffer.h"
#include <Shader.h>

using namespace std;
using namespace Corrade::Utility;

namespace Magnum { namespace Examples {

MotionBlurCamera::MotionBlurCamera(SceneGraph::Object3D* parent): Camera3D(parent), framebuffer(AbstractImage::Components::RGB, AbstractImage::ComponentType::UnsignedByte), currentFrame(0), canvas(frames) {
    for(GLint i = 0; i != FrameCount; ++i) {
        frames[i] = new Texture2D;
        frames[i]->setWrapping(Magnum::Math::Vector2<AbstractTexture::Wrapping>(AbstractTexture::Wrapping::ClampToEdge, AbstractTexture::Wrapping::ClampToEdge));
        frames[i]->setMinificationFilter(AbstractTexture::Filter::NearestNeighbor);
        frames[i]->setMagnificationFilter(AbstractTexture::Filter::NearestNeighbor);
    }
}

MotionBlurCamera::~MotionBlurCamera() {
    for(GLint i = 0; i != FrameCount; ++i)
        delete frames[i];
}

void MotionBlurCamera::setViewport(const Math::Vector2<GLsizei>& size) {
    Camera::setViewport(size);

    /* Initialize previous frames with black color */
    size_t textureSize = size.x()*size.y()*AbstractImage::pixelSize(framebuffer.components(), framebuffer.type());
    GLubyte* texture = new GLubyte[textureSize]();
    framebuffer.setData(size, framebuffer.components(), texture, Buffer::Usage::DynamicDraw);
    delete texture;

    Buffer::unbind(Buffer::Target::PixelPack);
    for(GLint i = 0; i != FrameCount; ++i)
        frames[i]->setData(0, AbstractTexture::Format::RGB, &framebuffer);
}

void MotionBlurCamera::draw() {
    Camera::draw();

    Framebuffer::read({0, 0}, viewport(), framebuffer.components(), framebuffer.type(), &framebuffer, Buffer::Usage::DynamicDraw);

    frames[currentFrame]->setData(0, AbstractTexture::Format::RGB, &framebuffer);

    canvas.draw(currentFrame);
    currentFrame = (currentFrame+1)%FrameCount;
}

MotionBlurCamera::MotionBlurShader::MotionBlurShader() {
    Resource rs("shaders");
    attachShader(Shader::fromData(Shader::Type::Vertex, rs.get("MotionBlurShader.vert")));
    attachShader(Shader::fromData(Shader::Type::Fragment, rs.get("MotionBlurShader.frag")));

    link();

    use();
    stringstream ss;
    for(GLint i = 0; i != MotionBlurCamera::FrameCount; ++i) {
        ss.str("");
        ss << "frame[" << i << ']';
        setUniform(uniformLocation(ss.str()), i);
    }
}

MotionBlurCamera::MotionBlurCanvas::MotionBlurCanvas(Texture2D** frames, SceneGraph::Object3D* parent): Object3D(parent), mesh(Mesh::Primitive::TriangleStrip, 4), frames(frames) {
    const Vector4 vertices[] = {
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {0.0f, -1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    };

    Buffer* buffer = mesh.addBuffer(Mesh::BufferType::NonInterleaved);
    buffer->setData(vertices, Buffer::Usage::StaticDraw);
    mesh.bindAttribute<MotionBlurShader::Position>(buffer);
}

void MotionBlurCamera::MotionBlurCanvas::draw(size_t currentFrame) {
    shader.use();
    for(GLint i = 0; i != MotionBlurCamera::FrameCount; ++i)
        frames[i]->bind((i+currentFrame)%MotionBlurCamera::FrameCount);
    mesh.draw();
}

}}
