#ifndef Magnum_Examples_FluidSimulation2D_SparseMatrix_h
#define Magnum_Examples_FluidSimulation2D_SparseMatrix_h
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

#include <algorithm>
#include <vector>

namespace Magnum { namespace Examples {
template<class T>
struct SparseMatrix {
    explicit SparseMatrix(uint32_t size_ = 0) { resize(size_); }

    void resize(std::size_t size_) {
        size = size_;
        rowIndices.resize(size);
        rowValues.resize(size);
        rowStartIdx.resize(size + 1);
    }

    void clear() {
        for(auto& indices : rowIndices) {
            indices.resize(0);
        }
        for(auto& vals : rowValues) {
            vals.resize(0);
        }
    }

    template<class IntType, class U>
    void addToElement(IntType i_, IntType j_, U inc_val_) {
        const auto i   = static_cast<uint32_t>(i_);
        const auto j   = static_cast<uint32_t>(j_);
        const auto val = static_cast<T>(inc_val_);
        auto&      currentRowIndices = rowIndices[i];

        auto iter = std::lower_bound(currentRowIndices.begin(), currentRowIndices.end(), j);
        if((iter != currentRowIndices.end()) && (*iter == j)) {
            const auto k = std::distance(currentRowIndices.begin(), iter);
            rowValues[i][k] += val;
        } else {
            auto insert_sorted_vector =
                [](std::vector<uint32_t>& vec, uint32_t item) {
                    return vec.insert(std::upper_bound(vec.begin(), vec.end(), item), item);
                };
            iter = insert_sorted_vector(currentRowIndices, j);
            const auto k = std::distance(currentRowIndices.begin(), iter);
            rowValues[i].insert(rowValues[i].begin() + k, val);
        }
    }

    void compressData() {
        rowStartIdx[0] = 0;
        for(uint32_t i = 0; i < size; ++i) {
            rowStartIdx[i + 1] = rowStartIdx[i] + static_cast<uint32_t>(rowIndices[i].size());
        }

        auto copyData = [](auto& dst, const auto& src) {
                            dst.resize(0);
                            for(const auto& vec : src) {
                                dst.insert(dst.end(), vec.begin(), vec.end());
                            }
                        };

        copyData(compactIndices, rowIndices);
        copyData(compactValues,  rowValues);
    }

    void multiply(const std::vector<T>& x, std::vector<T>& result) const {
        result.resize(size);
        for(uint32_t i = 0; i < size; ++i) {
            T tmp = 0;
            for(uint32_t j = rowStartIdx[i], jend = rowStartIdx[i + 1]; j < jend; ++j) {
                tmp += compactValues[j] * x[compactIndices[j]];
            }
            result[i] = tmp;
        }
    }

    uint32_t size;

    /* Sparse matrix data */
    std::vector<std::vector<uint32_t>> rowIndices;
    std::vector<std::vector<T>>        rowValues;

    /* Compact data */
    std::vector<T>        compactValues;
    std::vector<uint32_t> compactIndices;
    std::vector<uint32_t> rowStartIdx;
};
} }

#endif
