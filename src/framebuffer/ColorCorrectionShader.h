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

#include "AbstractShaderProgram.h"
#include "Texture.h"

namespace Magnum { namespace Examples {

/*
Fragment data:
0 - Original color
1 - Grayscale
2 - Color corrected
*/
class ColorCorrectionShader: public AbstractShaderProgram {
    public:
        typedef Attribute<0, Vector4> Vertex;

        ColorCorrectionShader();

        inline void setMatrixUniform(const Matrix4& matrix) {
            setUniform(matrixUniform, matrix);
        }

        inline void setTextureUniform(const Texture2D* texture) {
            setUniform(textureUniform, texture);
        }

        inline void setCorrectionTextureUniform(const BufferedTexture* texture) {
            setUniform(correctionTextureUniform, texture);
        }

    private:
        GLint matrixUniform,
            textureUniform,
            correctionTextureUniform;
};

}}

#endif
