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

#include "CubeMapShader.h"

#include <Corrade/Utility/Resource.h>
#include <Magnum/CubeMapTexture.h>
#include <Magnum/Shader.h>
#include <Magnum/Version.h>

namespace Magnum { namespace Examples {

namespace {
    enum: Int { TextureLayer = 0 };
}

CubeMapShader::CubeMapShader() {
    Utility::Resource rs("data");

    Shader vert(Version::GL330, Shader::Type::Vertex);
    Shader frag(Version::GL330, Shader::Type::Fragment);

    vert.addSource(rs.get("CubeMapShader.vert"));
    frag.addSource(rs.get("CubeMapShader.frag"));

    CORRADE_INTERNAL_ASSERT_OUTPUT(Shader::compile({vert, frag}));

    attachShaders({vert, frag});

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    _transformationProjectionMatrixUniform = uniformLocation("transformationProjectionMatrix");

    setUniform(uniformLocation("textureData"), TextureLayer);
}

CubeMapShader& CubeMapShader::setTexture(CubeMapTexture& texture) {
    texture.bind(TextureLayer);
    return *this;
}

}}
