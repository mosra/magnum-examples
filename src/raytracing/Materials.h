#ifndef Magnum_Examples_RayTracing_Materials_h
#define Magnum_Examples_RayTracing_Materials_h
/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>
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

#include <Magnum/Magnum.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Vector3.h>

namespace Magnum { namespace Examples {

struct Ray;
struct HitInfo;

class Material {
    public:
        virtual ~Material() = default;

        virtual bool scatter(const Ray& r, const HitInfo& hitInfo,
            Vector3& attenuation, Ray& scatteredRay) const = 0;
};

class Lambertian: public Material {
    public:
        explicit Lambertian(const Vector3& albedo): _albedo{albedo} {}

        bool scatter(const Ray& r, const HitInfo& hitInfo,
            Vector3& attenuation, Ray& scatteredRay) const override;

    private:
        Vector3 _albedo;
};

class Metal: public Material {
    public:
        explicit Metal(const Vector3& albedo, Float f): _albedo{albedo},
            _fuzziness{Math::min(f, 1.0f)} {}

        bool scatter(const Ray& r, const HitInfo& hitInfo,
            Vector3& attenuation, Ray& scatteredRay) const override;

    private:
        Vector3 _albedo;
        Float _fuzziness;
};

class Dielectric: public Material {
    public:
        explicit Dielectric(Float refractiveIndex):
            _refractiveIndex{refractiveIndex} {}

        bool scatter(const Ray& r, const HitInfo& hitInfo,
            Vector3& attenuation, Ray& scatteredRay) const override;

    private:
        Float _refractiveIndex;
};

}}

#endif
