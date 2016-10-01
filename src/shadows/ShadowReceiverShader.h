/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016 —
            Vladimír Vondruš <mosra@centrum.cz>
        2016 — Bill Robinson <airbaggins@gmail.com>

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

#pragma once

#include <Magnum/AbstractShaderProgram.h>
#include <Magnum/Shaders/Generic.h>

/// Shader that can synthesize shadows on an object
class ShadowReceiverShader : public Magnum::AbstractShaderProgram {
public:
    typedef Magnum::Shaders::Generic3D::Position Position;
    typedef Magnum::Shaders::Generic3D::Normal Normal;

    ShadowReceiverShader(int numShadowLevels);
    virtual ~ShadowReceiverShader();

    /** Matrix that transforms from local model space -> world space -> camera space -> clip coordinates (aka model-view-projection matrix)  */
    ShadowReceiverShader& setTransformationProjectionMatrix(const Magnum::Matrix4& matrix);

    /** Matrix that transforms from local model space -> world space (used for lighting) (aka model matrix) */
    ShadowReceiverShader& setModelMatrix(const Magnum::Matrix4& matrix);

    /** Matrix that transforms from world space -> shadow texture space */
    ShadowReceiverShader& setShadowmapMatrices(const Corrade::Containers::ArrayView<Magnum::Matrix4>& matrix);

    /** World-space direction to the light source */
    ShadowReceiverShader& setLightDirection(const Magnum::Vector3& vector3);

    /** The shadow map texture array */
    ShadowReceiverShader& setShadowmapTexture(Magnum::Texture2DArray& texture);

    /** Shadow bias uniform - normally it wants to be something from 0.0001 -> 0.001 */
    ShadowReceiverShader& setShadowBias(float bias);

private:
    Magnum::Int modelMatrixUniform;
    Magnum::Int transformationProjectionMatrixUniform;
    Magnum::Int shadowmapMatrixUniform;
    Magnum::Int lightDirectionUniform;
    Magnum::Int shadowBiasUniform;

    enum: Magnum::Int { ShadowmapTextureLayer = 0 };
};



