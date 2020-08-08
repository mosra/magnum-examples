/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 —
            Vladimír Vondruš <mosra@centrum.cz>
        2020 — Janos Meny <janos.meny@gmail.com>

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

#include "SsaoShader.h"

#include <Magnum/GL/Texture.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/ImageFormat.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Functions.h>

#include <Corrade/Containers/Reference.h>
#include <Corrade/Utility/Resource.h>
#include <Corrade/Utility/FormatStl.h>

#include <random>

namespace Magnum { namespace Examples {

namespace {
enum {
    PositionUnit = 0,
    NormalUnit = 1,
    NoiseUnit = 2,
    OcclusionUnit = 3,
};
}

SsaoShader::SsaoShader(Flag flag, UnsignedInt sampleCount) {

    constexpr Float mean = 0.f;
    constexpr Float std = 0.4f;

    Containers::Array<Vector3> randomSamples(Containers::NoInit, sampleCount);
    std::default_random_engine engine;
    std::normal_distribution<float> normalDistr(mean, std);

    for (UnsignedInt i = 0; i < sampleCount; ++i) {
        Vector3 p{normalDistr(engine), normalDistr(engine), normalDistr(engine)};
        if(p.z() < 0)
            p.z() *= -1.f;
        p = Math::fmod(p, Vector3{1});
        randomSamples[i] = p;
    }
    Utility::Resource rs{"ssao-data"};

    if(flag == Flag::FragmentShader){
        MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL330);
        GL::Shader vert{GL::Version::GL330, GL::Shader::Type::Vertex};
        GL::Shader frag{GL::Version::GL330, GL::Shader::Type::Fragment};

        vert.addSource(rs.get("FullScreenTriangle.vert"));
        frag.addSource(Utility::formatString("#define SAMPLE_COUNT {}\n", sampleCount))
            .addSource(rs.get("Ssao.frag"));

        CORRADE_INTERNAL_ASSERT_OUTPUT(GL::Shader::compile({vert, frag}));
        attachShaders({vert, frag});
    } else {
        MAGNUM_ASSERT_GL_VERSION_SUPPORTED(GL::Version::GL430);
        GL::Shader comp{GL::Version::GL430, GL::Shader::Type::Compute};
        comp.addSource(Utility::formatString("#define SAMPLE_COUNT {}\n", sampleCount))
                .addSource(rs.get("Ssao.comp"));

        CORRADE_INTERNAL_ASSERT_OUTPUT(comp.compile());
        attachShaders({comp});
    }

    CORRADE_INTERNAL_ASSERT_OUTPUT(link());

    bindFragmentDataLocation(AmbientOcclusionOutput, "ambientOcclusion");

    if(flag == Flag::FragmentShader){
        setUniform(uniformLocation("positions"), PositionUnit);
        setUniform(uniformLocation("normals"), NormalUnit);
        setUniform(uniformLocation("noise"), NoiseUnit);

        _projectionUniform = uniformLocation("projection");
        _biasUniform = uniformLocation("bias");
        _radiusUniform = uniformLocation("radius");
        _samplesUniform = uniformLocation("samples");
    }

    setUniform(_samplesUniform, randomSamples);
}

SsaoShader& SsaoShader::bindPositionTexture(GL::Texture2D& texture){
    texture.bind(PositionUnit);
    return *this;
}

SsaoShader& SsaoShader::bindNormalTexture(GL::Texture2D& texture){
    texture.bind(NormalUnit);
    return *this;
}

SsaoShader& SsaoShader::bindOcclusionTexture(GL::Texture2D& texture){
    texture.bindImage(OcclusionUnit, 0, GL::ImageAccess::WriteOnly, GL::ImageFormat::R32F);
    return *this;
}

SsaoShader& SsaoShader::bindNoiseTexture(Magnum::GL::Texture2D& texture) {
    texture.bind(NoiseUnit);
    return *this;
}

SsaoShader& SsaoShader::setProjectionMatrix(const Matrix4& projection) {
    setUniform(_projectionUniform, projection);
    return *this;
}

SsaoShader& SsaoShader::setSampleRadius(Float radius) {
    setUniform(_radiusUniform, radius);
    return *this;
}

SsaoShader& SsaoShader::setBias(Float bias) {
    setUniform(_biasUniform, bias);
    return *this;
}

}}
