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



