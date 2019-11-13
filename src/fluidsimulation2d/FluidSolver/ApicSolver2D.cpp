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

#include "FluidSolver/ApicSolver2D.h"

#include <random>

namespace Magnum { namespace Examples {
ApicSolver2D::ApicSolver2D(const Vector2& origin, Float cellSize, Int Ni, Int Nj, SceneObjects* sceneObjs) :
    _objects{sceneObjs},
    _particles{cellSize},
    _grid{origin, cellSize, Ni, Nj} {
    if(Ni < 1 || Nj < 1) {
        Fatal{} << "Invalid grid resolution";
    }
    if(!sceneObjs) {
        Fatal{} << "Invalid scene object";
    }

    /* Initialize data */
    initBoundary();
    generateParticles(_objects->emitterT0, 0);
}

/* This function should be called again every time the boundary changes */
void ApicSolver2D::initBoundary() {
    _grid.boundarySDF.loop2D(
        [&](std::size_t i, std::size_t j) {
            _grid.boundarySDF(i, j) = _objects->boundary.signedDistance(_grid.getWorldPos(i, j));
        });

    /* Initialize the fluid cell weights from boundary signed distance field */
    _grid.u_weights.loop2D(
        [&](std::size_t i, std::size_t j) {
            _grid.u_weights(i, j) = Float(1) - fractionInside(_grid.boundarySDF(i, j + 1), _grid.boundarySDF(i, j));
            _grid.u_weights(i, j) = Math::clamp(_grid.u_weights(i, j), Float(0), Float(1));
        });
    _grid.v_weights.loop2D(
        [&](std::size_t i, std::size_t j) {
            _grid.v_weights(i, j) = Float(1) - fractionInside(_grid.boundarySDF(i + 1, j), _grid.boundarySDF(i, j));
            _grid.v_weights(i, j) = Math::clamp(_grid.v_weights(i, j), Float(0), Float(1));
        });
}

void ApicSolver2D::generateParticles(const SDFObject& sdfObj, Float initialVelocity_y) {
    using Distribution = std::uniform_real_distribution<Float>;
    const Float  rndScale = _particles.particleRadius * 0.5f;
    std::mt19937 gen((std::random_device{})());
    Distribution distr(-rndScale, rndScale);

    /* Generate new particles */
    std::vector<Vector2> newParticles;
    _grid.fluidSDF.loop2D(
        [&](std::size_t i, std::size_t j) {
            const Vector2 cellCenter = _grid.getWorldPos(i + 0.5f, j + 0.5f);
            for(Int k = 0; k < 2; ++k) {
                const Vector2 ppos = cellCenter + Vector2(distr(gen), distr(gen));
                if(sdfObj.signedDistance(ppos) < 0) {
                    newParticles.push_back(ppos);
                }
            }
        });

    /* Insert into the system */
    _particles.addParticles(newParticles, initialVelocity_y);
}

void ApicSolver2D::addRepulsiveVelocity(const Vector2& p0, const Vector2& p1, Float dt, Float radius, Float magnitude) {
    Vector2    movingVel  = p1 - p0;
    const auto movingDist = movingVel.length();
    if(movingDist < _particles.particleRadius) {
        return;
    }
    movingVel /= dt;

    Vector2 from, to;
    for(std::size_t i = 0; i < 2; ++i) {
        from[i] = Math::min(p0[i], p1[i]);
        to[i]   = Math::max(p0[i], p1[i]);
    }

    const Int      span       = Int(radius / _grid.cellSize);
    const Vector2i fromCell   = _grid.getCellIdx(from) - Vector2i(span);
    const Vector2i toCell     = _grid.getCellIdx(to) + Vector2i(span);
    const Vector2  p01        = p1 - p0;
    const Float    p01DistSqr = p01.dot();

    const auto distToSegment =
        [&](const Vector2& pos) {
            const Float   t   = Math::max(0.0f, Math::min(1.0f, Math::dot(pos - p0, p01) / p01DistSqr));
            const Vector2 prj = p0 + t * p01;
            return (pos - prj).length();
        };

    for(Int j = fromCell.y(); j <= toCell.y(); ++j) {
        for(Int i = fromCell.x(); i <= toCell.x(); ++i) {
            if(!_grid.isValidCellIdx(i, j)) {
                continue;
            }

            const auto& particleIdxs = _grid.cellParticles(i, j);
            for(auto p : particleIdxs) {
                const auto dist = distToSegment(_particles.positions[p]);
                const auto t    = dist / radius;
                if(t < 1.0f) {
                    const auto w = Math::lerp(0.0f, magnitude, t);
                    _particles.velocities[p] += movingVel * w;
                }
            }
        }
    }
}

void ApicSolver2D::advanceFrame(Float frameDuration) {
    Float frameTime = 0;

    while(frameTime < frameDuration) {
        auto       substep       = timestepCFL();
        const auto remainingTime = frameDuration - frameTime;
        if(frameTime + substep > frameDuration) {
            substep = remainingTime;
        } else if(frameTime + Float(1.5) * substep > frameDuration) {
            substep = remainingTime * Float(0.5);
        }
        frameTime += substep;

        /* Advect particles */
        moveParticles(substep);

        /* Particles => grid */
        collectParticlesToCells();
        particleVelocity2Grid();

        /* Update grid velocity */
        extrapolate(_grid.u, _grid.u_tmp, _grid.u_valid, _grid.u_old_valid);
        extrapolate(_grid.v, _grid.v_tmp, _grid.v_valid, _grid.v_old_valid);
        addGravity(substep);
        computeFluidSDF();
        solvePressures(substep);

        /* Enforce boundary condition */
        constrainVelocity();

        /* Grid => particles */
        relaxParticlePositions(substep);
        gridVelocity2Particle();
    }
}

Float ApicSolver2D::timestepCFL() const {
    Float maxVel = 0;
    _grid.u.loop1D([&](std::size_t i) { maxVel = Math::max(maxVel, std::abs(_grid.u.data()[i])); });
    _grid.v.loop1D([&](std::size_t i) { maxVel = Math::max(maxVel, std::abs(_grid.v.data()[i])); });
    return maxVel > 0 ? _grid.cellSize / maxVel * 3.0f : 1.0f;
}

void ApicSolver2D::moveParticles(Float dt) {
    _particles.loopAll(
        [&](uint32_t p) {
            const Vector2 newPos    = _particles.positions[p] + _particles.velocities[p] * dt;
            _particles.positions[p] = _grid.constrainBoundary(newPos);
        });
}

void ApicSolver2D::collectParticlesToCells() {
    _grid.cellParticles.loop1D([&](std::size_t i) { _grid.cellParticles.data()[i].resize(0); });
    _particles.loopAll([&](uint32_t p) {
                           const Vector2 ppos       = _particles.positions[p];
                           const Vector2i gridCoord = _grid.getValidCellIdx(ppos);
                           _grid.cellParticles(gridCoord).push_back(p);
                       });
}

void ApicSolver2D::particleVelocity2Grid() {
    _grid.u.loop2D(
        [&](std::size_t i, std::size_t j) {
            Float sum_w           = 0.0f;
            Float sum_u           = 0.0f;
            const Vector2 nodePos = _grid.getWorldPos(i, j + 0.5f);
            _grid.loopNeigborParticles(static_cast<Int>(i), static_cast<Int>(j), -1, 0, -1, 1, [&](uint32_t p) {
                                           const Vector2 xpg = nodePos - _particles.positions[p];
                                           const auto w      = linearKernel(xpg, _grid.invCellSize);
                                           if(w > 0) {
                                               sum_w += w;
                                               sum_u += w * (_particles.velocities[p].x() +
                                                             Math::dot(_particles.affineMat[p][0], xpg));
                                           }
                                       });
            _grid.u(i, j)       = sum_w > 0 ? sum_u / sum_w : 0.0f;
            _grid.u_valid(i, j) = sum_w > 0 ? 1 : 0;
        });

    _grid.v.loop2D(
        [&](std::size_t i, std::size_t j) {
            Float sum_w           = 0.0;
            Float sum_v           = 0.0;
            const Vector2 nodePos = _grid.getWorldPos(i + 0.5f, j);
            _grid.loopNeigborParticles(static_cast<Int>(i), static_cast<Int>(j), -1, 1, -1, 0, [&](uint32_t p) {
                                           const Vector2 xpg = nodePos - _particles.positions[p];
                                           const auto w      = linearKernel(xpg, _grid.invCellSize);
                                           if(w > 0) {
                                               sum_w += w;
                                               sum_v += w * (_particles.velocities[p].y() +
                                                             Math::dot(_particles.affineMat[p][1], xpg));
                                           }
                                       });
            _grid.v(i, j)       = sum_w > 0 ? sum_v / sum_w : 0.0f;
            _grid.v_valid(i, j) = sum_w > 0 ? 1 : 0;
        });
}

void ApicSolver2D::extrapolate(Array2X<Float>& grid, Array2X<Float>& tmp_grid, Array2X<char>& valid, Array2X<char>& old_valid) const {
    tmp_grid  = grid;
    old_valid = valid;

    Array2X<Float>* pgrids[]  = { &grid, &tmp_grid };
    Array2X<char>*  pvalids[] = { &valid, &old_valid };

    for(int layers = 0; layers < 1; ++layers) {
        auto pgrid_src = pgrids[layers & 1];
        auto pgrid_tgt = pgrids[!(layers & 1)];

        auto pvalid_src = pvalids[layers & 1];
        auto pvalid_tgt = pvalids[!(layers & 1)];

        grid.loop2D([&](std::size_t i, std::size_t j) {
                        if(i == 0 || i == grid.size_x() - 1 ||
                           j == 0 || j == grid.size_y() - 1) {
                            return;
                        }

                        Float sum = 0;
                        Int count = 0;

                        if(!(*pvalid_src)(i, j)) {
                            const std::size_t rows[] = { i + 1, i - 1, i, i };
                            const std::size_t cols[] = { j, j, j + 1, j - 1 };

                            for(std::size_t cell = 0; cell < 4; ++cell) {
                                if((*pvalid_src)(rows[cell], cols[cell])) {
                                    sum += (*pgrid_src)(rows[cell], cols[cell]);
                                    ++count;
                                }
                            }

                            if(count > 0) {
                                (*pgrid_tgt)(i, j)  = sum / static_cast<Float>(count);
                                (*pvalid_tgt)(i, j) = 1;
                            }
                        }
                    });

        (*pgrid_src).swapContent(*pgrid_tgt);
        (*pvalid_src).swapContent(*pvalid_tgt);
    }
}

void ApicSolver2D::addGravity(Float dt) {
    _grid.v.loop2D(
        [&](std::size_t i, std::size_t j) {
            if(_grid.v_valid(i, j)) {
                _grid.v(i, j) -= 9.81f * dt; /* gravity */
            }
        });
}

void ApicSolver2D::computeFluidSDF() {
    _grid.fluidSDF.assign(3 * _grid.cellSize);

    _particles.loopAll(
        [&](uint32_t p) {
            const Vector2 ppos     = _particles.positions[p];
            const Vector2i gridPos = Vector2i(_grid.getGridPos(ppos) - Vector2(0.5));

            for(int j = gridPos.y() - 2; j <= gridPos.y() + 2; ++j) {
                for(int i = gridPos.x() - 2; i <= gridPos.x() + 2; ++i) {
                    if(!_grid.isValidCellIdx(i, j)) {
                        continue;
                    }
                    const Vector2 cellCenter = _grid.getWorldPos(i + 0.5f, j + 0.5f);
                    const Float sdfVal       = (cellCenter - ppos).length() - _particles.particleRadius;
                    if(_grid.fluidSDF(i, j) > sdfVal) {
                        _grid.fluidSDF(i, j) = sdfVal;
                    }
                }
            }
        });

    _grid.fluidSDF.loop2D(
        [&](std::size_t i, std::size_t j) {
            const Vector2 cellCenter = _grid.getWorldPos(i + 0.5f, j + 0.5f);
            const Float sdfVal       = _objects->boundary.signedDistance(cellCenter);
            if(_grid.fluidSDF(i, j) > sdfVal) {
                _grid.fluidSDF(i, j) = sdfVal;
            }
        });
}

void ApicSolver2D::solvePressures(Float dt) {
    const auto Ni       = static_cast<std::size_t>(_grid.Ni);
    const auto Nj       = static_cast<std::size_t>(_grid.Nj);
    const auto numCells = Ni * Nj;

    _pressureSolver.resize(numCells);
    _pressureSolver.clear();

    for(std::size_t j = 1; j < Nj - 1; ++j) {
        for(std::size_t i = 1; i < Ni - 1; ++i) {
            const auto row       = i + Ni * j;
            double     rhsVal    = 0;
            const auto centerSDF = _grid.fluidSDF(i, j);

            if(centerSDF >= 0) {
                _pressureSolver.rhs[row] = rhsVal;
                continue;
            }

            const Float       cellsWeights[] = {
                _grid.u_weights(i + 1, j),
                _grid.u_weights(i, j),
                _grid.v_weights(i, j + 1),
                _grid.v_weights(i, j)
            };
            const Float       cellsSDF[] = {
                _grid.fluidSDF(i + 1, j),
                _grid.fluidSDF(i - 1, j),
                _grid.fluidSDF(i, j + 1),
                _grid.fluidSDF(i, j - 1)
            };
            const Float       cellsVel[] = {
                -_grid.u(i + 1, j), /* minus velocity */
                _grid.u(i, j),
                -_grid.v(i, j + 1), /* minus velocity */
                _grid.v(i, j)
            };
            const std::size_t cols[] = {
                row + 1,
                row - 1,
                row + Ni,
                row - Ni
            };

            /* Fill-in matrix */
            for(std::size_t cell = 0; cell < 4; ++cell) {
                rhsVal += static_cast<double>(cellsWeights[cell] * cellsVel[cell]);
                const auto term = cellsWeights[cell] * dt;
                if(cellsSDF[cell] < 0) {
                    _pressureSolver.matrix.addToElement(row,  row,       term);
                    _pressureSolver.matrix.addToElement(row, cols[cell], -term);
                } else {
                    const auto theta = Math::max(0.01f, fractionInside(centerSDF, cellsSDF[cell]));
                    _pressureSolver.matrix.addToElement(row, row, term / theta);
                }
            }

            /* Write rhs */
            _pressureSolver.rhs[row] = rhsVal;
        }
    }

    _pressureSolver.solve(); /* now solve the linear system for cells' pressure */

    _grid.u.loop2D(
        [&](std::size_t i, std::size_t j) {
            /* Edges of the domain, or entirely in solid */
            if(i == 0 || i == _grid.u.size_x() - 1 || !(_grid.u_weights(i, j) > 0)) {
                _grid.u(i, j) = 0;
                return;
            }

            const auto centerSDF = _grid.fluidSDF(i, j);
            const auto leftSDF   = _grid.fluidSDF(i - 1, j);
            if(centerSDF < 0 || leftSDF < 0) {
                Float theta = 1;
                if(_grid.fluidSDF(i, j) >= 0 || leftSDF >= 0) {
                    theta = Math::max(0.01f, fractionInside(leftSDF, centerSDF));
                }
                const auto row      = i + j * Ni;
                const auto pressure = static_cast<Float>(_pressureSolver.solution[row] - _pressureSolver.solution[row - 1]);
                _grid.u(i, j)      -= pressure * (dt / theta);
            }
        });

    _grid.v.loop2D(
        [&](std::size_t i, std::size_t j) {
            /* Edges of the domain, or entirely in solid */
            if(j == 0 || j == _grid.v.size_y() - 1 || !(_grid.v_weights(i, j) > 0)) {
                _grid.v(i, j) = 0;
                return;
            }

            const auto centerSDF = _grid.fluidSDF(i, j);
            const auto bottomSDF = _grid.fluidSDF(i, j - 1);
            if(centerSDF < 0 || bottomSDF < 0) {
                Float theta = 1;
                if(centerSDF >= 0 || bottomSDF >= 0) {
                    theta = Math::max(0.01f, fractionInside(bottomSDF, centerSDF));
                }
                const auto row      = i + j * Ni;
                const auto pressure = static_cast<Float>(_pressureSolver.solution[row] - _pressureSolver.solution[row - Ni]);
                _grid.v(i, j)      -= pressure * (dt / theta);
            }
        });
}

void ApicSolver2D::constrainVelocity() {
    _grid.u_tmp = _grid.u;
    _grid.v_tmp = _grid.v;

    _grid.u.loop2D(
        [&](std::size_t i, std::size_t j) {
            if(_grid.u_weights(i, j) > 0) { /* not entirely in solid */
                return;
            }
            const Vector2 gridPos = Vector2(i, j + 0.5f);
            const Vector2 normal  = _grid.boundarySDF.interpolateGradient(gridPos);
            Vector2 vel           = _grid.velocityFromGridPos(gridPos);
            Float perp_component  = Math::dot(vel, normal);
            vel -= perp_component * normal;
            _grid.u_tmp(i, j) = vel[0];
        });

    _grid.v.loop2D(
        [&](std::size_t i, std::size_t j) {
            if(_grid.v_weights(i, j) > 0) { /* not entirely in solid */
                return;
            }
            const Vector2 gridPos = Vector2(i + 0.5f, j);
            const Vector2 normal  = _grid.boundarySDF.interpolateGradient(gridPos);
            Vector2 vel           = _grid.velocityFromGridPos(gridPos);
            Float perp_component  = Math::dot(vel, normal);
            vel -= perp_component * normal;
            _grid.v_tmp(i, j) = vel[1];
        });

    /* Only swap u_tmp and v_tmp after constraining both u and v */
    _grid.u.swapContent(_grid.u_tmp);
    _grid.v.swapContent(_grid.v_tmp);
}

void ApicSolver2D::relaxParticlePositions(Float dt) {
    const Float            restDist      = _grid.cellSize / std::sqrt(2.0f) * 1.1f;
    const Float            restDistSqr   = restDist * restDist;
    const Float            overlappedSqr = restDistSqr * 0.0001f;
    const Float            jitterMag     = restDist / dt / 128.0f * 0.01f;
    static constexpr Float stiffness     = 5.0f;

    _particles.loopAll(
        [&](uint32_t p) {
            const Vector2 ppos       = _particles.positions[p];
            const Vector2i gridCoord = _grid.getValidCellIdx(ppos);
            Vector2 spring           = Vector2(0);
            _grid.loopNeigborParticles(
                gridCoord.x(), gridCoord.y(), -1, 1, -1, 1, [&](uint32_t q) {
                    if(p == q) {
                        return;
                    }
                    const Vector2 xpq  = ppos - _particles.positions[q];
                    const auto distSqr = xpq.dot();
                    const auto w       = stiffness * smoothKernel(distSqr, restDistSqr);
                    if(distSqr > overlappedSqr) {
                        spring += xpq * (w / std::sqrt(distSqr) * restDist);
                    } else {
                        spring.x() += ((rand() & 255) - 128) * jitterMag;
                        spring.y() += ((rand() & 255) - 128) * jitterMag;
                    }
                });

            const Vector2 newPos = ppos + dt * spring;
            _particles.tmp[p]    = _grid.constrainBoundary(newPos);
        });

    _particles.positions.swap(_particles.tmp);
}

void ApicSolver2D::gridVelocity2Particle() {
    auto&      u     = _grid.u;
    auto&      v     = _grid.v;
    const auto dxInv = _grid.invCellSize;

    _particles.loopAll(
        [&](uint32_t p) {
            const Vector2 gridPos = _grid.getGridPos(_particles.positions[p]);
            const Vector2 px      = gridPos - Vector2(0, 0.5);
            const Vector2 py      = gridPos - Vector2(0.5, 0);

            _particles.velocities[p] = Vector2(u.interpolateValue(px),
                                               v.interpolateValue(py));
            _particles.affineMat[p] = Matrix2x2(u.affineInterpolateValue(px) * dxInv,
                                                v.affineInterpolateValue(py) * dxInv);
        });
}
} }
