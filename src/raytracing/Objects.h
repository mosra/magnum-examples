#ifndef Magnum_Examples_RayTracing_Objects_h
#define Magnum_Examples_RayTracing_Objects_h
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

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/Pointer.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>

namespace Magnum { namespace Examples {

struct Ray;
struct HitInfo;
class Material;

class Object {
    public:
        virtual ~Object() = default;

        virtual bool intersect(const Ray& r, Float t_min, Float t_max, HitInfo& hitInfo) const = 0;
};

class Sphere: public Object {
    public:
        explicit Sphere(const Vector3& center, Float radius,
            Containers::Pointer<Material>&& material): _center{center},
            _radiusSqr{radius*radius}, _material{Utility::move(material)} {}

        bool intersect(const Ray& r, Float tMin, Float tMax, HitInfo& hitInfo) const override;

    private:
        void computeHitInfo(const Ray& r, Float t, HitInfo& hitInfo) const;

        Vector3 _center;
        Float _radiusSqr;
        Containers::Pointer<Material> _material;
};

class ObjectList: public Object {
    public:
        bool intersect(const Ray& r, Float tMin, Float tMax, HitInfo& hitInfo) const override;
        void addObject(Containers::Pointer<Object>&& object);

    private:
        Containers::Array<Containers::Pointer<Object>> _objects;
};

}}

#endif
