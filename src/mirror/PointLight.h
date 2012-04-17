#ifndef Magnum_Examples_PointLight_h
#define Magnum_Examples_PointLight_h
/*
    Copyright © 2010, 2011, 2012 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Magnum.

    Magnum is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Magnum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

#include "Light.h"

namespace Magnum { namespace Examples {

class PointLight: public Light {
    public:
        inline PointLight(Object* parent = nullptr): Light(parent) {}

        inline Vector3 ambientColor() { return {0.0f, 0.0f, 0.0f}; }
        inline Vector3 diffuseColor() { return {1.0f, 1.0f, 1.0f}; }
        inline Vector3 specularColor() { return {1.0f, 1.0f, 1.0f}; }
};

}}

#endif