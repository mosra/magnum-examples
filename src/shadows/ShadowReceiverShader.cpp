/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>
        2016 — Bill Robinson <airbaggins@gmail.com>

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

#include "ShadowReceiverShader.h"

#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/Containers/StringStl.h>
#include <Corrade/Utility/Format.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/TextureArray.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Matrix4.h>

namespace Magnum { namespace Examples {

ShadowReceiverShader::ShadowReceiverShader(std::size_t numShadowLevels) {
    MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL330);

    const Utility::Resource rs{"shadow-data"};

    GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex};
    GL::Shader frag{GL::Version::GL330, GL::Shader::Type::Fragment};

    vert.addSource(Utility::format("#define NUM_SHADOW_MAP_LEVELS {}\n", + numShadowLevels))
        .addSource(rs.getString("ShadowReceiver.vert"));
    frag.addSource(Utility::format("#define NUM_SHADOW_MAP_LEVELS {}\n", + numShadowLevels))
        .addSource(rs.getString("ShadowReceiver.frag"));

    CORRADE_INTERNAL_ASSERT_OUTPUT(vert.compile() && frag.compile());

    bindAttributeLocation(Position::Location, "position");
    bindAttributeLocation(Normal::Location, "normal");

    attachShaders({vert, frag});

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    _modelMatrixUniform = uniformLocation("modelMatrix");
    _transformationProjectionMatrixUniform = uniformLocation("transformationProjectionMatrix");
    _shadowmapMatrixUniform = uniformLocation("shadowmapMatrix");
    _lightDirectionUniform = uniformLocation("lightDirection");
    _shadowBiasUniform = uniformLocation("shadowBias");

    setUniform(uniformLocation("shadowmapTexture"), ShadowmapTextureLayer);
}

ShadowReceiverShader& ShadowReceiverShader::setTransformationProjectionMatrix(const Matrix4& matrix) {
    setUniform(_transformationProjectionMatrixUniform, matrix);
    return *this;
}

ShadowReceiverShader& ShadowReceiverShader::setModelMatrix(const Matrix4& matrix) {
    setUniform(_modelMatrixUniform, matrix);
    return *this;
}

ShadowReceiverShader& ShadowReceiverShader::setShadowmapMatrices(const Containers::ArrayView<const Matrix4> matrices) {
    setUniform(_shadowmapMatrixUniform, matrices);
    return *this;
}

ShadowReceiverShader& ShadowReceiverShader::setLightDirection(const Vector3& vector) {
    setUniform(_lightDirectionUniform, vector);
    return *this;
}

ShadowReceiverShader& ShadowReceiverShader::setShadowmapTexture(GL::Texture2DArray& texture) {
    texture.bind(ShadowmapTextureLayer);
    return *this;
}

ShadowReceiverShader& ShadowReceiverShader::setShadowBias(const Float bias) {
    setUniform(_shadowBiasUniform, bias);
    return *this;
}

}}
