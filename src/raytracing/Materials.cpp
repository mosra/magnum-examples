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

#include <Magnum/Math/Functions.h>

#include "RndGenerators.h"
#include "Materials.h"
#include "Ray.h"

namespace Magnum { namespace Examples {
struct HitInfo;

namespace {
inline Float schlick(Float cosine, Float ref_idx) {
    Float r0 = (1 - ref_idx) / (1 + ref_idx);
    r0 = r0 * r0;
    return r0 + (1 - r0) * std::pow((1 - cosine), 5);
}
}

bool Lambertian::scatter(const Ray&, const HitInfo& hitInfo, Vector3& attenuation, Ray& scatteredRay) const {
    const Vector3 target = hitInfo.p + hitInfo.unitNormal + Rnd::randomInSphere();
    scatteredRay = Ray(hitInfo.p, target - hitInfo.p);
    attenuation  = _albedo;
    return true;
}

bool Metal::scatter(const Ray& r, const HitInfo& hitInfo, Vector3& attenuation, Ray& scatteredRay) const {
    const Vector3 reflectedRay = Math::reflect(r.unitDirection, hitInfo.unitNormal);
    scatteredRay = Ray(hitInfo.p, reflectedRay + _fuzziness * Rnd::randomInSphere());
    attenuation  = _albedo;
    return (dot(scatteredRay.unitDirection, hitInfo.unitNormal) > 0);
}

bool Dielectric::scatter(const Ray& r, const HitInfo& hitInfo, Vector3& attenuation, Ray& scatteredRay) const {
    attenuation = Vector3{ 1, 1, 1 };

    Float   ni_over_nt;
    Float   cosine;
    Vector3 outwardNormal;
    if(dot(r.unitDirection, hitInfo.unitNormal) > 0) {
        outwardNormal = -hitInfo.unitNormal;
        ni_over_nt    = _refractiveIndex;
        cosine        = dot(r.unitDirection, hitInfo.unitNormal);
        cosine        = std::sqrt(1 - _refractiveIndex * _refractiveIndex * (1 - cosine * cosine));
    } else {
        outwardNormal = hitInfo.unitNormal;
        ni_over_nt    = 1.0f / _refractiveIndex;
        cosine        = -dot(r.unitDirection, hitInfo.unitNormal);
    }

    const Vector3 refractedDir          = Math::refract(r.unitDirection, outwardNormal, ni_over_nt);
    const Float   reflectionProbability = dot(refractedDir, refractedDir) > 0 ?
                                          schlick(cosine, _refractiveIndex) :
                                          1.0f;
    scatteredRay.origin = hitInfo.p;
    if(Rnd::rand01() < reflectionProbability) {
        scatteredRay.unitDirection = Math::reflect(r.unitDirection, hitInfo.unitNormal);
    } else {
        scatteredRay.unitDirection = refractedDir.normalized();
    }
    return true;
}
} }
