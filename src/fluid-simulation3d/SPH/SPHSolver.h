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

#include "SPHKernels.h"
#include "DomainBox.h"

#include <Magnum/Magnum.h>
#include <vector>

namespace Magnum { namespace Examples {
/****************************************************************************************************/
struct SPHParams {
    float stiffness { 20000.0f };
    float viscosity { 0.05f };
    float boundaryRestitution { 0.5f };
};

/****************************************************************************************************/
/* This is a very basic implementation of SPH (Smoothed Particle Hydrodynamics) solver
 * For the purpose of fast running (as this is a real-time application example),
 * accuracy has been heavily sacrificed for performance
 */
class SPHSolver final {
public:
    explicit SPHSolver(float particleRadius);

    void setPositions(const std::vector<Vector3>& particlePositions);
    void reset();
    void advance();

    DomainBox& domainBox() { return _domainBox; }
    SPHParams& simulationParameters() { return _params; }

    size_t numParticles() const { return _positions.size(); }
    const auto& particlePositions() { return _positions; }

private:
    void computeDensities();
    void velocityIntegration(float timestep);
    void computeViscosity();
    void updatePositions(float timestep);

    static constexpr float s_restDensity { 1000.0f }; /* Rest density of fluid */
    const float            _particleRadius;           /* Particle radius = half distance between consecutive particles */
    const float            _particleMass;             /* particle_mass = pow(particle_spacing, 3) * rest_density */

    std::vector<Vector3> _positions;
    std::vector<Vector3> _positions_t0; /* Initial positions */

    /* For saving memory thus gaining performance,
     * particle densities (float32) are clamped to [0, 10000] then static_cast into uint16_t */
    std::vector<uint16_t> _densities;

    /* Other particle states */
    std::vector<std::vector<uint32_t>> _neighbors;
    std::vector<std::vector<Vector3>>  _relPositions;
    std::vector<Vector3>               _velocities;
    std::vector<Vector3>               _velocityDiffusions;

    /* SPH kernels */
    SPHKernels _kernels;

    /* Boundary */
    DomainBox _domainBox;

    /* Parameters */
    SPHParams _params;
};

/****************************************************************************************************/
} } /* namespace Magnum::Examples  */
