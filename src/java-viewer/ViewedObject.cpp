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

#include "ViewedObject.h"

namespace Magnum { namespace Examples {

void ViewedObject::draw(const Magnum::Matrix4& transformationMatrix, Magnum::Camera* camera) {
    shader->use();
    shader->setAmbientColorUniform(ambientColor);
    shader->setDiffuseColorUniform(diffuseColor);
    shader->setSpecularColorUniform(specularColor);
    shader->setShininessUniform(shininess);

    shader->setLightUniform(0, (camera->cameraMatrix()*Vector4(-30.0f, 100.0f, 100.0f)).xyz());
    shader->setLightUniform(1, (camera->cameraMatrix()*Vector4(60.0f, 100.0f, 100.0f)).xyz());
    shader->setLightUniform(2, (camera->cameraMatrix()*Vector4(0.0f, 100.0f, -100.0f)).xyz());

    shader->setTransformationMatrixUniform(transformationMatrix);
    shader->setProjectionMatrixUniform(camera->projectionMatrix());

    mesh->draw();
}

}}
