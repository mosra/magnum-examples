#ifndef Magnum_Examples_FluidSimulation2D_ApicSolver2D_h
#define Magnum_Examples_FluidSimulation2D_ApicSolver2D_h
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

#include <Corrade/Containers/Pointer.h>

#include "FluidSolver/SolverData.h"

namespace Magnum { namespace Examples {

/* 2D Affine Particle-in-Cell fluid solver */
class ApicSolver2D {
public:
    explicit ApicSolver2D(const Vector2& origin, Float cellSize, Int Ni, Int Nj, SceneObjects* sceneObjs);

    /* Manipulation */
    void reset() {
        _particles.reset();
        _particles.addParticles(_particles.positionsT0, 0);
    }

    void emitParticles() { generateParticles(_objects->emitter, 10); }

    void addRepulsiveVelocity(const Vector2& p0, const Vector2& p1, Float dt, Float radius, Float magnitude);

    void advanceFrame(Float frameDuration);

    /* Properties */
    UnsignedInt numParticles() const { return _particles.size(); }

    Float particleRadius() const { return _particles.particleRadius; }

    const std::vector<Vector2>& particlePositions() const {
        return _particles.positions;
    }

private:
    /* Initialization */
    void initBoundary();
    void generateParticles(const SDFObject& sdfObj, Float initialVelocity_y);

    /* Simulation */
    Float timestepCFL() const;
    void moveParticles(Float dt);
    void collectParticlesToCells();
    void particleVelocity2Grid();
    void extrapolate(Array2X<Float>& grid, Array2X<Float>& tmp_grid, Array2X<char>& valid, Array2X<char>& old_valid) const;
    void addGravity(Float dt);
    void computeFluidSDF();
    void solvePressures(Float dt);
    void constrainVelocity();
    void relaxParticlePositions(Float dt);
    void gridVelocity2Particle();

    Containers::Pointer<SceneObjects> _objects;
    ParticleData _particles;
    GridData _grid;
    LinearSystemSolver _pressureSolver;
};

}}

#endif
