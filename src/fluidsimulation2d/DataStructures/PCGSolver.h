#ifndef Magnum_Examples_FluidSimulation2D_PCGSolver_h
#define Magnum_Examples_FluidSimulation2D_PCGSolver_h
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

#include <Corrade/Utility/StlMath.h>

#include "SparseMatrix.h"

namespace Magnum { namespace Examples {
template<class T>
class PCGSolver {
public:
    PCGSolver(T toleranceFactor_ = T(1e-10), uint32_t maxIterations_ = 1000) :
        _toleranceFactor{toleranceFactor_}, _maxIterations{maxIterations_} {}

    bool solve(SparseMatrix<T>& matrix, const std::vector<T>& rhs, std::vector<T>& result) {
        uint32_t rows = matrix.size;
        if(rows == 0) {
            return false;
        }
        if(_m.size() != rows) {
            _m.resize(rows);
            _s.resize(rows);
            _z.resize(rows);
            _r.resize(rows);
        }

        _r = rhs;
        _lastResidual = maxAbs(_r);
        if(!(_lastResidual > 0)) {
            _lastIterationCount = 0;
            return true;
        }

        formPreconditioner(matrix);
        applyPreconditioner(_r, _z);
        T rho = dotProduct(_z, _r);
        if(!(rho > 0) || rho != rho) {
            _lastIterationCount = 0;
            return false;
        }

        _s = _z;
        matrix.compressData(); /* must prepare compact data for Matrix * Vector operation */

        const T  tolerance = _toleranceFactor * _lastResidual;
        uint32_t iter { 0 };
        for(; iter < _maxIterations; ++iter) {
            matrix.multiply(_s, _z);
            const T alpha = rho / dotProduct(_s, _z);
            addScaled(alpha,  _s, result);
            addScaled(-alpha, _z, _r);
            _lastResidual = maxAbs(_r);
            if(_lastResidual < tolerance) {
                _lastIterationCount = iter + 1;
                return true;
            }
            applyPreconditioner(_r, _z);
            const T rho_new = dotProduct(_z, _r);
            const T beta    = rho_new / rho;
            addScaled(beta, _s, _z);
            _s.swap(_z);
            rho = rho_new;
        }

        /* Failed to converge */
        _lastIterationCount = _maxIterations;
        return false;
    }

    /* API to query last solve */
    uint32_t lastIterationCount() const { return _lastIterationCount; }
    T lastResidual() const { return _lastResidual; }

private:
    void formPreconditioner(const SparseMatrix<T>& matrix) {
        static constexpr T s_modification_parameter = T(0.97);

        const auto size = matrix.size;
        _precond.reset(size);

        /* Copy data from the matrix */
        for(uint32_t i = 0; i < size; ++i) {
            _precond.colStartIdx[i] = static_cast<uint32_t>(_precond.colIndices.size());
            const auto& indices = matrix.rowIndices[i];
            const auto& values  = matrix.rowValues[i];
            for(uint32_t j = 0, jend = indices.size(); j < jend; ++j) {
                if(indices[j] > i) {
                    _precond.colIndices.push_back(indices[j]);
                    _precond.colValues.push_back(values[j]);
                } else if(indices[j] == i) {
                    _precond.invdiag[i] = values[j];
                }
            }
        }
        _precond.colStartIdx[size] = static_cast<uint32_t>(_precond.colIndices.size());

        for(uint32_t k = 0; k < size; ++k) {
            auto invdiag = _precond.invdiag[k];
            if(invdiag == T(0)) {
                continue; /* null row/column */
            }
            invdiag = T(1) / std::sqrt(invdiag);
            _precond.invdiag[k] = invdiag;

            const auto pStart = _precond.colStartIdx[k];
            const auto pEnd   = _precond.colStartIdx[k + 1];
            for(uint32_t p = pStart; p < pEnd; ++p) {
                _precond.colValues[p] *= invdiag;
            }

            /* Process the lower elements of column k */
            for(uint32_t p = pStart; p < pEnd; ++p) {
                const auto j          = _precond.colIndices[p];
                const auto multiplier = _precond.colValues[p];
                T          missing    = 0;
                uint32_t   a          = pStart;

                uint32_t b = 0;
                while(a < pEnd && _precond.colIndices[a] < j) {
                    while(b < matrix.rowIndices[j].size()) {
                        if(matrix.rowIndices[j][b] < _precond.colIndices[a]) {
                            ++b;
                        } else if(matrix.rowIndices[j][b] == _precond.colIndices[a]) {
                            break;
                        } else {
                            missing += _precond.colValues[a];
                            break;
                        }
                    }
                    ++a;
                }

                invdiag = _precond.invdiag[j];
                if(a < pEnd && _precond.colIndices[a] == j) {
                    invdiag -= multiplier * _precond.colValues[a];
                }
                ++a;

                b = _precond.colStartIdx[j];
                const auto jEnd = _precond.colStartIdx[j + 1];
                while(a < pEnd && b < jEnd) {
                    if(_precond.colIndices[b] < _precond.colIndices[a]) {
                        ++b;
                    } else if(_precond.colIndices[b] == _precond.colIndices[a]) {
                        _precond.colValues[b] -= multiplier * _precond.colValues[a];
                        ++a;
                        ++b;
                    } else {
                        missing += _precond.colValues[a++];
                    }
                }

                while(a < pEnd) {
                    missing += _precond.colValues[a++];
                }

                _precond.invdiag[j] = invdiag - s_modification_parameter * multiplier * missing;
            }
        }
    }

