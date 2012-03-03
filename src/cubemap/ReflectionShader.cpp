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

#include "ReflectionShader.h"

#include "Utility/Resource.h"

using namespace Corrade::Utility;

namespace Magnum { namespace Examples {

ReflectionShader::ReflectionShader() {
    Resource rs("data");
    Shader* vertexShader = Shader::fromData(Shader::Vertex, rs.get("ReflectionShader.vert"));
    Shader* fragmentShader = Shader::fromData(Shader::Fragment, rs.get("ReflectionShader.frag"));

    attachShader(vertexShader);
    attachShader(fragmentShader);

    bindAttribute(Vertex::Location, "vertex");

    link();

    modelViewMatrixUniform = uniformLocation("modelViewMatrix");
    projectionMatrixUniform = uniformLocation("projectionMatrix");
    cameraMatrixUniform = uniformLocation("cameraMatrix");
    reflectivityUniform = uniformLocation("reflectivity");
    diffuseColorUniform = uniformLocation("diffuseColor");
    textureUniform = uniformLocation("textureData");

    delete vertexShader;
    delete fragmentShader;
}

}}
