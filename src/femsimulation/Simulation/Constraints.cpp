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
#include <Eigen/SVD>

#include "Simulation/Constraints.h"

namespace Magnum { namespace Examples {
Float AttachmentConstraint::evaluateEnergyAndGradient(const EgVecXf& x, EgVecXf& gradient) const {
    const EgVec3f d = x.block3(_vIdx) - _fixedPos;
    const EgVec3f g = _stiffness * d;
    gradient.block3(_vIdx) += g;
    const Float energy = 0.5f * _stiffness * d.squaredNorm();
    return energy;
}

void AttachmentConstraint::getWLaplacianContribution(Containers::Array<EgTripletf>& triplets) const {
    arrayAppend(triplets, Containers::InPlaceInit, EgTripletf(_vIdx, _vIdx, _stiffness));
}

FEMConstraint::FEMConstraint(const Vector4ui& vIDs, const EgVecXf& x) :
    Constraint(Constraint::Type::FEM), _vIDs(vIDs) {
    for(size_t i = 0; i < 3; ++i) {
        _Dm.col(i) = x.block3(_vIDs[i]) - x.block3(_vIDs[3]);
    }

    _Dm_inv   = _Dm.inverse();
    _Dm_inv_T = _Dm_inv.transpose();
    _w        = std::abs(_Dm.determinant()) / 6.0f;

    EgMat3x4f IND;
    IND.block<3, 3>(0, 0) = EgMat3f::Identity();
    IND.block<3, 1>(0, 3) = EgVec3f(-1, -1, -1);
    _G = _Dm_inv_T * IND;
}

void FEMConstraint::setFEMMaterial(Material type, Float mu, Float lambda, Float kappa) {
    _material = type;
    _mu       = mu;
    _lambda   = lambda;
    _kappa    = kappa;

    /* Compute the Laplacian weight which depends on the material parameters */
    computeLaplacianWeight();
}

void FEMConstraint::computeLaplacianWeight() {
    Float laplacianCoeff{ 0 };
    switch(_material) {
        case Material::Corotational:
            // 2mu (x-1) + lambda (x-1)
            laplacianCoeff = 2 * _mu + _lambda;
            break;
        case Material::StVK:
            // mu * (x^2  - 1) + 0.5lambda * (x^3 - x)
            // 10% window
            laplacianCoeff = 2 * _mu + 1.0033 * _lambda;
            break;
        case Material::NeoHookeanExtendLog:
            // mu * x - mu / x + lambda * log(x) / x
            // 10% window
            laplacianCoeff = 2.0066 * _mu + 1.0122 * _lambda;
            break;
        default:
            break;
    }
    _L = laplacianCoeff * _w * _G.transpose() * _G;
}

Float FEMConstraint::getMassContribution(EgVecXf& m, EgVecXf& m_1d) {
    const Float vw = 0.25f * _w;
    for(UnsignedInt i = 0; i != 4; i++) {
        m[3 * _vIDs[i] + 0] += vw;
        m[3 * _vIDs[i] + 1] += vw;
        m[3 * _vIDs[i] + 2] += vw;
        m_1d[_vIDs[i]]      += vw;
    }
    return _w;
}

Float FEMConstraint::computeStressAndEnergyDensity(const EgMat3f& F, EgMat3f& P) const {
    Float e_density = 0;
    switch(_material) {
        case Material::Corotational: {
            EgMat3f U;
            EgMat3f V;
            EgVec3f SIGMA;
            singularValueDecomp(U, SIGMA, V, F);
            const EgMat3f R          = U * V.transpose();
            const EgMat3f FmR        = F - R;
            const Float   traceRTFm3 = (R.transpose() * F).trace() - 3;
            P         = 2 * _mu * FmR + _lambda * traceRTFm3 * R;
            e_density = _mu * FmR.squaredNorm() + 0.5f * _lambda * traceRTFm3 * traceRTFm3;
        }
        break;
        case Material::StVK: {
            const EgMat3f I      = EgMat3f::Identity();
            const EgMat3f E      = 0.5f * (F.transpose() * F - I);
            const Float   traceE = E.trace();
            P = F * (2 * _mu * E + _lambda * traceE * I);
            const Float J = F.determinant();
            e_density = _mu * E.squaredNorm() + 0.5f * _lambda * traceE * traceE;
            if(J < 1 && J != 0) {
                const Float one_mJd6 = (1 - J) / 6;
                P         += -_kappa / 24 * one_mJd6 * one_mJd6 * J * F.inverse().transpose();
                e_density += _kappa / 12 * one_mJd6 * one_mJd6 * one_mJd6;
            }
        }
        break;
        case Material::NeoHookeanExtendLog: {
            P         = _mu * F;
            e_density = 0.5f * _mu * ((F.transpose() * F).trace() - 3);

            const Float J = F.determinant();
            if(J == 0) { /* degenerated tet */
                break;
            }
            const Float   J0    = _neohookean_clamp_value;
            const EgMat3f FinvT = F.inverse().transpose(); /* F is invertible if J != 0 */
            if(J > J0) {
                const Float logJ = std::log(J);
                P         += (-_mu + _lambda * logJ) * FinvT;
                e_density += (-_mu + 0.5f * _lambda * logJ) * logJ;
            } else {
                const Float JmJ0dJ0 = (J - J0) / J0;
#ifdef LOGJ_QUADRATIC_EXTENSION
                const Float fJ    = std::log(J0) + JmJ0dJ0 - 0.5f * JmJ0dJ0 * JmJ0dJ0;
                const Float dfJdJ = (1.0f - JmJ0dJ0) / J0;
#else
                Float fJ    = std::log(J0) + JmJ0dJ0;
                Float dfJdJ = 1.0f / J0;
#endif
                P         += (-_mu + _lambda * fJ) * dfJdJ * J * FinvT;
                e_density += (-_mu + 0.5f * _lambda * fJ) * fJ;
            }
        }
        break;
        default:
            break;
    }
    return e_density;
}

Float FEMConstraint::evaluateEnergyAndGradient(const EgVecXf& x, EgVecXf& gradient) const {
    EgMat3f       P;
    const EgMat3f F         = getDeformGrad(x);
    const Float   e_density = computeStressAndEnergyDensity(F, P);
    const EgMat3f H         = _w * P * _Dm_inv_T;
    EgVec3f       H3        = EgVec3f::Zero();
    for(UnsignedInt i = 0; i < 3; i++) {
        gradient.block3(_vIDs[i]) += H.col(i);
        H3 += H.col(i);
    }
    gradient.block3(_vIDs[3]) -= H3;
    return e_density * _w;
}

void FEMConstraint::getWLaplacianContribution(Containers::Array<EgTripletf>& triplets) const {
    for(UnsignedInt i = 0; i < 4; i++) {
        for(UnsignedInt j = 0; j < 4; j++) {
            arrayAppend(triplets, Containers::InPlaceInit, EgTripletf(_vIDs[i], _vIDs[j], _L(i, j)));
        }
    }
}

EgMat3f FEMConstraint::getDeformGrad(const EgVecXf& x) const {
    EgMat3f Ds;
    for(size_t i = 0; i < 3; ++i) {
        Ds.col(i) = x.block3(_vIDs[i]) - x.block3(_vIDs[3]);
    }
    return Ds * _Dm_inv;
}

void FEMConstraint::singularValueDecomp(EgMat3f& U, EgVec3f& SIGMA, EgMat3f& V, const EgMat3f& A, bool signed_svd) const {
    Eigen::JacobiSVD<EgMat3f> svd;
    svd.compute(A, Eigen::ComputeFullU | Eigen::ComputeFullV);
    U     = svd.matrixU();
    V     = svd.matrixV();
    SIGMA = svd.singularValues();
    if(signed_svd) {
        Float detU = U.determinant();
        Float detV = V.determinant();
        if(detU < 0) {
            U.block<3, 1>(0, 2) *= -1;
            SIGMA[2] *= -1;
        }
        if(detV < 0) {
            V.block<3, 1>(0, 2) *= -1;
            SIGMA[2] *= -1;
        }
    }
}
} }
