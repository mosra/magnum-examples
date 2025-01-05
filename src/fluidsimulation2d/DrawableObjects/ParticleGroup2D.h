#ifndef Magnum_Examples_FluidSimulation2D_Particle2DGroup_h
#define Magnum_Examples_FluidSimulation2D_Particle2DGroup_h
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

#include <vector>
#include <Corrade/Containers/Pointer.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/SceneGraph/Camera.h>

#include "Shaders/ParticleSphereShader2D.h"

namespace Magnum { namespace Examples {

class ParticleGroup2D {
    public:
        explicit ParticleGroup2D(const std::vector<Vector2>& points, Float particleRadius);

        ParticleGroup2D& draw(Containers::Pointer<SceneGraph::Camera2D>& camera, Int screenHeight, Int projectionHeight);

        bool isDirty() const { return _dirty; }

        ParticleGroup2D& setDirty() {
            _dirty = true;
            return *this;
        }

        Float particleRadius() const { return _particleRadius; }

        ParticleGroup2D& setParticleRadius(Float radius) {
            _particleRadius = radius;
            return *this;
        }

        ParticleSphereShader2D::ColorMode colorMode() const { return _colorMode; }

        ParticleGroup2D& setColorMode(ParticleSphereShader2D::ColorMode colorMode) {
            _colorMode = colorMode;
            return *this;
        }

        const Color3& color() const { return _color; }

        ParticleGroup2D& setColor(const Color3& color) {
            _color = color;
            return *this;
        }

    private:
        const std::vector<Vector2>& _points;
        bool _dirty = false;

        Float _particleRadius = 1.0f;
        ParticleSphereShader2D::ColorMode _colorMode = ParticleSphereShader2D::ColorMode::RampColorById;
        Color3 _color{0.1f};

        GL::Buffer _bufferParticles;
        GL::Mesh   _meshParticles;
        Containers::Pointer<ParticleSphereShader2D> _particleShader;
};

}}

#endif
