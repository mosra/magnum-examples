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

#include "SPHSolver.h"
#include "TaskScheduler.h"

namespace Magnum { namespace Examples {
/****************************************************************************************************/
SPHSolver::SPHSolver(float particleRadius) :
    _particleRadius(particleRadius),
    _particleMass(std::pow(2.0f * particleRadius, 3.0f) * s_restDensity * 0.9f),
    _kernels(particleRadius * 4.0f),
    _domainBox(particleRadius, Vector3(particleRadius),  Vector3(3, 3, 1) - Vector3(particleRadius)) {}

/****************************************************************************************************/
void SPHSolver::setPositions(const std::vector<Vector3>& positions) {
    _positions    = positions;
    _positions_t0 = positions;

    /* Resize other variables */
    const auto nParticles = positions.size();
    _densities.resize(nParticles);
    _neighbors.resize(nParticles);
    _relPositions.resize(nParticles);
    _velocities.assign(nParticles, Vector3(0)); /* Must initialize zero for all velocities */
    _velocityDiffusions.resize(nParticles);
}

/****************************************************************************************************/
void SPHSolver::reset() {
    _positions = _positions_t0;
    _velocities.assign(numParticles(), Vector3(0)); /* Must initialize zero for all velocities */
}

/****************************************************************************************************/
void SPHSolver::advance() {
    /* Find neighbors and compute relative positions with them */
    _domainBox.findNeighbors(_positions, _neighbors, _relPositions);

    /* This is a fixed time step approach!
     * In practice, adaptive time step should be used
     */
    const auto timestep = 0.05f * _particleRadius;

    computeDensities();
    velocityIntegration(timestep);
    computeViscosity();
    updatePositions(timestep);
}

/****************************************************************************************************/
void SPHSolver::computeDensities() {
    TaskScheduler::for_each(
        _positions.size(),
        [&](const size_t p) {
            const auto& relPositions = _relPositions[p];
            if(relPositions.size() == 0) {
                return;
            }

            auto pdensity = _kernels.W0();
            for(const auto& xpq : relPositions) {
                pdensity += _kernels.W(xpq);
            }
            pdensity *= _particleMass;

            /* Clamp and cast to uint16_t */
            if(pdensity > 10000.0f) {
                pdensity = 10000.0f;
            }
            _densities[p] = static_cast<uint16_t>(std::round(pdensity));
        });
}

/****************************************************************************************************/
void SPHSolver::velocityIntegration(float timestep) {
    auto pressure = [&](const float rho) {
                        const auto ratio = rho / s_restDensity;
                        if(ratio < 1.0f) {
                            return 0.0f;
                        }
                        const auto ratioExp2 = ratio * ratio;
                        const auto ratioExp4 = ratioExp2 * ratioExp2;
                        const auto ratioExp7 = ratioExp4 * ratioExp2 * ratio;
                        return ratioExp7 - 1.0f;
                    };

    TaskScheduler::for_each(
        _positions.size(),
        [&](const size_t p) {
            const auto& neighbors = _neighbors[p];
            if(neighbors.size() == 0) {
                /* A lonely particle only interacts with gravity */
                _velocities[p].y() -= timestep * 9.81f;
                return;
            }

            const auto& relPositions = _relPositions[p];
            const auto pdensity      = static_cast<float>(_densities[p]);
            const auto ppressure     = pressure(pdensity);
            const auto Kp = ppressure / (pdensity * pdensity);
            Vector3 accel(0);

            /* Compute the pressure acceleration caused by normal fluid particles */
            for(size_t idx = 0; idx < neighbors.size(); ++idx) {
                const auto q         = neighbors[idx];
                const auto qdensity  = static_cast<float>(_densities[q]);
                const auto qpressure = pressure(qdensity);
                const auto Kq        = qpressure / (qdensity * qdensity);
                const auto r         = relPositions[idx];

                /* Pressure acceleration */
                accel -= (Kp + Kq) * _kernels.gradW(r);
            }

            /* Compute the pressure acceleration caused by ghost boundary particles */
            for(size_t idx = neighbors.size(); idx < relPositions.size(); ++idx) {
                const auto r = relPositions[idx];
                accel       -= Kp * _kernels.gradW(r);
            }

            accel     *= _params.stiffness * _particleMass;
            accel.y() -= 9.81f; /* add gravity */

            /* Update velocity from acceleration and gravity */
            _velocities[p] += accel * timestep;
        });
}

/****************************************************************************************************/
void SPHSolver::computeViscosity() {
    TaskScheduler::for_each(
        _positions.size(),
        [&](const size_t p) {
            const auto& neighbors = _neighbors[p];
            if(neighbors.size() == 0) {
                _velocityDiffusions[p] = Vector3(0);
                return;
            }

            const auto& relPositions = _relPositions[p];
            const auto pvel          = _velocities[p];

            Vector3 diffuseVel(0);
            for(size_t idx = 0; idx < neighbors.size(); ++idx) {
                const auto q        = neighbors[idx];
                const auto qvel     = _velocities[q];
                const auto qdensity = static_cast<float>(_densities[q]);
                const auto r        = relPositions[idx];

                diffuseVel += (1.0f / qdensity) * _kernels.W(r) * (qvel - pvel);
            }
            diffuseVel *= _params.viscosity * _particleMass;
            _velocityDiffusions[p] = diffuseVel;
        });

    /* Add diffused velocity back to velocity, causing viscosity */
    TaskScheduler::for_each(
        _positions.size(),
        [&](const size_t p) {
            _velocities[p] += _velocityDiffusions[p];
        });
}

/****************************************************************************************************/
void SPHSolver::updatePositions(float timestep) {
    TaskScheduler::for_each(
        _positions.size(),
        [&](const size_t p) {
            auto pvel = _velocities[p];
            auto ppos = _positions[p] + pvel * timestep;
            if(_domainBox.enforceBoundary(ppos, pvel, _params.boundaryRestitution)) {
                _velocities[p] = pvel;
            }
            _positions[p] = ppos;
        });
}

/****************************************************************************************************/
} } /* namespace Magnum::Examples  */
