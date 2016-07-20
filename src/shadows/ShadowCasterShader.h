#pragma once

#include <Magnum/AbstractShaderProgram.h>

class ShadowCasterShader : public Magnum::AbstractShaderProgram {
public:
	ShadowCasterShader();
	virtual ~ShadowCasterShader();

	ShadowCasterShader& setTransformationMatrix(const Magnum::Matrix4& matrix);

private:
	Magnum::Int transformationMatrixUniform;
};



