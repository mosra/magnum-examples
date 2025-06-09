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

#include "Ball.h"

namespace Magnum { namespace Examples {
    
Ball::Ball(float x, float y) {
    _position = Math::Vector2<float>(x, y);
    
    _startX = x;
    _startY = y;
    _dirX = 1;
    _dirY = 1;
    _speed = 0.005f;

  using namespace Math::Literals;

  struct BoxVertex {
    Vector2 position;
    Color3 color;
  };
  const BoxVertex vertices[]{
        {{0.0f,    0.0f}, 0xfffff0_rgbf},
        {{WIDTH ,  0.0f}, 0xf0ffff_rgbf},
        {{WIDTH, HEIGHT}, 0xfff0ff_rgbf},
        {{0.0f,  HEIGHT}, 0xffffff_rgbf},
    };
  _mesh.setCount(Containers::arraySize(vertices))
         .addVertexBuffer(GL::Buffer{vertices}, 0,
            Shaders::VertexColorGL2D::Position{},
            Shaders::VertexColorGL2D::Color3{});    
}

Ball::~Ball() {
}

void Ball::bounce() {
  _dirX = -_dirX;
  _speed *= 1.1f;
}

void Ball::update() {
  auto delta = Math::Vector2<float>(_dirX * _speed, _dirY * _speed);
  _position = _position + delta;

  if (_speed > 0.005f) {
    _speed *= 0.99f;
  }

  if (_position.y() < -1.0f || _position.y() + HEIGHT > 1.0f) {
    _dirY = -_dirY;
  }

  if (_position.x() < -1.0f || _position.x() + WIDTH > 1.0f) {
    _dirX = -_dirX;
  }
}

void Ball::draw() {
  auto transformation = Matrix3::translation(_position);
  _shader.setTransformationProjectionMatrix(transformation)
    .draw(_mesh);  
}

void Ball::start() {
  _position = Math::Vector2<float>(_startX, _startY);
}

}}
