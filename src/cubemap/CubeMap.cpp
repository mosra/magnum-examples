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

#include "CubeMap.h"

#include <fstream>

#include "Utility/Resource.h"

#include "CubeMapTexture.h"
#include "Scene.h"
#include "Trade/AbstractImporter.h"
#include "Primitives/Cube.h"

#include "Reflector.h"

using namespace std;
using namespace Corrade::Utility;

namespace Magnum { namespace Examples {

CubeMap::CubeMap(Trade::AbstractImporter* importer, const string& prefix, Object* parent): Object(parent) {
    Buffer* buffer = cube.addBuffer(false);
    Primitives::Cube().build(&cube, buffer);
    cube.bindAttribute<Vector4>(buffer, CubeMapShader::Vertex);

    scale(20.0f, 20.0f, 20.0f);

    /* Textures */
    shared_ptr<Trade::Image2D> image;
    importer->open(prefix + "+x.tga");
    image = importer->image2D(0);
    texture.setDataPositiveX(0, AbstractTexture::InternalFormat::RGB, image.get());

    importer->open(prefix + "-x.tga");
    image = importer->image2D(0);
    texture.setDataNegativeX(0, AbstractTexture::InternalFormat::RGB, image.get());

    importer->open(prefix + "+y.tga");
    image = importer->image2D(0);
    texture.setDataPositiveY(0, AbstractTexture::InternalFormat::RGB, image.get());

    importer->open(prefix + "-y.tga");
    image = importer->image2D(0);
    texture.setDataNegativeY(0, AbstractTexture::InternalFormat::RGB, image.get());

    importer->open(prefix + "+z.tga");
    image = importer->image2D(0);
    texture.setDataPositiveZ(0, AbstractTexture::InternalFormat::RGB, image.get());

    importer->open(prefix + "-z.tga");
    image = importer->image2D(0);
    texture.setDataNegativeZ(0, AbstractTexture::InternalFormat::RGB, image.get());

    texture.setMagnificationFilter(CubeMapTexture::Filter::LinearInterpolation);
    texture.setMinificationFilter(CubeMapTexture::Filter::LinearInterpolation, CubeMapTexture::Mipmap::LinearInterpolation);
    texture.setWrapping(Math::Vector2<CubeMapTexture::Wrapping>(CubeMapTexture::Wrapping::ClampToEdge, CubeMapTexture::Wrapping::ClampToEdge));
    texture.generateMipmap();

    reflector = new Reflector(&texture, scene());
    reflector->scale(0.5f, 0.5f, 0.5f);
    reflector->translate(-0.5f, 0.0f, 0.0f);

    reflector = new Reflector(&texture, scene());
    reflector->scale(0.3f, 0.3f, 0.3f);
    reflector->translate(0.3f, 0.0f, 0.0f);
}

void CubeMap::draw(const Matrix4& transformationMatrix, Camera* camera) {
    texture.bind();
    shader.use();
    shader.setModelViewProjectionMatrixUniform(camera->projectionMatrix()*transformationMatrix);
    shader.setTextureUniform(&texture);
    cube.draw();
}

}}
