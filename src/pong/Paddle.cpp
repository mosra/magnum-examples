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

#include "Paddle.h"

namespace Magnum { namespace Examples {

Paddle::Paddle(float x, float y, bool human) : _is_human(human) {
  _position = Math::Vector2<float>(x, y);
  _speed = 0.005f;
  _momentum = 1.0f;
  _dir = 0;
  _has_key = false;

  using namespace Math::Literals;

  struct BoxVertex {
    Vector2 position;
    Color3 color;
  };
  const BoxVertex vertices[]{
        {{0.0f,  0.0f},  human ? 0xff0000_rgbf : 0x00ff00_rgbf},
        {{WIDTH,  0.0f}, 0x00ff00_rgbf},
        {{WIDTH, HEIGHT}, 0x0000ff_rgbf}
    };
  _mesh.setCount(Containers::arraySize(vertices))
         .addVertexBuffer(GL::Buffer{vertices}, 0,
            Shaders::VertexColorGL2D::Position{},
            Shaders::VertexColorGL2D::Color3{});
}

Paddle::~Paddle() {
}

void Paddle::handleEvent(Platform::Application::KeyEvent& event) {
  if (_is_human) {
    if (event.key() == Platform::Application::KeyEvent::Key::Up) {
        if(_dir < 0) {
          _momentum *= 1.01f;
        }else{
          _speed = 0.005f;
          _momentum = 1.0f;
        }
        if (_momentum > 1.15f) {
          _momentum = 1.15f;
        }
        if (_speed > 0.01f) {
          _speed = 0.01f;
        }
        auto delta = Math::Vector2<float>(0, _speed);
        _position = _position + delta;
        _speed *= _momentum;
        _dir = -1;
        _has_key = true;
        event.setAccepted();
    } else if (event.key() == Platform::Application::KeyEvent::Key::Down) {
        if(_dir > 0) {
          _momentum *= 1.01f;
        }else{
          _speed = 0.005f;
          _momentum = 1.0f;
        }
        if (_momentum > 1.15f) {
          _momentum = 1.15f;
        }
        if (_speed > 0.01f) {
          _speed = 0.01f;
        }
        auto delta = Math::Vector2<float>(0, _speed);
        _position = _position - delta;
        _speed *= _momentum;
        _dir = 1;
        _has_key = true;
        event.setAccepted();
    }
  }
}

void Paddle::update(Ball *ball) {
  if(!_has_key) {
    if(_momentum > 1.0f){
      _momentum *= 0.9f;
    }
    if(_speed > 0.005f){
      _speed -= 0.0001f;
    }
  }
  
  if(ball->touches(Math::Range2D<float>(_position, _position + Math::Vector2<float>(WIDTH, HEIGHT)))) {
    ball->bounce();
  }
  
  if(!this->_is_human){
    auto delta = Math::Vector2<float>(0, _speed);
    if (_position.y() < ball->y()) {
      _position = _position + delta;
    }else if(_position.y() > ball->y()) {
      _position = _position - delta;
    }
  }
  if (_position.y() < -1.0f) {
    _position = Math::Vector2<float>(_position.x(), 1.0f);
  } else if (_position.y() > 1.0f) {
    _position = Math::Vector2<float>(_position.x(), 1.0f);
  }
}

void Paddle::draw() {
  auto transformation = Matrix3::translation(_position);
  _shader.setTransformationProjectionMatrix(transformation)
    .draw(_mesh);
}

}}
