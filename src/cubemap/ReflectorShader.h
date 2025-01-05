#ifndef Magnum_Examples_CubeMap_ReflectorShader_h
#define Magnum_Examples_CubeMap_ReflectorShader_h
/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Matrix4.h>

namespace Magnum { namespace Examples {

class ReflectorShader: public GL::AbstractShaderProgram {
    public:
        typedef GL::Attribute<0, Vector3> Position;
        typedef GL::Attribute<1, Vector2> TextureCoords;

        explicit ReflectorShader();

        ReflectorShader& setTransformationMatrix(const Matrix4& matrix) {
            setUniform(_transformationMatrixUniform, matrix);
            return *this;
        }

        ReflectorShader& setNormalMatrix(const Matrix3& matrix) {
            setUniform(_normalMatrixUniform, matrix);
            return *this;
        }

        ReflectorShader& setProjectionMatrix(const Matrix4& matrix) {
            setUniform(_projectionMatrixUniform, matrix);
            return *this;
        }

        ReflectorShader& setCameraMatrix(const Matrix3& matrix) {
            setUniform(_cameraMatrixUniform, matrix);
            return *this;
        }

        ReflectorShader& setReflectivity(Float reflectivity) {
            setUniform(_reflectivityUniform, reflectivity);
            return *this;
        }

        ReflectorShader& setDiffuseColor(const Color3& color) {
            setUniform(_diffuseColorUniform, color);
            return *this;
        }

        ReflectorShader& setTexture(GL::CubeMapTexture& texture);

        ReflectorShader& setTarnishTexture(GL::Texture2D& texture);

    private:
        Int _transformationMatrixUniform,
            _normalMatrixUniform,
            _projectionMatrixUniform,
            _cameraMatrixUniform,
            _reflectivityUniform,
            _diffuseColorUniform;
};

}}

#endif
