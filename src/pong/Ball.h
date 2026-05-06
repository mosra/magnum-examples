/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023 — Vladimír Vondruš <mosra@centrum.cz>
        2023 — Pablo Duboue <pablo.duboue@gmail.com>

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

#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Range.h>
#include <Magnum/Shaders/VertexColorGL.h>

#include "Ball.h"

namespace Magnum { namespace Examples {
    
class Ball {
public:
    Ball(float x, float y);
    ~Ball();

    void update();
    void draw();
    void bounce();
    void start();

    float x() { return _position.x(); }
    float y() { return _position.y(); }
    bool touches(Math::Range2D<float> other) {
      return Math::intersects<2, float>(Math::Range2D<float> { _position, _position + Math::Vector2<float>(WIDTH, HEIGHT) }, other);
    }

private:
    const float WIDTH = 0.02;
    const float HEIGHT = 0.02;
    Math::Vector2<float> _position;
    GL::Mesh _mesh;
    Shaders::VertexColorGL2D _shader;
    int _dirX;
    int _dirY;
    int _startX;
    int _startY;
    float _speed;
};

}}
