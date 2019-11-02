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

#pragma once

#include <Magnum/Magnum.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Vector3.h>

#include <cassert>
#include <vector>

namespace Magnum { namespace Examples {
/****************************************************************************************************/
/*
 * A grid data structure to search for indices of particle neighbors within a given distance
 * Upon searching for neighbors, the relative positions with neighbors are also computed
 */
class DomainBox {
public:
    explicit DomainBox(float particleRadius, const Vector3& lowerDomainBound, const Vector3& upperDomainBound);

    Vector3& lowerDomainBound() { return _lowerDomainBound; }
    Vector3& upperDomainBound() { return _upperDomainBound; }

    void findNeighbors(const std::vector<Vector3>&         positions,
                       std::vector<std::vector<uint32_t>>& neighbors,
                       std::vector<std::vector<Vector3>>&  relativePositions);

    bool enforceBoundary(Vector3& ppos, Vector3& pvel, float restitution);

private:
    void generateBoundaryParticles();
    void collectIndices(const std::vector<Vector3>& positions);
    void tightenGrid(const std::vector<Vector3>& positions);

    template<int d>
    inline bool isValidIndex(int idx) {
        return idx >= 0 && static_cast<uint32_t>(idx) < _gridSize[d];
    }

    inline Vector3i getCellIndex(const Vector3& ppos) {
        Vector3i cellIdx{ Math::NoInit };
        for(size_t i = 0; i < 3; ++i) {
            cellIdx[i] = static_cast<int>((ppos[i] - _lowerGridBound[i]) * _invCellLength);
        }
        assert(isValidIndex<0>(cellIdx[0] &&)
               isValidIndex<1>(cellIdx[1] &&)
               isValidIndex<2>(cellIdx[2]));
        return cellIdx;
    }

    inline uint32_t getFlatIndex(int i, int j, int k) {
        const auto flatIndex = (static_cast<uint32_t>(k) * _gridSize[1] +
                                static_cast<uint32_t>(j)) * _gridSize[0] + static_cast<uint32_t>(i);
        assert(flatIndex < _cells.size());
        return flatIndex;
    }

    std::vector<std::vector<uint32_t>> _cells;
    std::vector<Vector3>               _boundaryParticles;

    Vector3  _lowerDomainBound, _upperDomainBound;
    Vector3  _lowerGridBound;
    Vector3  _upperGridBound;
    uint32_t _gridSize[3] { 1u, 1u, 1u };
    float    _cellLength{ 1.0f };
    float    _maxDistSqr { 1.0f };
    float    _invCellLength{ 1.0f };
    float    _particleRadius { 1.0f };
    float    _overlappedDistSqr { 1.0e-4f };
};

/****************************************************************************************************/
} } /* namespace Magnum::Examples  */
