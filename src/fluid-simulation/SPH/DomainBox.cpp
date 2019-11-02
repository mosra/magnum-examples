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

#include "DomainBox.h"
#include "TaskScheduler.h"

#include <random>

namespace Magnum { namespace Examples {
/****************************************************************************************************/
DomainBox::DomainBox(float particleRadius, const Vector3& lowerCorner, const Vector3& upperCorner) :
    _lowerDomainBound(lowerCorner), _upperDomainBound(upperCorner),
    _cellLength(particleRadius * 4.0f),
    _maxDistSqr(particleRadius * particleRadius * 16.0f),
    _invCellLength(1.0f / _cellLength),
    _particleRadius(particleRadius),
    _overlappedDistSqr(particleRadius * particleRadius * 1.0e-8f) {
    generateBoundaryParticles();
}

/****************************************************************************************************/
void DomainBox::findNeighbors(const std::vector<Vector3>&         positions,
                              std::vector<std::vector<uint32_t>>& neighbors,
                              std::vector<std::vector<Vector3>>&  relativePositions) {
    /* Collect particle indices into cells */
    collectIndices(positions);

    /*
     * Only consider ghost boundary particles if the fluid particle is within boundaryDist to the boundary
     * boundaryDist = 2*particleRadius because ghost boundary particles are at 2*particleRadius outside the boundary
     */
    const auto boundaryDist = 2.0f * _particleRadius;
    const auto lower_x      = _lowerDomainBound[0] + boundaryDist;
    const auto upper_x      = _upperDomainBound[0] - boundaryDist;
    const auto lower_y      = _lowerDomainBound[1] + boundaryDist;
    const auto lower_z      = _lowerDomainBound[2] + boundaryDist;
    const auto upper_z      = _upperDomainBound[2] - boundaryDist;

    TaskScheduler::for_each(
        positions.size(), [&](uint64_t p) {
            auto ppos           = positions[p];
            auto& pNeighbors    = neighbors[p];
            auto& pRelPositions = relativePositions[p];

            /* Clear old lists */
            pNeighbors.resize(0);
            pRelPositions.resize(0);

            const auto cellIdx = getCellIndex(ppos);
            for(int k = -1; k <= 1; ++k) {
                int zIdx = cellIdx[2] + k;
                if(!isValidIndex<2>(zIdx)) {
                    continue;
                }
                for(int j = -1; j <= 1; ++j) {
                    int yIdx = cellIdx[1] + j;
                    if(!isValidIndex<1>(yIdx)) {
                        continue;
                    }
                    for(int i = -1; i <= 1; ++i) {
                        int xIdx = cellIdx[0] + i;
                        if(!isValidIndex<0>(xIdx)) {
                            continue;
                        }

                        const auto& cell = _cells[getFlatIndex(xIdx, yIdx, zIdx)];
                        for(auto q : cell) {
                            if(static_cast<uint32_t>(p) == q) { /* Exclude particle p from its neighbor list */
                                continue;
                            }
                            const auto qpos = positions[q];
                            const auto r    = ppos - qpos;
                            const auto l2   = r.dot();
                            if(l2 < _maxDistSqr && l2 > _overlappedDistSqr) {
                                pNeighbors.push_back(q);
                                pRelPositions.push_back(r);
                            }
                        }
                    }
                }
            }

            /* Check with ghost boundary particles to fix the inherent SPH's problem of density deficiency at boundary
             * If a particle is closed to the boundary, compute the relative position with the boundary particles
             * There is no need to collect boundary particle indices.
             */

            if(ppos[1] < lower_y) { /* Don't need to consider the top face of the domain box */
                const auto transPos = ppos - Vector3(_cellLength * std::floor(ppos[0] * _invCellLength),
                                                     0,
                                                     _cellLength * std::floor(ppos[2] * _invCellLength));
                const auto y = _lowerDomainBound[1] - 2.0f * _particleRadius;
                for(const auto& bdpos : _boundaryParticles) {
                    const auto r  = transPos - Vector3(bdpos[0], y + bdpos[2], bdpos[1]);
                    const auto l2 = r.dot();
                    if(l2 < _maxDistSqr && l2 > _overlappedDistSqr) {
                        pRelPositions.push_back(r);
                    }
                }
            }

            if(ppos[0] < lower_x || ppos[0] > upper_x) {
                const auto transPos = ppos - Vector3(0,
                                                     _cellLength * std::floor(ppos[1] * _invCellLength),
                                                     _cellLength * std::floor(ppos[2] * _invCellLength));
                const auto x = ppos[0] < lower_x ? _lowerDomainBound[0] - 2.0f * _particleRadius :
                               _upperDomainBound[0] + 2.0f * _particleRadius;
                for(const auto& bdpos : _boundaryParticles) {
                    const auto r  = transPos - Vector3(x + bdpos[2], bdpos[0], bdpos[1]);
                    const auto l2 = r.dot();
                    if(l2 < _maxDistSqr && l2 > _overlappedDistSqr) {
                        pRelPositions.push_back(r);
                    }
                }
            }

            if(ppos[2] < lower_z || ppos[2] > upper_z) {
                const auto transPos = ppos - Vector3(_cellLength * std::floor(ppos[0] * _invCellLength),
                                                     _cellLength * std::floor(ppos[1] * _invCellLength),
                                                     0);
                const auto z = ppos[2] < lower_z ? _lowerDomainBound[2] - 2.0f * _particleRadius :
                               _upperDomainBound[2] + 2.0f * _particleRadius;
                for(const auto& bdpos : _boundaryParticles) {
                    const auto r  = transPos - Vector3(bdpos[0], bdpos[1], z + bdpos[2]);
                    const auto l2 = r.dot();
                    if(l2 < _maxDistSqr && l2 > _overlappedDistSqr) {
                        pRelPositions.push_back(r);
                    }
                }
            }
        });
}

/****************************************************************************************************/
void DomainBox::generateBoundaryParticles() {
    /* Generate 1 layer of ghots boundary particles for 3x3 cells */
    static constexpr int N       = 8;
    const auto           spacing = 3.0f * _cellLength / static_cast<float>(N);
    const auto           corner  = Vector2(-_cellLength + spacing * 0.5f);

    std::random_device                    rd;
    std::mt19937                          gen(rd());
    std::uniform_real_distribution<float> distr(-_particleRadius, _particleRadius);

    for(int l1 = 0; l1 < N; ++l1) {
        for(int l2 = 0; l2 < N; ++l2) {
            const auto pos2D = corner + Vector2(l1, l2) * spacing;
            _boundaryParticles.emplace_back(
                Vector3(pos2D[0] + distr(gen) * 0.2f, /* Store jittered position in 2D    */
                        pos2D[1] + distr(gen) * 0.2f, /* Store jittered position in 2D    */
                        distr(gen) * 0.1f));          /* Store just some jittering amount */
        }
    }
}

/****************************************************************************************************/
void DomainBox::collectIndices(const std::vector<Vector3>& positions) {
    if(positions.size() == 0) {
        return;
    }

    /* Resize grid to tightenly enclose the particles */
    tightenGrid(positions);

    /* Clear cells */
    TaskScheduler::for_each(_cells.size(), [&](size_t idx) { _cells[idx].resize(0); });

    /* Collect particle indices into cells */
    uint32_t nParticles = static_cast<uint32_t>(positions.size());
    for(uint32_t p = 0; p < nParticles; ++p) {
        const auto cellIdx     = getCellIndex(positions[p]);
        const auto cellFlatIdx = getFlatIndex(cellIdx[0], cellIdx[1], cellIdx[2]);
        _cells[cellFlatIdx].push_back(p);
    }
}

/****************************************************************************************************/
void DomainBox::tightenGrid(const std::vector<Vector3>& positions) {
    if(positions.size() == 0) {
        return;
    }

    /* Compute the particles axis-aligned bounding box */
    auto lowerBound = positions[0];
    auto upperBound = positions[0];
    for(size_t p = 1; p < positions.size(); ++p) {
        const auto ppos = positions[p];
        for(size_t i = 0; i < 3; ++i) {
            lowerBound[i] = Math::min(lowerBound[i], ppos[i]);
            upperBound[i] = Math::max(upperBound[i], ppos[i]);
        }
    }

    /* Important: we must slightly expand the upper bound to ensure that the grid cells cover all particles
     * Otherwise, the particles at upper bounds may be outside of the grid
     */
    upperBound += Vector3(_cellLength * 0.1f);

    /* Store the bounds */
    _lowerGridBound = lowerBound;
    _upperGridBound = upperBound;

    /* Compute grid 3D sizes then resize the cells array */
    uint32_t numCells = 1;
    for(size_t i = 0; i < 3; ++i) {
        _gridSize[i] = static_cast<uint32_t>(Math::ceil((upperBound[i] - lowerBound[i]) * _invCellLength));
        if(_gridSize[i] == 0) {
            _gridSize[i] = 1;
        }
        numCells *= _gridSize[i];
    }

    _cells.resize(numCells);
}

/****************************************************************************************************/
bool DomainBox::enforceBoundary(Vector3& ppos, Vector3& pvel, float restitution) {
    /* Enforce particle position to be within the given boundary */
    bool bVelChanged { false };
    for(size_t i = 0; i < 3; ++i) {
        if(ppos[i] < _lowerDomainBound[i]) {
            ppos[i]    += (_lowerDomainBound[i] - ppos[i]) * (restitution + 1.0f);
            pvel[i]    *= -restitution;
            bVelChanged = true;
        } else if(ppos[i] > _upperDomainBound[i]) {
            ppos[i]    -= (ppos[i] - _upperDomainBound[i]) * (restitution + 1.0f);
            pvel[i]    *= -restitution;
            bVelChanged = true;
        }
    }
    return bVelChanged;
}

/****************************************************************************************************/
} } /* namespace Magnum::Examples  */
