#ifndef Magnum_Examples_ColorCorrectionShader_h
#define Magnum_Examples_ColorCorrectionShader_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013 Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include <Math/Matrix3.h>
#include <AbstractShaderProgram.h>

namespace Magnum { namespace Examples {

class ColorCorrectionShader: public AbstractShaderProgram {
    public:
        typedef Attribute<0, Vector2> Position;

        enum: UnsignedInt {
            OriginalColorOutput = 0,
            GrayscaleOutput = 1,
            ColorCorrectedOutput = 2
        };

        enum: Int {
            TextureLayer = 0,
            ColorCorrectionTextureLayer = 1
        };

        ColorCorrectionShader();

        inline ColorCorrectionShader& setTransformationProjectionMatrix(const Matrix3& matrix) {
            setUniform(transformationProjectionMatrixUniform, matrix);
            return *this;
        }

    private:
        Int transformationProjectionMatrixUniform;
};

}}

#endif
