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

#include "Player.h"

#include "Primitives/Capsule.h"

namespace Magnum { namespace Examples {

Player::Player(Shaders::PhongShader* shader, PointLight* light, Object* parent): Solid(Primitives::Capsule(16, 32, 2.5f), shader, light, {0.6f, 0.2f, 0.2f}, parent), _camera(this) {
    translate(Vector3::yAxis(1.25f));

    _camera.setClearColor({0.9f, 0.9f, 0.9f});
    _camera.setPerspective(deg(70.0f), 0.001f, 1000.0f);
    _camera.rotate(deg(-20.0f), Vector3::xAxis());
    _camera.translate({0.0f, 4.0f, 6.0f});
}

void Player::draw(const Matrix4& transformationMatrix, Camera* camera) {
    Solid::draw(transformationMatrix, camera);
}

}}
