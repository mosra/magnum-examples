/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 —
            Vladimír Vondruš <mosra@centrum.cz>
        2020 — Nghia Truong <nghiatruong.vn@gmail.com>

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

#include <Corrade/Containers/GrowableArray.h>

#include "Simulation/Simulator.h"

namespace Magnum { namespace Examples {
Simulator::~Simulator() {
    /* Must do this to avoid memory leak*/
    for(Constraint* c : _constraints) {
        delete c;
    }
}

void Simulator::reset() {
    /* Reset timing */
    _generalParams.time = 0;

    /* Reset wind */
    _windParams.enable = false;

    /* Reset mesh data */
    _mesh->reset();
}

void Simulator::advanceStep() {
    if(_constraints.size() == 0) {
        initConstraints();
        CORRADE_INTERNAL_ASSERT(_constraints.size() > 0);
    }
    /* Enable wind if the system time is larger than timeEnable */
    if(_windParams.timeEnable >= 0
       && _generalParams.time >= _windParams.timeEnable
       && !_windParams.enable) {
        _windParams.enable = true;
    }

    calculateExternalForce(_windParams.enable);

    /* Change global time step, as it will be used somewhere else */
    const Float old_dt = _generalParams.dt;
    _generalParams.dt = _generalParams.dt / _generalParams.subSteps;

    for(int step = 0; step < _generalParams.subSteps; ++step) {
        _integration.y = _mesh->_positions +
                         _mesh->_velocities * _generalParams.dt +
                         _mesh->_invMassMatrix * _integration.externalForces * _generalParams.dt * _generalParams.dt;

        EgVecXf x = _integration.y; /* x will be modified during line search */
        _lineSearch.firstIteration = true;
        bool converge = false;
        for(UnsignedInt iter = 0; !converge && iter < _lineSearch.iterations; ++iter) {
            converge = performLBFGSOneIteration(x);
            _lineSearch.firstIteration = false;
        }

        _mesh->_velocities = (x - _mesh->_positions) / _generalParams.dt;
        _mesh->_positions  = x;
        if(std::abs(_generalParams.damping) > 0) {
            _mesh->_velocities *= 1 - _generalParams.damping;
        }
    }
    _generalParams.dt    = old_dt;
    _generalParams.time += _generalParams.dt;
}

void Simulator::updateConstraintParameters() {
    for(const auto c: _constraints) {
        if(c->type() == Constraint::Type::FEM) {
            static_cast<FEMConstraint*>(c)->setFEMMaterial(
                static_cast<FEMConstraint::Material>(_materialParams.type),
                _materialParams.mu, _materialParams.lambda, _materialParams.kappa);
        } else if(c->type() == Constraint::Type::Attachment) {
            static_cast<AttachmentConstraint*>(c)->setStiffness(_generalParams.attachmentStiffness);
        }
    }

    /* Need to prerefactor the Laplacean, as material has changed */
    _lbfgs.prefactored = false;
}

void Simulator::initConstraints() {
    /* Setup attachment constraints */
    for(const auto vIdx : _mesh->_fixedVerts) {
        const EgVec3f fixedPoint = _mesh->_positions.block3(vIdx);
        arrayAppend(_constraints, Containers::InPlaceInit,
                    new AttachmentConstraint(vIdx, fixedPoint, _generalParams.attachmentStiffness));
    }

    /* Compute mass for the vertices */
    Float totalVolume = 0;
    _mesh->_massMatrix.resize(3 * _mesh->_numVerts);
    _mesh->_massMatrix1D.resize(_mesh->_numVerts);
    _mesh->_invMassMatrix.resize(3 * _mesh->_numVerts);
    _mesh->_massMatrix.setZero();
    _mesh->_massMatrix1D.setZero();
    _mesh->_invMassMatrix.setZero();

    for(const auto& tet:  _mesh->_tets) {
        FEMConstraint* c = new FEMConstraint(tet, _mesh->_positions_t0);
        c->setFEMMaterial(static_cast<FEMConstraint::Material>(_materialParams.type),
                          _materialParams.mu, _materialParams.lambda, _materialParams.kappa);
        totalVolume += c->getMassContribution(_mesh->_massMatrix.diagonal(),
                                              _mesh->_massMatrix1D.diagonal());
        arrayAppend(_constraints, static_cast<Constraint*>(c));
    }
    _mesh->_massMatrix   = _mesh->_massMatrix * (_mesh->_totalMass / totalVolume);
    _mesh->_massMatrix1D = _mesh->_massMatrix1D * (_mesh->_totalMass / totalVolume);

    /* Compute inverse mass for the vertices */
    _mesh->_invMassMatrix.resize(3 * _mesh->_numVerts);
    for(UnsignedInt i = 0; i < 3 * _mesh->_numVerts; ++i) {
        _mesh->_invMassMatrix.diagonal()[i] = 1.0f / _mesh->_massMatrix.diagonal()[i];
    }
}

void Simulator::calculateExternalForce(bool addWind) {
    EgVec3f force = _generalParams.gravity;
    if(addWind) {
        force.z() = (std::sin((_generalParams.time - _windParams.timeEnable) * _windParams.frequency) + 0.5f) * _windParams.magnitude;
    }
    _integration.externalForces.resize(_mesh->_numVerts * 3);
    for(UnsignedInt i = 0; i < _mesh->_numVerts; ++i) {
        _integration.externalForces.block3(i) = force;
    }
    _integration.externalForces = _mesh->_massMatrix * _integration.externalForces;
}

bool Simulator::performLBFGSOneIteration(EgVecXf& x) {
    const Float       historyLength = _lbfgs.historyLength;
    const Float       epsLARGE      = _lbfgs.epsLARGE;
    const std::size_t epsSMALL      = _lbfgs.epsSMALL;

    bool    converged = false;
    Float   currentEnergy;
    EgVecXf currentGrad;
    auto&   yQueue             = _lbfgs.yQueue;
    auto&   sQueue             = _lbfgs.sQueue;
    auto&   prefetchedEnergy   = _lineSearch.prefetchedEnergy;
    auto&   prefetchedGradient = _lineSearch.prefetchedGradient;

    if(_lineSearch.firstIteration) {
        yQueue.clear();
        sQueue.clear();
        prefactorize();

        currentEnergy       = evaluateEnergyAndGradient(x, currentGrad);
        _lbfgs.lastX        = x;
        _lbfgs.lastGradient = currentGrad;

        const EgVecXf p_k = -lbfgsKernelLinearSolve(currentGrad);
        if(-p_k.dot(currentGrad) < epsSMALL || p_k.norm() / x.norm() < epsLARGE) {
            converged = true;
        }
        const Float alpha_k = linesearch(x, currentEnergy, currentGrad, p_k,
                                         prefetchedEnergy, prefetchedGradient);
        x += alpha_k * p_k;
    } else {
        currentEnergy = prefetchedEnergy;
        currentGrad   = prefetchedGradient;

        const EgVecXf s_k = x - _lbfgs.lastX;
        const EgVecXf y_k = currentGrad - _lbfgs.lastGradient;
        if(sQueue.size() > _lbfgs.historyLength) {
            sQueue.pop_back();
            yQueue.pop_back();
        }
        sQueue.push_front(std::move(s_k));
        yQueue.push_front(std::move(y_k));

        _lbfgs.lastX        = x;
        _lbfgs.lastGradient = currentGrad;
        EgVecXf q = currentGrad;

        Containers::Array<Float> rho;
        Containers::Array<Float> alpha;
        const std::size_t        queueSize  = sQueue.size();
        const std::size_t        visitBound = (historyLength < queueSize) ? historyLength : queueSize;
        for(std::size_t i = 0; i < visitBound; ++i) {
            const Float yi_dot_si = yQueue[i].dot(sQueue[i]);
            if(yi_dot_si < _lbfgs.epsSMALL) {
                return true;
            }
            const Float rho_i = 1.0f / yi_dot_si;
            arrayAppend(rho,   rho_i);
            arrayAppend(alpha, rho[i] * sQueue[i].dot(q));
            q = q - alpha[i] * yQueue[i];
        }

        EgVecXf p_k = -lbfgsKernelLinearSolve(q);
        for(int i = int(visitBound) - 1; i >= 0; --i) {
            const Float beta = rho[i] * yQueue[i].dot(p_k);
            p_k -= sQueue[i] * (alpha[i] - beta);
        }
        if(-p_k.dot(currentGrad) < epsSMALL || p_k.squaredNorm() < epsSMALL) {
            converged = true;
        }
        const Float alpha_k = linesearch(x, currentEnergy, currentGrad, p_k,
                                         prefetchedEnergy, prefetchedGradient);
        x += alpha_k * p_k;
    }
    return converged;
}

Float Simulator::evaluateEnergyAndGradient(const EgVecXf& x, EgVecXf& gradient) {
    const Float   hSqr          = _generalParams.dt * _generalParams.dt;
    const EgVecXf xmy           = x - _integration.y;
    const EgVecXf Mxmy          = _mesh->_massMatrix * xmy;
    const Float   energyInertia = 0.5f * xmy.transpose() * Mxmy;
    gradient.resize(_mesh->_numVerts * 3);
    gradient.setZero();
    Float energyConstraints = 0;
    for(const auto c: _constraints) {
        energyConstraints += c->evaluateEnergyAndGradient(x, gradient);
    }
    gradient = Mxmy + hSqr * gradient;
    const Float energy = energyInertia + hSqr * energyConstraints;
    return energy;
}

EgVecXf Simulator::lbfgsKernelLinearSolve(const EgVecXf& rhs) {
    auto& rhsN3 = _lbfgs.linearSolveRhsN3;
    rhsN3.resize(rhs.size() / 3, 3);
    std::memcpy(rhsN3.data(), rhs.data(), sizeof(Float) * rhs.size());
    const EgMatX3f r_n3 = _lbfgs.lltSolver.solve(rhsN3);

    EgVecXf r;
    r.resize(rhs.size());
    std::memcpy(r.data(), r_n3.data(), sizeof(Float) * r.size());
    return r;
}

Float Simulator::linesearch(const EgVecXf& x, Float energy, const EgVecXf& gradDir, const EgVecXf& descentDir,
                            Float& nextEnergy, EgVecXf& nextGradDir) {
    EgVecXf     x_plus_tdx(_mesh->_numVerts * 3);
    Float       t      = 1.0f / _lineSearch.beta;
    const Float objVal = energy;
    do{
        t         *= _lineSearch.beta;
        x_plus_tdx = x + t * descentDir;
        const Float lhs = evaluateEnergyAndGradient(x_plus_tdx, nextGradDir);
        const Float rhs = objVal + _lineSearch.alpha * t * gradDir.transpose().dot(descentDir);
        if(lhs < rhs) {
            nextEnergy = lhs;
            break;
        }
    } while (t > 1e-5f);

    if(t < 1e-5f) {
        t           = 0;
        nextEnergy  = energy;
        nextGradDir = gradDir;
    }
    return t;
}

void Simulator::prefactorize() {
    if(!_lbfgs.prefactored) {
        /* Compute the Laplacian matrix */
        Containers::Array<EgTripletf> triplets;
        for(const auto c: _constraints) {
            c->getWLaplacianContribution(triplets);
        }
        EgSparseMatf weightedLaplacian1D;
        weightedLaplacian1D.resize(_mesh->_numVerts, _mesh->_numVerts);
        weightedLaplacian1D.setFromTriplets(triplets.begin(), triplets.end());
        weightedLaplacian1D *= (_generalParams.dt * _generalParams.dt);
        for(UnsignedInt i = 0; i < weightedLaplacian1D.rows(); ++i) {
            weightedLaplacian1D.coeffRef(i, i) += _mesh->_massMatrix1D.diagonal()[i];
        }

        /* Prefactor the matrix */
        EgSparseMatf& A = weightedLaplacian1D;
        _lbfgs.lltSolver.analyzePattern(A);
        _lbfgs.lltSolver.factorize(A);
        Float regularization = 1e-10f;
        while(_lbfgs.lltSolver.info() != Eigen::Success) {
            regularization *= 10;
            for(int i = 0; i < A.rows(); ++i) {
                A.coeffRef(i, i) += regularization;
            }
            _lbfgs.lltSolver.factorize(A);
        }
        _lbfgs.prefactored = true;
    }
}
} }
