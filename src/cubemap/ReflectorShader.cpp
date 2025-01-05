/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>

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

#include "ReflectorShader.h"

#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/Containers/StringStl.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/CubeMapTexture.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Version.h>

namespace Magnum { namespace Examples {

namespace {
    enum: Int {
        TextureLayer = 0,
        TarnishTextureLayer = 1
    };
}

ReflectorShader::ReflectorShader() {
    Utility::Resource rs("data");

    GL::Shader vert(GL::Version::GL330, GL::Shader::Type::Vertex);
    GL::Shader frag(GL::Version::GL330, GL::Shader::Type::Fragment);

    vert.addSource(rs.getString("ReflectorShader.vert"));
    frag.addSource(rs.getString("ReflectorShader.frag"));

    CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile() && frag.compile());

    attachShaders({vert, frag});

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

ReflectorShader& ReflectorShader::setTexture(GL::CubeMapTexture& texture) {
    texture.bind(TextureLayer);
    return *this;
}

ReflectorShader& ReflectorShader::setTarnishTexture(GL::Texture2D& texture) {
    texture.bind(TarnishTextureLayer);
    return *this;
}

}}
