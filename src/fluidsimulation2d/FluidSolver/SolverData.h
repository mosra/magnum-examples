#ifndef Magnum_Examples_FluidSimulation2D_SolverData_h
#define Magnum_Examples_FluidSimulation2D_SolverData_h
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

#include <vector>
#include <Magnum/Math/Matrix.h>

#include "DataStructures/Array2X.h"
#include "DataStructures/SDFObject.h"
#include "DataStructures/PCGSolver.h"

namespace Magnum { namespace Examples {
struct SceneObjects {
    SDFObject emitterT0; /* emitter that is called once upon initialization */
    SDFObject emitter;   /* emitter that is called by user when requested */
    SDFObject boundary;  /* solid boundary */
};

struct ParticleData {
    explicit ParticleData(Float cellSize) : particleRadius{cellSize* 0.5f} {}

    uint32_t size() const { return static_cast<uint32_t>(positions.size()); }

    void addParticles(const std::vector<Vector2>& newParticles, Float initialVelocity_y) {
        if(positionsT0.size() == 0) {
            positionsT0 = newParticles;
        }
        positions.insert(positions.end(), newParticles.begin(), newParticles.end());
        velocities.resize(size(), Vector2(0, -initialVelocity_y));
        affineMat.resize(size(), Matrix2x2(0));
        tmp.resize(size(), Vector2(0));
    }

    void reset() {
        positions.resize(0);
        velocities.resize(0);
        affineMat.resize(0);
        tmp.resize(0);
    }

    template<class Function>
    void loopAll(Function&& func) const {
        for(uint32_t p = 0, pend = size(); p < pend; ++p) {
            func(p);
        }
    }

    const Float            particleRadius;
    std::vector<Vector2>   positionsT0;
    std::vector<Vector2>   positions;
    std::vector<Vector2>   velocities;
    std::vector<Matrix2x2> affineMat;
    std::vector<Vector2>   tmp;
};

struct GridData {
    GridData(const Vector2& origin_, Float cellSize_, Int Ni_, Int Nj_) :
        origin{origin_}, Ni{Ni_}, Nj{Nj_},
        cellSize{cellSize_},
        invCellSize{Float(1.0) / cellSize} {
        u.resize(Ni + 1, Nj);
        v.resize(Ni, Nj + 1);
        u_tmp.resize(Ni + 1, Nj);
        v_tmp.resize(Ni, Nj + 1);
        u_weights.resize(Ni + 1, Nj);
        v_weights.resize(Ni, Nj + 1);
        u_valid.resize(Ni + 1, Nj);
        v_valid.resize(Ni, Nj + 1);
        u_old_valid.resize(Ni + 1, Nj);
        v_old_valid.resize(Ni, Nj + 1);

        fluidSDF.resize(Ni, Nj);
        boundarySDF.resize(Ni + 1, Nj + 1);
        cellParticles.resize(Ni, Nj);
    }

    Vector2 getGridPos(const Vector2& worldPos) const { return (worldPos - origin) * invCellSize; }
    Vector2 getWorldPos(Float grid_x, Float grid_y) const { return Vector2(grid_x, grid_y) * cellSize + origin; }

    bool isValidCellIdx(int x, int y) const { return x >= 0 && x < Ni && y >= 0 && y < Nj; }
    Vector2i getCellIdx(const Vector2& worldPos) const { return Vector2i(getGridPos(worldPos)); }
    Vector2i getValidCellIdx(const Vector2& worldPos) const {
        auto tmp = getCellIdx(worldPos);
        tmp.x() = Math::max(0, Math::min(Ni - 1, tmp.x()));
        tmp.y() = Math::max(0, Math::min(Nj - 1, tmp.y()));
        return tmp;
    }

    Vector2 velocityFromGridPos(const Vector2& gridPos) const {
        const Vector2 px = Vector2(gridPos[0], gridPos[1] - 0.5f);
        const Vector2 py = Vector2(gridPos[0] - 0.5f, gridPos[1]);
        return Vector2(u.interpolateValue(px),
                       v.interpolateValue(py));
    }

    Vector2 constrainBoundary(const Vector2& worldPos) const {
        const Vector2 gridPos = getGridPos(worldPos);
        const auto    sdfVal  = boundarySDF.interpolateValue(gridPos);
        if(sdfVal < 0) {
            const auto normal = boundarySDF.interpolateGradient(gridPos);
            return worldPos - sdfVal * normal;
        } else {
            return worldPos;
        }
    }

    template<class Function>
    void loopNeigborParticles(int i, int j, int il, int ih, int jl, int jh, Function&& func) const {
        for(int sj = j + jl; sj <= j + jh; ++sj) {
            for(int si = i + il; si <= i + ih; ++si) {
                if(si < 0 || si > Ni - 1 || sj < 0 || sj > Nj - 1) { continue; }
                const auto& neighbors = cellParticles(si, sj);
                for(auto p : neighbors) {
                    func(p);
                }
            }
        }
    }

    /* Grid spatial information */
    const Vector2 origin;
    const Int     Ni, Nj;
    const Float   cellSize;
    const Float   invCellSize;

    /* Nodes and cells' data */
    Array2X<Float> u, u_tmp, u_weights;
    Array2X<Float> v, v_tmp, v_weights;
    Array2X<char>  u_valid, v_valid, u_old_valid, v_old_valid;
    Array2X<Float> boundarySDF;
    Array2X<Float> fluidSDF;

    Array2X<std::vector<uint32_t>> cellParticles;
};

struct LinearSystemSolver {
    void resize(std::size_t newSize) {
        rhs.resize(newSize);
        solution.resize(newSize);
        matrix.resize(newSize);
    }

    void clear() {
        matrix.clear();
        solution.assign(solution.size(), 0);
    }

    void solve() {
        bool bSuccess = pcgSolver.solve(matrix, rhs, solution);
        if(!bSuccess) {
            Error{} << "Pressure solve failed!";
        }
    }

    /* Use double for linear system (the solver converges slower if using float number) */
    using pcg_real = Double;
    PCGSolver<pcg_real>    pcgSolver;
    SparseMatrix<pcg_real> matrix;
    std::vector<pcg_real>  rhs;
    std::vector<pcg_real>  solution;
};
} }

#endif
