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

#include "SPHSolver.h"

#include "TaskScheduler.h"

namespace Magnum { namespace Examples {

SPHSolver::SPHSolver(Float particleRadius):
    _particleRadius{particleRadius},
    _particleMass{Math::pow(2.0f*particleRadius, 3.0f)*RestDensity*0.9f},
    _kernels{particleRadius*4.0f},
    _domainBox{particleRadius, Vector3{particleRadius}, Vector3{3.0f, 3.0f, 1.0f} - Vector3{particleRadius}} {}

void SPHSolver::setPositions(const std::vector<Vector3>& positions) {
    _positions = positions;
    _positionsT0 = positions;

    /* Resize other variables */
    const auto nParticles = positions.size();
    _densities.resize(nParticles);
    _neighbors.resize(nParticles);
    _relPositions.resize(nParticles);
    /* Must initialize zero for all velocities */
    _velocities.assign(nParticles, Vector3{0.0f});
    _velocityDiffusions.resize(nParticles);
}

void SPHSolver::reset() {
    _positions = _positionsT0;
    /* Must initialize zero for all velocities */
    _velocities.assign(numParticles(), Vector3{0.0f});
}

void SPHSolver::advance() {
    /* Find neighbors and compute relative positions with them */
    _domainBox.findNeighbors(_positions, _neighbors, _relPositions);

    /* This is a fixed time step approach! In practice, adaptive time step
       should be used. */
    const Float timestep = 0.05f*_particleRadius;

    computeDensities();
    velocityIntegration(timestep);
    computeViscosity();
    updatePositions(timestep);
}

void SPHSolver::computeDensities() {
    TaskScheduler::forEach(_positions.size(), [&](const std::size_t p) {
        const std::vector<Vector3>& relPositions = _relPositions[p];
        if(relPositions.size() == 0) return;

        auto pdensity = _kernels.W0();
        for(const Vector3& xpq: relPositions)
            pdensity += _kernels.W(xpq);
        pdensity *= _particleMass;

        /* Clamp and cast to uint16_t */
        _densities[p] = UnsignedShort(Math::round(Math::min(pdensity, 10000.0f)));
    });
}

void SPHSolver::velocityIntegration(float timestep) {
    auto pressure = [](const Float rho) {
        const Float ratio = rho/RestDensity;
        if(ratio < 1.0f) return 0.0f;

        const Float ratioExp2 = ratio*ratio;
        const Float ratioExp4 = ratioExp2*ratioExp2;
        const Float ratioExp7 = ratioExp4*ratioExp2*ratio;
        return ratioExp7 - 1.0f;
    };

    TaskScheduler::forEach(_positions.size(), [&](const std::size_t p) {
        const auto& neighbors = _neighbors[p];
        if(neighbors.size() == 0) {
            /* A lonely particle only interacts with gravity */
            _velocities[p].y() -= timestep*9.81f;
            return;
        }

        const std::vector<Vector3>& relPositions = _relPositions[p];
        const Float pdensity = Float(_densities[p]);
        const Float ppressure = pressure(pdensity);
        const Float Kp = ppressure/(pdensity*pdensity);

        /* Compute the pressure acceleration caused by normal fluid particles */
        Vector3 accel{0.0f};
        for(std::size_t idx = 0; idx < neighbors.size(); ++idx) {
            const UnsignedInt q = neighbors[idx];
            const Float qdensity = Float(_densities[q]);
            const Float qpressure = pressure(qdensity);
            const Float Kq = qpressure/(qdensity*qdensity);
            const Vector3 r = relPositions[idx];

            /* Pressure acceleration */
            accel -= (Kp + Kq)*_kernels.gradW(r);
        }

        /* Compute the pressure acceleration caused by ghost boundary particles */
        for(std::size_t idx = neighbors.size(); idx < relPositions.size(); ++idx) {
            const Vector3 r = relPositions[idx];
            accel -= Kp*_kernels.gradW(r);
        }

        accel *= _params.stiffness*_particleMass;
        accel.y() -= 9.81f; /* add gravity */

        /* Update velocity from acceleration and gravity */
        _velocities[p] += accel*timestep;
    });
}

void SPHSolver::computeViscosity() {
    TaskScheduler::forEach(_positions.size(), [&](const std::size_t p) {
        const std::vector<UnsignedInt>& neighbors = _neighbors[p];
        if(neighbors.empty()) {
            _velocityDiffusions[p] = Vector3(0);
            return;
        }

        const std::vector<Vector3>& relPositions = _relPositions[p];
        const Vector3 pvel = _velocities[p];

        Vector3 diffuseVel{0.0f};
        for(std::size_t idx = 0; idx != neighbors.size(); ++idx) {
            const UnsignedInt q = neighbors[idx];
            const Vector3 qvel = _velocities[q];
            const Float qdensity = Float(_densities[q]);
            const Vector3 r = relPositions[idx];

            diffuseVel += (1.0f/qdensity)*_kernels.W(r)*(qvel - pvel);
        }

        diffuseVel *= _params.viscosity*_particleMass;
        _velocityDiffusions[p] = diffuseVel;
    });

    /* Add diffused velocity back to velocity, causing viscosity */
    TaskScheduler::forEach(_positions.size(), [&](const std::size_t p) {
        _velocities[p] += _velocityDiffusions[p];
    });
}

void SPHSolver::updatePositions(Float timestep) {
    TaskScheduler::forEach(_positions.size(), [&](const std::size_t p) {
        Vector3 pvel = _velocities[p];
        auto ppos = _positions[p] + pvel * timestep;
        if(_domainBox.enforceBoundary(ppos, pvel, _params.boundaryRestitution)) {
            _velocities[p] = pvel;
        }
        _positions[p] = ppos;
    });
}

}}
