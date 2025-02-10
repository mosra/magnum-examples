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

#include "FluidSolver/ApicSolver2D.h"

#include <random>

namespace Magnum { namespace Examples {

ApicSolver2D::ApicSolver2D(const Vector2& origin, Float cellSize, Int nI, Int nJ, SceneObjects* sceneObjs):
    _objects{sceneObjs},
    _particles{cellSize},
    _grid{origin, cellSize, nI, nJ}
{
    if(nI < 1 || nJ < 1) {
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
    _grid.boundarySDF.loop2D([&](std::size_t i, std::size_t j) {
        _grid.boundarySDF(i, j) = _objects->boundary.signedDistance(_grid.getWorldPos({Float(i), Float(j)}));
    });

    /* Initialize the fluid cell weights from boundary signed distance field */
    _grid.uWeights.loop2D([&](std::size_t i, std::size_t j) {
        _grid.uWeights(i, j) = Float(1) - fractionInside(_grid.boundarySDF(i, j + 1), _grid.boundarySDF(i, j));
        _grid.uWeights(i, j) = Math::clamp(_grid.uWeights(i, j), Float(0), Float(1));
    });
    _grid.vWeights.loop2D([&](std::size_t i, std::size_t j) {
        _grid.vWeights(i, j) = Float(1) - fractionInside(_grid.boundarySDF(i + 1, j), _grid.boundarySDF(i, j));
        _grid.vWeights(i, j) = Math::clamp(_grid.vWeights(i, j), Float(0), Float(1));
    });
}

void ApicSolver2D::generateParticles(const SDFObject& sdfObj, Float initialVelocity_y) {
    using Distribution = std::uniform_real_distribution<Float>;
    const Float rndScale = _particles.particleRadius*0.5f;
    std::mt19937 gen(std::random_device{}());
    Distribution distr(-rndScale, rndScale);

    /* Generate new particles */
    std::vector<Vector2> newParticles;
    _grid.fluidSDF.loop2D(
        [&](std::size_t i, std::size_t j) {
            const Vector2 cellCenter = _grid.getWorldPos({i + 0.5f, j + 0.5f});
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
    Vector2 movingVel  = p1 - p0;
    const Float movingDist = movingVel.length();
    if(movingDist < _particles.particleRadius)
        return;

    movingVel /= dt;

    Vector2 from, to;
    for(std::size_t i = 0; i < 2; ++i) {
        from[i] = Math::min(p0[i], p1[i]);
        to[i]   = Math::max(p0[i], p1[i]);
    }

    const Int span = Int(radius / _grid.cellSize);
    const Vector2i fromCell = _grid.getCellIdx(from) - Vector2i(span);
    const Vector2i toCell = _grid.getCellIdx(to) + Vector2i(span);
    const Vector2 p01 = p1 - p0;
    const Float p01DistSqr = p01.dot();

    const auto distToSegment = [&](const Vector2& pos) -> Float {
        const Float t = Math::max(0.0f, Math::min(1.0f, Math::dot(pos - p0, p01)/p01DistSqr));
        const Vector2 prj = p0 + t * p01;
        return (pos - prj).length();
    };

    for(Int j = fromCell.y(); j <= toCell.y(); ++j) {
        for(Int i = fromCell.x(); i <= toCell.x(); ++i) {
            if(!_grid.isValidCellIdx(i, j)) continue;

            const std::vector<UnsignedInt>& particleIdxs = _grid.cellParticles(i, j);
            for(UnsignedInt p: particleIdxs) {
                const Float dist = distToSegment(_particles.positions[p]);
                const Float t = dist/radius;
                if(t < 1.0f) {
                    const Float w = Math::lerp(0.0f, magnitude, t);
                    _particles.velocities[p] += movingVel * w;
                }
            }
        }
    }
}

void ApicSolver2D::advanceFrame(Float frameDuration) {
    Float frameTime = 0;

    while(frameTime < frameDuration) {
        Float substep = timestepCFL();
        const Float remainingTime = frameDuration - frameTime;
        if(frameTime + substep > frameDuration)
            substep = remainingTime;
        else if(frameTime + Float(1.5) * substep > frameDuration)
            substep = remainingTime * Float(0.5);
        frameTime += substep;

        /* Advect particles */
        moveParticles(substep);

        /* Particles => grid */
        collectParticlesToCells();
        particleVelocity2Grid();

        /* Update grid velocity */
        extrapolate(_grid.u, _grid.uTmp, _grid.uValid, _grid.uOldValid);
        extrapolate(_grid.v, _grid.vTmp, _grid.vValid, _grid.vOldValid);
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
    _grid.u.loop1D([&](std::size_t i) {
        maxVel = Math::max(maxVel, Math::abs(_grid.u.data()[i]));
    });
    _grid.v.loop1D([&](std::size_t i) {
        maxVel = Math::max(maxVel, Math::abs(_grid.v.data()[i]));
    });
    return maxVel > 0 ? _grid.cellSize/maxVel*3.0f : 1.0f;
}

void ApicSolver2D::moveParticles(Float dt) {
    _particles.loopAll([&](UnsignedInt p) {
        const Vector2 newPos = _particles.positions[p] + _particles.velocities[p]*dt;
        _particles.positions[p] = _grid.constrainBoundary(newPos);
    });
}

void ApicSolver2D::collectParticlesToCells() {
    _grid.cellParticles.loop1D([&](std::size_t i) {
        _grid.cellParticles.data()[i].resize(0);
    });
    _particles.loopAll([&](UnsignedInt p) {
        const Vector2 ppos = _particles.positions[p];
        const Vector2i gridCoord = _grid.getValidCellIdx(ppos);
        _grid.cellParticles(gridCoord).push_back(p);
    });
}

void ApicSolver2D::particleVelocity2Grid() {
    _grid.u.loop2D([&](std::size_t i, std::size_t j) {
        Float sumW = 0.0f;
        Float sumU = 0.0f;
        const Vector2 nodePos = _grid.getWorldPos({Float(i), j + 0.5f});
        _grid.loopNeigborParticles(static_cast<Int>(i), static_cast<Int>(j), -1, 0, -1, 1, [&](UnsignedInt p) {
            const Vector2 xpg = nodePos - _particles.positions[p];
            const auto w      = linearKernel(xpg, _grid.invCellSize);
            if(w > 0) {
                sumW += w;
                sumU += w*(_particles.velocities[p].x() + Math::dot(_particles.affineMat[p][0], xpg));
            }
        });
        _grid.u(i, j) = sumW > 0 ? sumU/sumW : 0.0f;
        _grid.uValid(i, j) = sumW > 0 ? 1 : 0;
    });

    _grid.v.loop2D([&](std::size_t i, std::size_t j) {
        Float sumW = 0.0;
        Float sumV = 0.0;
        const Vector2 nodePos = _grid.getWorldPos({i + 0.5f, Float(j)});
        _grid.loopNeigborParticles(static_cast<Int>(i), static_cast<Int>(j), -1, 1, -1, 0, [&](UnsignedInt p) {
            const Vector2 xpg = nodePos - _particles.positions[p];
            const auto w      = linearKernel(xpg, _grid.invCellSize);
            if(w > 0) {
                sumW += w;
                sumV += w*(_particles.velocities[p].y() + Math::dot(_particles.affineMat[p][1], xpg));
            }
        });
        _grid.v(i, j) = sumW > 0 ? sumV/sumW : 0.0f;
        _grid.vValid(i, j) = sumW > 0 ? 1 : 0;
    });
}

void ApicSolver2D::extrapolate(Array2X<Float>& grid, Array2X<Float>& tmp_grid, Array2X<char>& valid, Array2X<char>& old_valid) const {
    tmp_grid  = grid;
    old_valid = valid;

    Array2X<Float>* pgrids[] = {&grid, &tmp_grid};
    Array2X<char>* pvalids[] = {&valid, &old_valid};

    for(int layers = 0; layers < 1; ++layers) {
        auto& gridSrc = *pgrids[layers & 1];
        auto& gridTgt = *pgrids[!(layers & 1)];

        auto& validSrc = *pvalids[layers & 1];
        auto& validTgt = *pvalids[!(layers & 1)];

        grid.loop2D([&](std::size_t i, std::size_t j) {
            if(i == 0 || i == grid.sizeX() - 1 ||
               j == 0 || j == grid.sizeY() - 1) return;

            Float sum = 0;
            Int count = 0;

            if(!validSrc(i, j)) {
                const std::size_t rows[] = { i + 1, i - 1, i, i };
                const std::size_t cols[] = { j, j, j + 1, j - 1 };

                for(std::size_t cell = 0; cell < 4; ++cell) {
                    if(validSrc(rows[cell], cols[cell])) {
                        sum += gridSrc(rows[cell], cols[cell]);
                        ++count;
                    }
                }

                if(count > 0) {
                    gridTgt(i, j) = sum/Float(count);
                    validTgt(i, j) = 1;
                }
            }
        });

        gridSrc.swapContent(gridTgt);
        validSrc.swapContent(validTgt);
    }
}

void ApicSolver2D::addGravity(Float dt) {
    _grid.v.loop2D([&](std::size_t i, std::size_t j) {
        if(_grid.vValid(i, j)) {
            _grid.v(i, j) -= 9.81f*dt; /* gravity */
        }
    });
}

void ApicSolver2D::computeFluidSDF() {
    _grid.fluidSDF.assign(3 * _grid.cellSize);

    _particles.loopAll([&](UnsignedInt p) {
        const Vector2 ppos = _particles.positions[p];
        const Vector2i gridPos = Vector2i(_grid.getGridPos(ppos) - Vector2(0.5));

        for(Int j = gridPos.y() - 2; j <= gridPos.y() + 2; ++j) {
            for(Int i = gridPos.x() - 2; i <= gridPos.x() + 2; ++i) {
                if(!_grid.isValidCellIdx(i, j)) continue;

                const Vector2 cellCenter = _grid.getWorldPos({i + 0.5f, j + 0.5f});
                const Float sdfVal = (cellCenter - ppos).length() - _particles.particleRadius;
                if(_grid.fluidSDF(i, j) > sdfVal)
                    _grid.fluidSDF(i, j) = sdfVal;
            }
        }
    });

    _grid.fluidSDF.loop2D([&](std::size_t i, std::size_t j) {
        const Vector2 cellCenter = _grid.getWorldPos({i + 0.5f, j + 0.5f});
        const Float sdfVal = _objects->boundary.signedDistance(cellCenter);
        if(_grid.fluidSDF(i, j) > sdfVal)
            _grid.fluidSDF(i, j) = sdfVal;
    });
}

void ApicSolver2D::solvePressures(Float dt) {
    const std::size_t nI = std::size_t(_grid.nI);
    const std::size_t nJ = std::size_t(_grid.nJ);
    const std::size_t numCells = nI * nJ;

    _pressureSolver.resize(numCells);
    _pressureSolver.clear();

    for(std::size_t j = 1; j < nJ - 1; ++j) {
        for(std::size_t i = 1; i < nI - 1; ++i) {
            const std::size_t row = i + nI * j;
            Double rhsVal = 0.0;
            const Float centerSDF = _grid.fluidSDF(i, j);

            if(centerSDF >= 0) {
                _pressureSolver.rhs[row] = rhsVal;
                continue;
            }

            const Float cellsWeights[] = {
                _grid.uWeights(i + 1, j),
                _grid.uWeights(i, j),
                _grid.vWeights(i, j + 1),
                _grid.vWeights(i, j)
            };
            const Float cellsSDF[] = {
                _grid.fluidSDF(i + 1, j),
                _grid.fluidSDF(i - 1, j),
                _grid.fluidSDF(i, j + 1),
                _grid.fluidSDF(i, j - 1)
            };
            const Float cellsVel[] = {
                -_grid.u(i + 1, j), /* minus velocity */
                 _grid.u(i, j),
                -_grid.v(i, j + 1), /* minus velocity */
                 _grid.v(i, j)
            };
            const std::size_t cols[] = {
                row + 1,
                row - 1,
                row + nI,
                row - nI
            };

            /* Fill-in matrix */
            for(std::size_t cell = 0; cell < 4; ++cell) {
                rhsVal += Double(cellsWeights[cell]*cellsVel[cell]);
                const Float term = cellsWeights[cell] * dt;
                if(cellsSDF[cell] < 0.0f) {
                    _pressureSolver.matrix.addToElement(row, row, term);
                    _pressureSolver.matrix.addToElement(row, cols[cell], -term);
                } else {
                    const Float theta = Math::max(0.01f, fractionInside(centerSDF, cellsSDF[cell]));
                    _pressureSolver.matrix.addToElement(row, row, term / theta);
                }
            }

            /* Write rhs */
            _pressureSolver.rhs[row] = rhsVal;
        }
    }

    _pressureSolver.solve(); /* now solve the linear system for cells' pressure */

    _grid.u.loop2D([&](std::size_t i, std::size_t j) {
        /* Edges of the domain, or entirely in solid */
        if(i == 0 || i == _grid.u.sizeX() - 1 || !(_grid.uWeights(i, j) > 0)) {
            _grid.u(i, j) = 0;
            return;
        }

        const Float centerSDF = _grid.fluidSDF(i, j);
        const Float leftSDF   = _grid.fluidSDF(i - 1, j);
        if(centerSDF < 0 || leftSDF < 0) {
            Float theta = 1;
            if(_grid.fluidSDF(i, j) >= 0 || leftSDF >= 0) {
                theta = Math::max(0.01f, fractionInside(leftSDF, centerSDF));
            }
            const std::size_t row = i + j*nI;
            const Float pressure = Float(_pressureSolver.solution[row] - _pressureSolver.solution[row - 1]);
            _grid.u(i, j) -= pressure*(dt/theta);
        }
    });

    _grid.v.loop2D([&](std::size_t i, std::size_t j) {
        /* Edges of the domain, or entirely in solid */
        if(j == 0 || j == _grid.v.sizeY() - 1 || !(_grid.vWeights(i, j) > 0)) {
            _grid.v(i, j) = 0;
            return;
        }

        const Float centerSDF = _grid.fluidSDF(i, j);
        const Float bottomSDF = _grid.fluidSDF(i, j - 1);
        if(centerSDF < 0 || bottomSDF < 0) {
            Float theta = 1;
            if(centerSDF >= 0 || bottomSDF >= 0) {
                theta = Math::max(0.01f, fractionInside(bottomSDF, centerSDF));
            }
            const std::size_t row = i + j*nI;
            const Float pressure = Float(_pressureSolver.solution[row] - _pressureSolver.solution[row - nI]);
            _grid.v(i, j) -= pressure*(dt/theta);
        }
    });
}

void ApicSolver2D::constrainVelocity() {
    _grid.uTmp = _grid.u;
    _grid.vTmp = _grid.v;

    _grid.u.loop2D([&](std::size_t i, std::size_t j) {
        if(_grid.uWeights(i, j) > 0) /* not entirely in solid */
            return;

        const Vector2 gridPos = Vector2(i, j + 0.5f);
        const Vector2 normal = _grid.boundarySDF.interpolateGradient(gridPos);
        Vector2 vel = _grid.velocityFromGridPos(gridPos);
        Float perp_component = Math::dot(vel, normal);
        vel -= perp_component*normal;
        _grid.uTmp(i, j) = vel[0];
    });

    _grid.v.loop2D([&](std::size_t i, std::size_t j) {
        if(_grid.vWeights(i, j) > 0) /* not entirely in solid */
            return;

        const Vector2 gridPos = Vector2(i + 0.5f, j);
        const Vector2 normal  = _grid.boundarySDF.interpolateGradient(gridPos);
        Vector2 vel = _grid.velocityFromGridPos(gridPos);
        Float perp_component = Math::dot(vel, normal);
        vel -= perp_component*normal;
        _grid.vTmp(i, j) = vel[1];
    });

    /* Only swap u_tmp and v_tmp after constraining both u and v */
    _grid.u.swapContent(_grid.uTmp);
    _grid.v.swapContent(_grid.vTmp);
}

void ApicSolver2D::relaxParticlePositions(Float dt) {
    const Float restDist = _grid.cellSize/Constants::sqrt2()*1.1f;
    const Float restDistSqr = restDist*restDist;
    const Float overlappedSqr = restDistSqr*0.0001f;
    const Float jitterMag = restDist/dt/128.0f*0.01f;
    constexpr Float stiffness = 5.0f;

    _particles.loopAll([&](UnsignedInt p) {
        const Vector2 ppos = _particles.positions[p];
        const Vector2i gridCoord = _grid.getValidCellIdx(ppos);
        Vector2 spring = Vector2{0.0f};

        _grid.loopNeigborParticles(gridCoord.x(), gridCoord.y(), -1, 1, -1, 1, [&](UnsignedInt q) {
            if(p == q) return;

            const Vector2 xpq  = ppos - _particles.positions[q];
            const auto distSqr = xpq.dot();
            const auto w       = stiffness*smoothKernel(distSqr, restDistSqr);
            if(distSqr > overlappedSqr) {
                spring += xpq * (w / Math::sqrt(distSqr)*restDist);
            } else {
                spring.x() += ((rand() & 255) - 128)*jitterMag;
                spring.y() += ((rand() & 255) - 128)*jitterMag;
            }
        });

        const Vector2 newPos = ppos + dt*spring;
        _particles.tmp[p] = _grid.constrainBoundary(newPos);
    });

    _particles.positions.swap(_particles.tmp);
}

void ApicSolver2D::gridVelocity2Particle() {
    Array2X<Float>& u = _grid.u;
    Array2X<Float>& v = _grid.v;
    const auto dxInv = _grid.invCellSize;

    _particles.loopAll([&](UnsignedInt p) {
        const Vector2 gridPos = _grid.getGridPos(_particles.positions[p]);
        const Vector2 px = gridPos - Vector2(0, 0.5);
        const Vector2 py = gridPos - Vector2(0.5, 0);

        _particles.velocities[p] = Vector2(u.interpolateValue(px),
                                           v.interpolateValue(py));
        _particles.affineMat[p] = Matrix2x2(u.affineInterpolateValue(px)*dxInv,
                                            v.affineInterpolateValue(py)*dxInv);
    });
}

}}
