#ifndef Magnum_Examples_FluidSimulation2D_Array2X_h
#define Magnum_Examples_FluidSimulation2D_Array2X_h
/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 —
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

#include <cstring>
#include <vector>
#include <Corrade/Utility/Debug.h>
#include <Magnum/Math/Vector2.h>

#include "MathHelpers.h"

namespace Magnum { namespace Examples {

template<class T> class Array2X {
    public:
        /*implicit*/ Array2X() = default;
        /*implicit*/ Array2X(std::size_t nx, std::size_t ny): _size{nx, ny}, _data(nx*ny) {}
        /*implicit*/ Array2X(std::size_t nx, std::size_t ny, const T& value) : _size{nx, ny}, _data(nx*ny, value) {}

        Array2X<T>& operator=(const Array2X<T>& other) {
            if(other.sizeX() != sizeX() || other.sizeY() != sizeY()) {
                Fatal{} << "Copy array with different size!";
            }
            std::memcpy(data(), other.data(), count() * sizeof(T));
            return *this;
        }

        /* Accessors */

        template<class IntType> const T& operator()(IntType i, IntType j) const {
            CORRADE_INTERNAL_ASSERT(i >= 0 && j >= 0 &&
                std::size_t(i) < _size[0] && std::size_t(j) < _size[1]);
            return _data[i + _size[0] * j];
        }

        template<class IntType> T& operator()(IntType i, IntType j) {
            CORRADE_INTERNAL_ASSERT(i >= 0 && j >= 0 &&
                std::size_t(i) < _size[0] && std::size_t(j) < _size[1]);
            return _data[i + _size[0] * j];
        }

        template<class IntType> const T& operator()(const Math::Vector2<IntType>& coord) const {
            return operator()(coord[0], coord[1]);
        }

        template<class IntType> T& operator()(const Math::Vector2<IntType>& coord) {
            return operator()(coord[0], coord[1]);
        }

        const T* data() const { return _data.data(); }
        T* data() { return _data.data(); }

        std::size_t sizeX() const { return _size[0]; }
        std::size_t sizeY() const { return _size[1]; }
        std::size_t count() const { return _data.size(); }

        /* Modifiers */

        void assign(const T& value) { _data.assign(_data.size(), value); }
        void setZero() { _data.assign(_data.size(), T(0)); }

        template<class IntType> void resize(IntType nx, IntType ny) {
            _size[0] = std::size_t(nx);
            _size[1] = std::size_t(ny);
            _data.resize(_size[0] * _size[1]);
        }

        template<class IntType> void resize(IntType nx, IntType ny, const T& value) {
            _size[0] = std::size_t(nx);
            _size[1] = std::size_t(ny);
            _data.resize(_size[0] * _size[1], value);
        }

        void swapContent(Array2X<T>& other) {
            /* Only allow to swap content of array having the same sizes */
            if(other.sizeX() != sizeX() || other.sizeY() != sizeY()) {
                Fatal{} << "Swap content of arrays having different sizes!";
            }
            _data.swap(other._data);
        }

        /* Data manipulation */

        template<class Function> void loop1D(Function&& func) const {
            for(std::size_t i = 0, iend = count(); i < iend; ++i) {
                func(i);
            }
        }

        template<class Function> void loop2D(Function&& func) const {
            for(std::size_t j = 0; j < sizeY(); ++j) {
                for(std::size_t i = 0; i < sizeX(); ++i) {
                    func(i, j);
                }
            }
        }

        T interpolateValue(const Math::Vector2<T>& point) const {
            Int i, j;
            T   fx, fy;
            barycentric(point[0], i, fx, 0, Int(sizeX()));
            barycentric(point[1], j, fy, 0, Int(sizeY()));
            T v00 = (*this)(i, j);
            T v10 = (*this)(i + 1, j);
            T v01 = (*this)(i, j + 1);
            T v11 = (*this)(i + 1, j + 1);
            return bilerp(v00, v10, v01, v11, fx, fy);
        }

        Math::Vector2<T> affineInterpolateValue(const Math::Vector2<T>& point) const {
            Int i, j;
            T fx, fy;
            barycentric(point[0], i, fx, 0, Int(sizeX()));
            barycentric(point[1], j, fy, 0, Int(sizeY()));
            T v00 = (*this)(i, j);
            T v10 = (*this)(i + 1, j);
            T v01 = (*this)(i, j + 1);
            T v11 = (*this)(i + 1, j + 1);
            return bilerpGradient(v00, v10, v01, v11, fx, fy);
        }

        Math::Vector2<T> interpolateGradient(const Math::Vector2<T>& point) const {
            Int i, j;
            T fx, fy;
            barycentric(point[0], i, fx, 0, Int(sizeX()));
            barycentric(point[1], j, fy, 0, Int(sizeY()));
            T v00  = (*this)(i, j);
            T v01  = (*this)(i, j + 1);
            T v10  = (*this)(i + 1, j);
            T v11  = (*this)(i + 1, j + 1);
            T ddy0 = (v01 - v00);
            T ddy1 = (v11 - v10);
            T ddx0 = (v10 - v00);
            T ddx1 = (v11 - v01);

            Math::Vector2<T> grad(Math::lerp(ddx0, ddx1, fy),
                                  Math::lerp(ddy0, ddy1, fx));
            const T magSqr = grad.dot();
            if(magSqr > T(1e-20)) {
                grad /= Math::sqrt(magSqr);
            }
            return grad;
        }

    private:
        std::size_t _size[2]{};
        std::vector<T> _data;
};

}}

#endif
