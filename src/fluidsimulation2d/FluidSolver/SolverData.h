#ifndef Magnum_Examples_FluidSimulation2D_SolverData_h
#define Magnum_Examples_FluidSimulation2D_SolverData_h
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

    UnsignedInt size() const { return static_cast<UnsignedInt>(positions.size()); }

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
        for(UnsignedInt p = 0, pend = size(); p < pend; ++p) {
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
    GridData(const Vector2& origin_, Float cellSize_, Int nI_, Int nJ_) :
        origin{origin_}, nI{nI_}, nJ{nJ_},
        cellSize{cellSize_},
        invCellSize{1.0f/cellSize}
    {
        u.resize(nI + 1, nJ);
        v.resize(nI, nJ + 1);
        uTmp.resize(nI + 1, nJ);
        vTmp.resize(nI, nJ + 1);
        uWeights.resize(nI + 1, nJ);
        vWeights.resize(nI, nJ + 1);
        uValid.resize(nI + 1, nJ);
        vValid.resize(nI, nJ + 1);
        uOldValid.resize(nI + 1, nJ);
        vOldValid.resize(nI, nJ + 1);

        fluidSDF.resize(nI, nJ);
        boundarySDF.resize(nI + 1, nJ + 1);
        cellParticles.resize(nI, nJ);
    }

    Vector2 getGridPos(const Vector2& worldPos) const {
        return (worldPos - origin)*invCellSize;
    }
    Vector2 getWorldPos(const Vector2& gridPos) const {
        return gridPos*cellSize + origin;
    }

    bool isValidCellIdx(Int x, Int y) const {
        return x >= 0 && x < nI && y >= 0 && y < nJ;
    }
    Vector2i getCellIdx(const Vector2& worldPos) const {
        return Vector2i(getGridPos(worldPos));
    }
    Vector2i getValidCellIdx(const Vector2& worldPos) const {
        Vector2i tmp = getCellIdx(worldPos);
        tmp.x() = Math::max(0, Math::min(nI - 1, tmp.x()));
        tmp.y() = Math::max(0, Math::min(nJ - 1, tmp.y()));
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
        const Float sdfVal = boundarySDF.interpolateValue(gridPos);
        if(sdfVal < 0) {
            const Vector2 normal = boundarySDF.interpolateGradient(gridPos);
            return worldPos - sdfVal*normal;
        } else {
            return worldPos;
        }
    }

    template<class Function> void loopNeigborParticles(Int i, Int j, Int il, Int ih, Int jl, Int jh, Function&& func) const {
        for(Int sj = j + jl; sj <= j + jh; ++sj) {
            for(Int si = i + il; si <= i + ih; ++si) {
                if(si < 0 || si > nI - 1 || sj < 0 || sj > nJ - 1)
                    continue;

                const std::vector<UnsignedInt>& neighbors = cellParticles(si, sj);
                for(auto p : neighbors) {
                    func(p);
                }
            }
        }
    }

    /* Grid spatial information */
    const Vector2 origin;
    const Int nI, nJ;
    const Float cellSize;
    const Float invCellSize;

    /* Nodes and cells' data */
    Array2X<Float> u, uTmp, uWeights;
    Array2X<Float> v, vTmp, vWeights;
    Array2X<char> uValid, vValid, uOldValid, vOldValid;
    Array2X<Float> boundarySDF;
    Array2X<Float> fluidSDF;

    Array2X<std::vector<UnsignedInt>> cellParticles;
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
        if(!pcgSolver.solve(matrix, rhs, solution)) {
            Error{} << "Pressure solve failed!";
        }
    }

    /* Use double for linear system (the solver converges slower if using float
       numbers) */
    using pcg_real = Double;
    PCGSolver<pcg_real> pcgSolver;
    SparseMatrix<pcg_real> matrix;
    std::vector<pcg_real> rhs;
    std::vector<pcg_real> solution;
};

}}

#endif
