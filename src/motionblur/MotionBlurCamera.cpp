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
#include <Math/Point3D.h>
#include <DefaultFramebuffer.h>
#include <Shader.h>

namespace Magnum { namespace Examples {

MotionBlurCamera::MotionBlurCamera(SceneGraph::AbstractObject3D<>* object): Camera3D(object), framebuffer(AbstractImage::Format::RGB, AbstractImage::Type::UnsignedByte), currentFrame(0), canvas(frames) {
    for(GLint i = 0; i != FrameCount; ++i) {
        (frames[i] = new Texture2D)
            ->setWrapping(Texture2D::Wrapping::ClampToEdge)
            ->setMinificationFilter(Texture2D::Filter::NearestNeighbor)
            ->setMagnificationFilter(Texture2D::Filter::NearestNeighbor);
    }
}

MotionBlurCamera::~MotionBlurCamera() {
    for(GLint i = 0; i != FrameCount; ++i)
        delete frames[i];
}

void MotionBlurCamera::setViewport(const Vector2i& size) {
    Camera3D::setViewport(size);

    /* Initialize previous frames with black color */
    std::size_t textureSize = size.x()*size.y()*AbstractImage::pixelSize(AbstractImage::Format::RGB, AbstractImage::Type::UnsignedByte);
    GLubyte* texture = new GLubyte[textureSize]();
    framebuffer.setData(size, AbstractImage::Format::RGB, AbstractImage::Type::UnsignedByte, texture, Buffer::Usage::DynamicDraw);
    delete texture;

    Buffer::unbind(Buffer::Target::PixelPack);
    for(GLint i = 0; i != FrameCount; ++i)
        frames[i]->setImage(0, Texture2D::InternalFormat::RGB8, &framebuffer);
}

void MotionBlurCamera::draw(SceneGraph::DrawableGroup3D<>& group) {
    Camera3D::draw(group);

    defaultFramebuffer.read({0, 0}, viewport(), AbstractImage::Format::RGB, AbstractImage::Type::UnsignedByte, &framebuffer, Buffer::Usage::DynamicDraw);

    frames[currentFrame]->setImage(0, Texture2D::InternalFormat::RGB8, &framebuffer);

    canvas.draw(currentFrame);
    currentFrame = (currentFrame+1)%FrameCount;
}

MotionBlurCamera::MotionBlurShader::MotionBlurShader() {
    Corrade::Utility::Resource rs("shaders");
    attachShader(Shader::fromData(Version::GL330, Shader::Type::Vertex, rs.get("MotionBlurShader.vert")));
    attachShader(Shader::fromData(Version::GL330, Shader::Type::Fragment, rs.get("MotionBlurShader.frag")));

    link();

    std::stringstream ss;
    for(GLint i = 0; i != MotionBlurCamera::FrameCount; ++i) {
        ss.str("");
        ss << "frame[" << i << "]";
        setUniform(uniformLocation(ss.str()), i);
    }
}

MotionBlurCamera::MotionBlurCanvas::MotionBlurCanvas(Texture2D** frames, Object3D* parent): Object3D(parent), frames(frames) {
    const Point3D vertices[] = {
        {1.0f, -1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {0.0f, -1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}
    };

    buffer.setData(vertices, Buffer::Usage::StaticDraw);
    mesh.setPrimitive(Mesh::Primitive::TriangleStrip)
        ->setVertexCount(4)
        ->addVertexBuffer(&buffer, MotionBlurShader::Position());
}

void MotionBlurCamera::MotionBlurCanvas::draw(std::size_t currentFrame) {
    shader.use();
    for(GLint i = 0; i != MotionBlurCamera::FrameCount; ++i)
        frames[i]->bind((i+currentFrame)%MotionBlurCamera::FrameCount);
    mesh.draw();
}

}}
