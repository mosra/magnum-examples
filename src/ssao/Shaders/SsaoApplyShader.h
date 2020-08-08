#ifndef Magnum_Examples_Ssao_SsaoApplyShader_h
#define Magnum_Examples_Ssao_SsaoApplyShader_h

/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 —
            Vladimír Vondruš <mosra@centrum.cz>
        2020 — Janos Meny <janos.meny@gmail.com>

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
#include <Magnum/Shaders/Generic.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Color.h>

namespace Magnum { namespace Examples {

class SsaoApplyShader : public Magnum::GL::AbstractShaderProgram {
public:

    enum class Flag : UnsignedInt {
        DrawAmbientOcclusion = 1,
    };

    explicit SsaoApplyShader(Flag flag = {});

    explicit SsaoApplyShader(Magnum::NoCreateT) : Magnum::GL::AbstractShaderProgram{Magnum::NoCreate} {};

    SsaoApplyShader& bindAlbedoTexture(Magnum::GL::Texture2D&);

    SsaoApplyShader& bindOcclusionTexture(Magnum::GL::Texture2D&);

    SsaoApplyShader& bindPositionTexture(Magnum::GL::Texture2D&);

    SsaoApplyShader& bindNormalTexture(Magnum::GL::Texture2D&);

    SsaoApplyShader& setShininess(Float);

    SsaoApplyShader& setLightPosition(const Vector3&);

    SsaoApplyShader& setLightColor(const Color3&);

    SsaoApplyShader& setSpecularColor(const Color3&);
private:
    Int _lightPositionUniform = 0,
        _lightColorUniform = 1,
        _shininessUniform = 2,
        _specularColorUniform = 3;
};

}}

#endif