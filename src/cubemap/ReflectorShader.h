#ifndef Magnum_Examples_ReflectorShader_h
#define Magnum_Examples_ReflectorShader_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014
              Vladimír Vondruš <mosra@centrum.cz>

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

#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/AbstractShaderProgram.h>
#include <Magnum/Color.h>

namespace Magnum { namespace Examples {

class ReflectorShader: public AbstractShaderProgram {
    public:
        typedef Attribute<0, Vector3> Position;
        typedef Attribute<1, Vector2> TextureCoords;

        explicit ReflectorShader();

        ReflectorShader& setTransformationMatrix(const Matrix4& matrix) {
            setUniform(transformationMatrixUniform, matrix);
            return *this;
        }

        ReflectorShader& setNormalMatrix(const Matrix3& matrix) {
            setUniform(normalMatrixUniform, matrix);
            return *this;
        }

        ReflectorShader& setProjectionMatrix(const Matrix4& matrix) {
            setUniform(projectionMatrixUniform, matrix);
            return *this;
        }

        ReflectorShader& setCameraMatrix(const Matrix3& matrix) {
            setUniform(cameraMatrixUniform, matrix);
            return *this;
        }

        ReflectorShader& setReflectivity(Float reflectivity) {
            setUniform(reflectivityUniform, reflectivity);
            return *this;
        }

        ReflectorShader& setDiffuseColor(const Color3& color) {
            setUniform(diffuseColorUniform, color);
            return *this;
        }

        ReflectorShader& setTexture(CubeMapTexture& texture);

        ReflectorShader& setTarnishTexture(Texture2D& texture);

    private:
        Int transformationMatrixUniform,
            normalMatrixUniform,
            projectionMatrixUniform,
            cameraMatrixUniform,
            reflectivityUniform,
            diffuseColorUniform;
};

}}

#endif
