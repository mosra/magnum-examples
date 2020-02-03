/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
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

#include "RndGenerators.h"
#include "Materials.h"
#include "Ray.h"

namespace Magnum { namespace Examples {
struct HitInfo;

namespace Math {
inline Vector3 reflect(const Vector3& v, const Vector3& n) {
    return v - 2 * dot(v, n) * n;
}

inline bool refract(const Vector3& v, const Vector3& n, Float ni_over_nt, Vector3& refracted) {
    const Vector3 uv           = v.normalized();
    const Float   dt           = dot(uv, n);
    const Float   discriminant = 1.0 - ni_over_nt * ni_over_nt * (1 - dt * dt);
    if(discriminant > 0) {
        refracted = ni_over_nt * (uv - n * dt) - n * sqrt(discriminant);
        return true;
    }
    return false;
}

inline Float schlick(Float cosine, Float ref_idx) {
    Float r0 = (1 - ref_idx) / (1 + ref_idx);
    r0 = r0 * r0;
    return r0 + (1 - r0) * std::pow((1 - cosine), 5);
}
}

bool Lambertian::scatter(const Ray&, const HitInfo& hitInfo, Vector3& attenuation, Ray& scatteredRay) const {
    const Vector3 target = hitInfo.p + hitInfo.normal + Rnd::randomInSphere();
    scatteredRay = Ray(hitInfo.p, target - hitInfo.p);
    attenuation  = _albedo;
    return true;
}

bool Metal::scatter(const Ray& r, const HitInfo& hitInfo, Vector3& attenuation, Ray& scatteredRay) const {
    const Vector3 reflected = Math::reflect(r.direction.normalized(), hitInfo.normal);
    scatteredRay = Ray(hitInfo.p, reflected + _fuzziness * Rnd::randomInSphere());
    attenuation  = _albedo;
    return (dot(scatteredRay.direction, hitInfo.normal) > 0);
}

bool Dielectric::scatter(const Ray& r, const HitInfo& hitInfo, Vector3& attenuation, Ray& scatteredRay) const {
    attenuation = Vector3{ 1, 1, 1 };

    Float   ni_over_nt;
    Float   cosine;
    Vector3 outwardNormal;
    if(dot(r.direction, hitInfo.normal) > 0) {
        outwardNormal = -hitInfo.normal;
        ni_over_nt    = _refractiveIndex;
        cosine        = dot(r.direction, hitInfo.normal) / r.direction.length();
        cosine        = std::sqrt(1 - _refractiveIndex * _refractiveIndex * (1 - cosine * cosine));
    } else {
        outwardNormal = hitInfo.normal;
        ni_over_nt    = 1.0f / _refractiveIndex;
        cosine        = -dot(r.direction, hitInfo.normal) / r.direction.length();
    }

    Vector3     refractedDir;
    const Float reflectionProbability = Math::refract(r.direction, outwardNormal, ni_over_nt, refractedDir) ?
                                        Math::schlick(cosine, _refractiveIndex) :
                                        1.0f;
    scatteredRay = Ray(hitInfo.p, Rnd::rand01() < reflectionProbability ?
                       Math::reflect(r.direction, hitInfo.normal) :
                       refractedDir);
    return true;
}
} }
