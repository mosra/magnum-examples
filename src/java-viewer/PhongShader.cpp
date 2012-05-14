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

#include "PhongShader.h"

#include <sstream>
#include <Utility/Resource.h>

using namespace std;
using namespace Corrade::Utility;

namespace Magnum { namespace Examples {

PhongShader::PhongShader() {
    Resource rs("shaders");
    stringstream ss;
    ss << "#define LIGHT_COUNT " << LightCount << std::endl;

    Shader vertexShader(Shader::Type::Vertex);
    vertexShader.addSource(ss.str());
    vertexShader.addSource(rs.get("PhongShader.vert"));
    attachShader(vertexShader);

    Shader fragmentShader(Shader::Type::Fragment);
    fragmentShader.addSource(ss.str());
    fragmentShader.addSource(rs.get("PhongShader.frag"));
    attachShader(fragmentShader);

    bindAttribute(Vertex::Location, "vertex");
    bindAttribute(Normal::Location, "normal");

    link();

    ambientColorUniform = uniformLocation("ambientColor");
    diffuseColorUniform = uniformLocation("diffuseColor");
    specularColorUniform = uniformLocation("specularColor");
    shininessUniform = uniformLocation("shininess");
    transformationMatrixUniform = uniformLocation("transformationMatrix");
    projectionMatrixUniform = uniformLocation("projectionMatrix");

    for(unsigned int i = 0; i != LightCount; ++i) {
        ss.str("");
        ss << i << ']';
        lightUniform[i] = uniformLocation("light[" + ss.str());
        lightColorUniform[i] = uniformLocation("lightColor[" + ss.str());
    }
}

}}
