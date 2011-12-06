#ifndef Magnum_Examples_ReflectionShader_h
#define Magnum_Examples_ReflectionShader_h
/*
    Copyright © 2010, 2011 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Magnum.

    Magnum is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Magnum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

#include "AbstractShaderProgram.h"

#include "CubeMapTexture.h"

namespace Magnum { namespace Examples {

class ReflectionShader: public AbstractShaderProgram {
    public:
        enum Attribute {
            Vertex = 1
        };

        ReflectionShader();

        void setModelViewMatrixUniform(const Matrix4& matrix) {
            setUniform(modelViewMatrixUniform, matrix);
        }

        void setProjectionMatrixUniform(const Matrix4& matrix) {
            setUniform(projectionMatrixUniform, matrix);
        }

        void setCameraMatrixUniform(const Matrix4& matrix) {
            setUniform(cameraMatrixUniform, matrix);
        }

        void setReflectivityUniform(GLfloat reflectivity) {
            setUniform(reflectivityUniform, reflectivity);
        }

        void setDiffuseColorUniform(const Vector3& color) {
            setUniform(diffuseColorUniform, color);
        }

        void setTextureUniform(const CubeMapTexture* texture) {
            setUniform(textureUniform, texture);
        }

    private:
        GLint modelViewMatrixUniform,
            projectionMatrixUniform,
            cameraMatrixUniform,
            reflectivityUniform,
            diffuseColorUniform,
            textureUniform;
};

}}

#endif
