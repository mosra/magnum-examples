/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016 —
            Vladimír Vondruš <mosra@centrum.cz>
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

#include <Corrade/Utility/Resource.h>
#include <Magnum/Context.h>
#include <Magnum/Shader.h>
#include <Magnum/Version.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/TextureArray.h>

using namespace Magnum;

ShadowReceiverShader::ShadowReceiverShader(int numShadowLevels) {
    MAGNUM_ASSERT_VERSION_SUPPORTED(Version::GL330);
    const Utility::Resource rs{"shadow-data"};

    Shader vert{Version::GL330, Shader::Type::Vertex};
    Shader frag{Version::GL330, Shader::Type::Fragment};

    std::string preamble = "#define NUM_SHADOW_MAP_LEVELS " + std::to_string(numShadowLevels) + "\n";
    vert.addSource(preamble);
    vert.addSource(rs.get("ShadowReceiver.vert"));
    frag.addSource(preamble);
    frag.addSource(rs.get("ShadowReceiver.frag"));

    CORRADE_INTERNAL_ASSERT_OUTPUT(Shader::compile({vert, frag}));

    bindAttributeLocation(Position::Location, "position");
    bindAttributeLocation(Normal::Location, "normal");

    attachShaders({vert, frag});

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    modelMatrixUniform = uniformLocation("modelMatrix");
    transformationProjectionMatrixUniform = uniformLocation("transformationProjectionMatrix");
    shadowmapMatrixUniform = uniformLocation("shadowmapMatrix");
    lightDirectionUniform = uniformLocation("lightDirection");
    shadowBiasUniform = uniformLocation("shadowBias");

    setUniform(uniformLocation("shadowmapTexture"), ShadowmapTextureLayer);
}

ShadowReceiverShader::~ShadowReceiverShader() {

}

ShadowReceiverShader &ShadowReceiverShader::setTransformationProjectionMatrix(const Magnum::Matrix4 &matrix) {
    setUniform(transformationProjectionMatrixUniform, matrix);
    return *this;
}

ShadowReceiverShader &ShadowReceiverShader::setModelMatrix(const Magnum::Matrix4 &matrix) {
    setUniform(modelMatrixUniform, matrix);
    return *this;
}

ShadowReceiverShader &ShadowReceiverShader::setShadowmapMatrices(const Corrade::Containers::ArrayView<Magnum::Matrix4> &matrix) {
    setUniform(shadowmapMatrixUniform, matrix);
    return *this;
}

ShadowReceiverShader &ShadowReceiverShader::setLightDirection(const Magnum::Vector3 &vector3) {
    setUniform(lightDirectionUniform, vector3);
    return *this;
}

ShadowReceiverShader &ShadowReceiverShader::setShadowmapTexture(Magnum::Texture2DArray &texture) {
    texture.bind(ShadowmapTextureLayer);
    return *this;
}

ShadowReceiverShader &ShadowReceiverShader::setShadowBias(float bias) {
    setUniform(shadowBiasUniform, bias);
    return *this;
}
