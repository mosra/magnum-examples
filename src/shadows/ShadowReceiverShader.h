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
#include <Magnum/Shaders/Generic.h>

/// Shader that can synthesize shadows on an object
class ShadowReceiverShader : public Magnum::AbstractShaderProgram {
public:
	ShadowReceiverShader(int numShaderLevels);
	virtual ~ShadowReceiverShader();

	typedef Magnum::Shaders::Generic3D::Position Position;
	typedef Magnum::Shaders::Generic3D::Normal Normal;

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



