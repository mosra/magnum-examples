/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015
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

#include "ReflectorShader.h"

#include <Corrade/Utility/Resource.h>
#include <Magnum/CubeMapTexture.h>
#include <Magnum/Shader.h>
#include <Magnum/Texture.h>
#include <Magnum/Version.h>

namespace Magnum { namespace Examples {

namespace {
    enum: Int {
        TextureLayer = 0,
        TarnishTextureLayer = 1
    };
}

ReflectorShader::ReflectorShader() {
    Utility::Resource rs("data");

    Shader vert(
        #ifndef MAGNUM_TARGET_GLES
        Version::GL330,
        #else
        Version::GLES300,
        #endif
        Shader::Type::Vertex);
    Shader frag(
        #ifndef MAGNUM_TARGET_GLES
        Version::GL330,
        #else
        Version::GLES300,
        #endif
        Shader::Type::Fragment);

    vert.addSource(rs.get("ReflectorShader.vert"));
    frag.addSource(rs.get("ReflectorShader.frag"));

    /* GCC 4.4 has explicit std::reference_wrapper constructor */
    CORRADE_INTERNAL_ASSERT_OUTPUT(Shader::compile({std::ref(vert), std::ref(frag)}));

    attachShaders({std::ref(vert), std::ref(frag)});

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    _transformationMatrixUniform = uniformLocation("transformationMatrix");
    _normalMatrixUniform = uniformLocation("normalMatrix");
    _projectionMatrixUniform = uniformLocation("projectionMatrix");
    _cameraMatrixUniform = uniformLocation("cameraMatrix");
    _reflectivityUniform = uniformLocation("reflectivity");
    _diffuseColorUniform = uniformLocation("diffuseColor");

    setUniform(uniformLocation("textureData"), TextureLayer);
    setUniform(uniformLocation("tarnishTextureData"), TarnishTextureLayer);
}

ReflectorShader& ReflectorShader::setTexture(CubeMapTexture& texture) {
    texture.bind(TextureLayer);
    return *this;
}

ReflectorShader& ReflectorShader::setTarnishTexture(Texture2D& texture) {
    texture.bind(TarnishTextureLayer);
    return *this;
}

}}
