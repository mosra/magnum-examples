/* magnum-shadows - A Cascading/Parallel-Split Shadow Mapping example
 * Written in 2016 by Bill Robinson airbaggins@gmail.com
 *
 * To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to
 * this software to the public domain worldwide. This software is distributed without any warranty.
 *
 * See <http://creativecommons.org/publicdomain/zero/1.0/>.
 *
 * Credit is appreciated, but not required.
 * */
#pragma once

#include <Magnum/AbstractShaderProgram.h>

class ShadowCasterShader : public Magnum::AbstractShaderProgram {
public:
    ShadowCasterShader();
    virtual ~ShadowCasterShader();

    /** Matrix that transforms from local model space -> world space -> camera space -> clip coordinates (aka model-view-projection matrix)  */
    ShadowCasterShader& setTransformationMatrix(const Magnum::Matrix4& matrix);

private:
    Magnum::Int transformationMatrixUniform;
};



