#ifndef Magnum_Examples_ColorCorrectionShader_h
#define Magnum_Examples_ColorCorrectionShader_h
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

#include <Math/Matrix3.h>
#include <AbstractShaderProgram.h>

namespace Magnum { namespace Examples {

class ColorCorrectionShader: public AbstractShaderProgram {
    public:
        typedef Attribute<0, Vector2> Position;

        enum: GLuint {
            OriginalColorOutput = 0,
            GrayscaleOutput = 1,
            ColorCorrectedOutput = 2
        };

        enum: GLint {
            TextureLayer = 0,
            ColorCorrectionTextureLayer = 1
        };

        ColorCorrectionShader();

        inline ColorCorrectionShader* setTransformationProjectionMatrix(const Matrix3& matrix) {
            setUniform(transformationProjectionMatrixUniform, matrix);
            return this;
        }

    private:
        GLint transformationProjectionMatrixUniform;
};

}}

#endif