    void applyPreconditioner(const std::vector<T>& rhs, std::vector<T>& result) const {
        const uint32_t rows = _precond.rows;

        /* Solve L * result = rhs */
        result = rhs;
        for(uint32_t i = 0; i < rows; ++i) {
            const auto colStart = _precond.colStartIdx[i];
            const auto colEnd   = _precond.colStartIdx[i + 1];
            const auto tmp      = result[i] * _precond.invdiag[i];
            result[i] = tmp;
            for(uint32_t j = colStart; j < colEnd; ++j) {
                result[_precond.colIndices[j]] -= _precond.colValues[j] * tmp;
            }
        }

        /* solve L^T * result = result */
        uint32_t i = rows;
        do {
            --i;
            const auto colStart = _precond.colStartIdx[i];
            const auto colEnd   = _precond.colStartIdx[i + 1];
            auto       tmp      = result[i];
            for(uint32_t j = colStart; j < colEnd; ++j) {
                tmp -= _precond.colValues[j] * result[_precond.colIndices[j]];
            }
            tmp      *= _precond.invdiag[i];
            result[i] = tmp;
        } while (i != 0);
    }

    T dotProduct(const std::vector<T>& x, const std::vector<T>& y) const {
        T sum = 0;
        for(std::size_t i = 0; i < x.size(); ++i) {
            sum += x[i] * y[i];
        }
        return sum;
    }

    T maxAbs(const std::vector<T>& x) const {
        T maxVal = 0;
        for(std::size_t i = 0; i < x.size(); ++i) {
            const auto absVal = std::abs(x[i]);
            if(absVal > maxVal) {
                maxVal = absVal;
            }
        }
        return maxVal;
    }

    void addScaled(T alpha, const std::vector<T>& x, std::vector<T>& y) const {
        for(std::size_t i = 0; i < x.size(); ++i) {
            y[i] += alpha * x[i];
        }
    }

    /* Solver parameters */
    const uint32_t _maxIterations;
    const T        _toleranceFactor;

    /* Preconditioner - triangular matrix */
    struct {
        uint32_t              rows;
        std::vector<T>        invdiag;     /* inverse of diagonal elements                                                        */
        std::vector<T>        colValues;   /* values below the diagonal, listed column by column                                  */
        std::vector<uint32_t> colIndices;  /* a list of all row indices, for each column in turn                                  */
        std::vector<uint32_t> colStartIdx; /* where each column begins in rowindex (plus an extra entry at the end, of #nonzeros) */

        void reset(uint32_t rows_) {
            rows = rows_;
            invdiag.assign(rows, T(0)); /* important: must set zero */
            colValues.resize(0);
            colIndices.resize(0);
            colStartIdx.resize(rows + 1);
        }
    } _precond;

    /* Solver temporary variables */
    std::vector<T> _m, _z, _s, _r;

    /* Status of last solve */
    uint32_t _lastIterationCount { 0 };
    T        _lastResidual { 0 };
};
} }

#endif
