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

using namespace std;
using namespace Corrade::Utility;

namespace Magnum { namespace Examples {

MotionBlurCamera::MotionBlurCamera(Object* parent): Camera(parent), framebuffer(AbstractTexture::ColorFormat::RGB, Type::UnsignedByte), currentFrame(0), canvas(frames) {
    for(size_t i = 0; i != FrameCount; ++i) {
        frames[i] = new Texture2D(i);
        frames[i]->setWrapping(Magnum::Math::Vector2<AbstractTexture::Wrapping>(AbstractTexture::Wrapping::ClampToEdge, AbstractTexture::Wrapping::ClampToEdge));
        frames[i]->setMinificationFilter(AbstractTexture::Filter::NearestNeighbor);
        frames[i]->setMagnificationFilter(AbstractTexture::Filter::NearestNeighbor);
    }
}

MotionBlurCamera::~MotionBlurCamera() {
    for(size_t i = 0; i != FrameCount; ++i)
        delete frames[i];
}

void MotionBlurCamera::setViewport(const Math::Vector2<GLsizei>& size) {
    Camera::setViewport(size);

    framebuffer.setDimensions(size, Buffer::Usage::DynamicDraw);

    /* Initialize previous frames with black color */
    size_t textureSize = size.x()*size.y()*AbstractTexture::pixelSize(AbstractTexture::ColorFormat::RGB, Type::UnsignedByte);
    GLubyte* texture = new GLubyte[textureSize]();
    Buffer::unbind(Buffer::Target::PixelPack);
    for(size_t i = 0; i != FrameCount; ++i)
        frames[i]->setData<GLubyte>(0, AbstractTexture::InternalFormat::RGB, size, framebuffer.colorFormat(), texture);
    delete[] texture;
}

void MotionBlurCamera::draw() {
    Camera::draw();

    framebuffer.setDataFromFramebuffer(Magnum::Math::Vector2<GLint>(0, 0));

    frames[currentFrame]->setData(0, AbstractTexture::InternalFormat::RGB, &framebuffer);

    canvas.draw(currentFrame);
    currentFrame = (currentFrame+1)%FrameCount;
}

MotionBlurCamera::MotionBlurShader::MotionBlurShader() {
    Resource rs("shaders");
    Shader* vertexShader = Shader::fromData(Shader::Vertex, rs.get("MotionBlurShader.vert"));
    Shader* fragmentShader = Shader::fromData(Shader::Fragment, rs.get("MotionBlurShader.frag"));
    attachShader(vertexShader);
    attachShader(fragmentShader);

    bindAttribute(Vertex::Location, "vertex");

    link();
    delete vertexShader;
    delete fragmentShader;

    stringstream ss;
    for(size_t i = 0; i != MotionBlurCamera::FrameCount; ++i) {
        ss.str("");
        ss << "frame[" << i << ']';
        frameUniforms[i] = uniformLocation(ss.str());
    }
}

MotionBlurCamera::MotionBlurCanvas::MotionBlurCanvas(Texture2D** frames, Object* parent): Object(parent), mesh(Mesh::Primitive::TriangleStrip, 4), frames(frames) {
    const Vector4 vertices[] = {
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {0.0f, -1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    };

    Buffer* buffer = mesh.addBuffer(false);
    buffer->setData(sizeof(vertices), vertices, Buffer::Usage::StaticDraw);
    mesh.bindAttribute<MotionBlurShader::Vertex>(buffer);
}

void MotionBlurCamera::MotionBlurCanvas::draw(size_t currentFrame) {
    shader.use();
    for(size_t i = 0; i != MotionBlurCamera::FrameCount; ++i)
        shader.setFrameUniform((i+currentFrame)%MotionBlurCamera::FrameCount, frames[(i+currentFrame)%MotionBlurCamera::FrameCount]);
    mesh.draw();
}

}}
