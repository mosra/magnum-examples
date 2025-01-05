#ifndef Magnum_Examples_FluidSimulation3D_SPH_SPHSolver_h
#define Magnum_Examples_FluidSimulation3D_SPH_SPHSolver_h
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
#include <Magnum/Magnum.h>

#include "SPH/SPHKernels.h"
#include "SPH/DomainBox.h"

namespace Magnum { namespace Examples {

struct SPHParams {
    Float stiffness = 20000.0f;
    Float viscosity = 0.05f;
    Float boundaryRestitution = 0.5f;
};

/* This is a very basic implementation of SPH (Smoothed Particle Hydrodynamics)
   solver. For the purpose of fast running (as this is a real-time application
   example), accuracy has been heavily sacrificed for performance. */
class SPHSolver {
    public:
        explicit SPHSolver(Float particleRadius);

        void setPositions(const std::vector<Vector3>& particlePositions);
        void reset();
        void advance();

        DomainBox& domainBox() { return _domainBox; }
        SPHParams& simulationParameters() { return _params; }

        std::size_t numParticles() const { return _positions.size(); }
        const std::vector<Vector3>& particlePositions() { return _positions; }

    private:
        void computeDensities();
        void velocityIntegration(Float timestep);
        void computeViscosity();
        void updatePositions(Float timestep);

        /* Rest density of fluid */
        constexpr static Float RestDensity = 1000.0f;

        /* Particle radius = half distance between consecutive particles */
        const Float _particleRadius;
        /* particle_mass = pow(particle_spacing, 3) * rest_density */
        const Float _particleMass;

        std::vector<Vector3> _positions;
        std::vector<Vector3> _positionsT0; /* Initial positions */

        /* For saving memory thus gaining performance,
           particle densities (float32) are clamped to [0, 10000] then cast
           into uint16_t */
        std::vector<uint16_t> _densities;

        /* Other particle states */
        std::vector<std::vector<uint32_t>> _neighbors;
        std::vector<std::vector<Vector3>>  _relPositions;
        std::vector<Vector3> _velocities;
        std::vector<Vector3> _velocityDiffusions;

        /* SPH kernels */
        SPHKernels _kernels;

        /* Boundary */
        DomainBox _domainBox;

        /* Parameters */
        SPHParams _params;
};

}}

#endif
