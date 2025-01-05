#ifndef Magnum_Examples_FluidSimulation2D_SDFObject_h
#define Magnum_Examples_FluidSimulation2D_SDFObject_h
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

#include <Corrade/Containers/Pointer.h>
#include <Corrade/Utility/Debug.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Vector2.h>

namespace Magnum { namespace Examples {

struct SDFObject {
    enum class ObjectType {
        /* Basic primitives */
        Circle = 0,
        Box,

        /* Boolean operations */
        Intersection,
        Subtraction,
        Union
    };

    explicit SDFObject(): center{0, 0}, radii{1}, type{ObjectType::Circle}, negativeInside{true} {}

    explicit SDFObject(const Vector2& center_, Float radius_, ObjectType type_, bool negativeInside_ = true):
        SDFObject(center_, Vector2{radius_, 0}, type_, negativeInside_) {}

    explicit SDFObject(const Vector2& center_, const Vector2& radii_, ObjectType type_, bool negativeInside_ = true):
        center{center_}, radii{radii_}, type{type_},
        negativeInside{negativeInside_}
    {
        if(type != ObjectType::Circle
           && type != ObjectType::Box) {
            Fatal{} << "Invalid object type";
        }
    }

    explicit SDFObject(SDFObject* obj1_, SDFObject* obj2_, ObjectType type_):
        type{type_}, obj1{obj1_}, obj2{obj2_}
    {
        if(type != ObjectType::Intersection &&
           type != ObjectType::Subtraction &&
           type != ObjectType::Union) {
            Fatal{} << "Invalid boolean operation";
        }
    }

    Float signedDistance(const Vector2& pos) const {
        switch(type) {
            case ObjectType::Circle: {
                const Float dist = (pos - center).length() - radii[0];
                return negativeInside ? dist : -dist;
            }
            case ObjectType::Box: {
                const Float dx = Math::abs(pos[0] - center[0]) - radii[0];
                const Float dy = Math::abs(pos[1] - center[1]) - radii[1];
                Float dist { 0 };
                if(dx < 0 && dy < 0) {
                    dist = Math::max(dx, dy);
                } else {
                    const Float dax = Math::max(dx, 0.0f);
                    const Float day = Math::max(dy, 0.0f);
                    dist = Math::sqrt(dax*dax + day*day);
                }
                return negativeInside ? dist : -dist;
            }

            case ObjectType::Intersection:
                return Math::max(obj1->signedDistance(pos), obj2->signedDistance(pos));
            case ObjectType::Subtraction:
                return Math::max(obj1->signedDistance(pos), -obj2->signedDistance(pos));
            case ObjectType::Union:
                return Math::min(obj1->signedDistance(pos), obj2->signedDistance(pos));
        }

        return 0;
    }

    Vector2 center;
    Vector2 radii;
    ObjectType type;
    bool negativeInside { true };

    Containers::Pointer<SDFObject> obj1;
    Containers::Pointer<SDFObject> obj2;
};

}}

#endif
