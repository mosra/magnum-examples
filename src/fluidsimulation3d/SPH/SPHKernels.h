#ifndef Magnum_Examples_FluidSimulation3D_SPH_SPHKernels_h
#define Magnum_Examples_FluidSimulation3D_SPH_SPHKernels_h
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

#include <Magnum/Magnum.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Vector3.h>

namespace Magnum { namespace Examples {

class Poly6Kernel {
    public:
        void setRadius(const Float radius) {
            _radius = radius;
            _radiusSqr = _radius*_radius;
            _k = 315.0f/(64.0f*Constants::pi()*Math::pow(_radius, 9.0f));
            _W0 = W(0.0f);
        }

        Float W(const Float r) const {
            const Float r2 = r * r;
            return r2 <= _radiusSqr ? Math::pow(_radiusSqr - r2, 3.0f)*_k : 0.0f;
        }

        Float W(const Vector3& r) const {
            const auto r2 = r.dot();
            return r2 <= _radiusSqr ? Math::pow(_radiusSqr - r2, 3.0f)*_k : 0.0f;
        }

        Float W0() const { return _W0; }

    private:
        Float _radius;
        Float _radiusSqr;
        Float _k;
        Float _W0;
};

class SpikyKernel {
    public:
        void setRadius(const Float radius) {
            _radius = radius;
            _radiusSqr = _radius * _radius;
            _l = -45.0f/(Constants::pi()*Math::pow(_radius, 6.0f));
        }

        Vector3 gradW(const Vector3& r) const {
            Vector3 res{0.0f};
            const Float r2 = r.dot();
            if(r2 <= _radiusSqr && r2 > 1.0e-12f) {
                const Float rl = Math::sqrt(r2);
                const Float hr = _radius - rl;
                const Float hr2 = hr * hr;
                res = _l*hr2*(r/rl);
            }

            return res;
        }

    protected:
        Float _radius;
        Float _radiusSqr;
        Float _l;
};

class SPHKernels {
    public:
        explicit SPHKernels(Float kernelRadius) {
            _poly6.setRadius(kernelRadius);
            _spiky.setRadius(kernelRadius);
        }

        Float W0() const { return _poly6.W0(); }
        Float W(const Vector3& r) const { return _poly6.W(r); }
        Vector3 gradW(const Vector3& r) const { return _spiky.gradW(r); }

    private:
        Poly6Kernel _poly6;
        SpikyKernel _spiky;
};

}}

#endif
