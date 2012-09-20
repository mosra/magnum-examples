#ifndef Magnum_Examples_ReflectionShader_h
#define Magnum_Examples_ReflectionShader_h
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

#include <AbstractShaderProgram.h>
#include <Color.h>

namespace Magnum { namespace Examples {

class ReflectionShader: public AbstractShaderProgram {
    public:
        typedef Attribute<0, Point3D> Position;
        typedef Attribute<1, Vector2> TextureCoords;

        static const GLint TextureLayer = 0;
        static const GLint TarnishTextureLayer = 1;

        ReflectionShader();

        inline ReflectionShader* setModelViewMatrix(const Matrix4& matrix) {
            setUniform(modelViewMatrixUniform, matrix);
            return this;
        }

        inline ReflectionShader* setProjectionMatrix(const Matrix4& matrix) {
            setUniform(projectionMatrixUniform, matrix);
            return this;
        }

        inline ReflectionShader* setCameraMatrix(const Matrix4& matrix) {
            setUniform(cameraMatrixUniform, matrix);
            return this;
        }

        inline ReflectionShader* setReflectivity(GLfloat reflectivity) {
            setUniform(reflectivityUniform, reflectivity);
            return this;
        }

        inline ReflectionShader* setDiffuseColor(const Color3<GLfloat>& color) {
            setUniform(diffuseColorUniform, color);
            return this;
        }

    private:
        GLint modelViewMatrixUniform,
            projectionMatrixUniform,
            cameraMatrixUniform,
            reflectivityUniform,
            diffuseColorUniform;
};

}}

#endif
