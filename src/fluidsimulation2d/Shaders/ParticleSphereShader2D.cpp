/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>
        2019 — Nghia Truong <nghiatruong.vn@gmail.com>

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

#include "Shaders/ParticleSphereShader2D.h"

#include <Corrade/Containers/Iterable.h>
#include <Corrade/Containers/StringView.h>
#include <Corrade/Containers/StringStl.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>

namespace Magnum { namespace Examples {

ParticleSphereShader2D::ParticleSphereShader2D() {
    Utility::Resource rs("data");

    GL::Shader vertShader{ GL::Version::GL330, GL::Shader::Type::Vertex };
    GL::Shader fragShader{ GL::Version::GL330, GL::Shader::Type::Fragment };
    vertShader.addSource(rs.getString("ParticleSphereShader2D.vert"));
    fragShader.addSource(rs.getString("ParticleSphereShader2D.frag"));
    CORRADE_INTERNAL_ASSERT_OUTPUT(vertShader.compile() && fragShader.compile());

    attachShaders({vertShader, fragShader});
    CORRADE_INTERNAL_ASSERT(link());

    _uNumParticles = uniformLocation("numParticles");
    _uParticleRadius = uniformLocation("particleRadius");

    _uColorMode = uniformLocation("colorMode");
    _uColor = uniformLocation("uniformColor");

    _uViewProjectionMatrix = uniformLocation("viewProjectionMatrix");
    _uScreenHeight = uniformLocation("screenHeight");
    _uDomainHeight = uniformLocation("domainHeight");
}

ParticleSphereShader2D& ParticleSphereShader2D::setNumParticles(Int numParticles) {
    setUniform(_uNumParticles, numParticles);
    return *this;
}

ParticleSphereShader2D& ParticleSphereShader2D::setParticleRadius(Float radius) {
    setUniform(_uParticleRadius, radius);
    return *this;
}

ParticleSphereShader2D& ParticleSphereShader2D::setColorMode(Int colorMode) {
    setUniform(_uColorMode, colorMode);
    return *this;
}

ParticleSphereShader2D& ParticleSphereShader2D::setColor(const Color3& color) {
    setUniform(_uColor, color);
    return *this;
}

ParticleSphereShader2D& ParticleSphereShader2D::setViewProjectionMatrix(const Matrix3& matrix) {
    setUniform(_uViewProjectionMatrix, matrix);
    return *this;
}

ParticleSphereShader2D& ParticleSphereShader2D::setScreenHeight(Int height) {
    setUniform(_uScreenHeight, height);
    return *this;
}

ParticleSphereShader2D& ParticleSphereShader2D::setDomainHeight(Int height) {
    setUniform(_uDomainHeight, height);
    return *this;
}
} }
