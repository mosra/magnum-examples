#ifndef Magnum_Examples_CubeMapShader_h
#define Magnum_Examples_CubeMapShader_h
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

class CubeMapShader: public AbstractShaderProgram {
    public:
        enum Attribute {
            Vertex = 1
        };

        CubeMapShader();

        void setModelViewProjectionMatrixUniform(const Matrix4& matrix) {
            setUniform(modelViewProjectionMatrixUniform, matrix);
        }

        void setTextureUniform(const CubeMapTexture* texture) {
            setUniform(textureUniform, texture);
        }

    private:
        GLint textureUniform, modelViewProjectionMatrixUniform;
};

}}

#endif
