/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
            Vladimír Vondruš <mosra@centrum.cz>
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

#include "ParticleSphereShader.h"

#include <Corrade/Containers/Reference.h>
#include <Corrade/Utility/Resource.h>
#include <Magnum/GL/Shader.h>
#include <Magnum/GL/Version.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>

namespace Magnum { namespace Examples {
/****************************************************************************************************/
ParticleSphereShader::ParticleSphereShader() {
    Utility::Resource rs("shaders");

    GL::Shader vertShader{ GL::Version::GL330, GL::Shader::Type::Vertex };
    GL::Shader fragShader{ GL::Version::GL330, GL::Shader::Type::Fragment };
    vertShader.addSource(rs.get("Particle.vert"));
    fragShader.addSource(rs.get("Particle.frag"));

    CORRADE_INTERNAL_ASSERT(GL::Shader::compile({ vertShader, fragShader }));
    attachShaders({ vertShader, fragShader });
    CORRADE_INTERNAL_ASSERT(link());

    _uNumParticles   = uniformLocation("numParticles");
    _uParticleRadius = uniformLocation("particleRadius");

    _uPointSizeScale = uniformLocation("pointSizeScale");
    _uColorMode      = uniformLocation("colorMode");
    _uAmbientColor   = uniformLocation("ambientColor");
    _uDiffuseColor   = uniformLocation("diffuseColor");
    _uSpecularColor  = uniformLocation("specularColor");
    _uShininess      = uniformLocation("shininess");

    _uViewMatrix       = uniformLocation("viewMatrix");
    _uProjectionMatrix = uniformLocation("projectionMatrix");
    _uLightDir         = uniformLocation("lightDir");
}

/****************************************************************************************************/
ParticleSphereShader& ParticleSphereShader::setNumParticles(int numParticles) {
    setUniform(_uNumParticles, numParticles);
    return *this;
}

ParticleSphereShader& ParticleSphereShader::setParticleRadius(float radius) {
    setUniform(_uParticleRadius, radius);
    return *this;
}

/****************************************************************************************************/
ParticleSphereShader& ParticleSphereShader::setPointSizeScale(float scale) {
    setUniform(_uPointSizeScale, scale);
    return *this;
}

ParticleSphereShader& ParticleSphereShader::setColorMode(int colorMode) {
    setUniform(_uColorMode, colorMode);
    return *this;
}

ParticleSphereShader& ParticleSphereShader::setAmbientColor(const Color3& color) {
    setUniform(_uAmbientColor, color);
    return *this;
}

ParticleSphereShader& ParticleSphereShader::setDiffuseColor(const Color3& color) {
    setUniform(_uDiffuseColor, color);
    return *this;
}

ParticleSphereShader& ParticleSphereShader::setSpecularColor(const Color3& color) {
    setUniform(_uSpecularColor, color);
    return *this;
}

ParticleSphereShader& ParticleSphereShader::setShininess(float shininess) {
    setUniform(_uShininess, shininess);
    return *this;
}

/****************************************************************************************************/
ParticleSphereShader& ParticleSphereShader::setViewMatrix(const Matrix4& matrix) {
    setUniform(_uViewMatrix, matrix);
    return *this;
}

ParticleSphereShader& ParticleSphereShader::setProjectionMatrix(const Matrix4& matrix) {
    setUniform(_uProjectionMatrix, matrix);
    return *this;
}

ParticleSphereShader& ParticleSphereShader::setLightDirection(const Vector3& lightDir) {
    setUniform(_uLightDir, lightDir);
    return *this;
}

/****************************************************************************************************/
} } /* namespace Magnum::Examples  */
