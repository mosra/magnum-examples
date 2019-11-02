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

#pragma once

#include "Shaders/ParticleSphereShader.h"

#include <Corrade/Containers/Pointer.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/SceneGraph/Camera.h>

#include <vector>

namespace Magnum { namespace Examples {
/****************************************************************************************************/
class ParticleGroup {
public:
    explicit ParticleGroup(const std::vector<Vector3>& points, float particleRadius);

    ParticleGroup& draw(Containers::Pointer<SceneGraph::Camera3D>& camera, const Vector2i& viewportSize);

    ParticleGroup& setDirty() { _bDirty = true; return *this; }
    ParticleGroup& setParticleRadius(float radius) { _particleRadius = radius; return *this; }
    ParticleGroup& setColorMode(int colorMode) { _colorMode = colorMode; return *this; }
    ParticleGroup& setAmbientColor(const Color3& color) { _ambientColor = color; return *this; }
    ParticleGroup& setDiffuseColor(const Color3& color) { _diffuseColor = color; return *this; }
    ParticleGroup& setSpecularColor(const Color3& color) { _specularColor = color; return *this; }
    ParticleGroup& setShininess(float shininess) { _shininess = shininess; return *this; }
    ParticleGroup& setLightDirection(const Vector3& lightDir) { _lightDir = lightDir; return *this; }

    int& colorMode() { return _colorMode; }
    Color3& ambientColor() { return _ambientColor; }
    Color3& diffuseColor() { return _diffuseColor; }
    Color3& specularColor() { return _specularColor; }
    float& shininess() { return _shininess; }
    Vector3& lightDirection() { return _lightDir; }

private:
    const std::vector<Vector3>& _points;
    bool _bDirty { false };

    float   _particleRadius { 1.0f };
    int     _colorMode { static_cast<int>(ParticleSphereShader::ColorMode::RAMP_COLOR_BY_ID) };
    Color3  _ambientColor { 0.1f };
    Color3  _diffuseColor { 0.0f, 0.5f, 0.9f };
    Color3  _specularColor { 1.0f };
    float   _shininess { 150.0f };
    Vector3 _lightDir { 1.0f, 1.0f, 2.0f };

    GL::Buffer _bufferParticles;
    GL::Mesh   _meshParticles;

    Containers::Pointer<ParticleSphereShader> _particleShader;
};

/****************************************************************************************************/
} } /* namespace Magnum::Examples  */
