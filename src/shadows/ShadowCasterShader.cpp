#include "ShadowCasterShader.h"

#include <Corrade/Utility/Resource.h>
#include <Magnum/Context.h>
#include <Magnum/Shader.h>
#include <Magnum/Version.h>
#include <Magnum/Math/Matrix4.h>

using namespace Magnum;

ShadowCasterShader::ShadowCasterShader() {
	MAGNUM_ASSERT_VERSION_SUPPORTED(Version::GL330);

	const Utility::Resource rs{"shadow-data"};

	Shader vert{Version::GL330, Shader::Type::Vertex};
	Shader frag{Version::GL330, Shader::Type::Fragment};

	vert.addSource(rs.get("ShadowCaster.vert"));
	frag.addSource(rs.get("ShadowCaster.frag"));

	CORRADE_INTERNAL_ASSERT_OUTPUT(Shader::compile({vert, frag}));

	attachShaders({vert, frag});

	CORRADE_INTERNAL_ASSERT_OUTPUT(link());

	transformationMatrixUniform = uniformLocation("transformationMatrix");
}

ShadowCasterShader::~ShadowCasterShader() {

}

ShadowCasterShader &ShadowCasterShader::setTransformationMatrix(const Magnum::Matrix4 &matrix) {
	setUniform(transformationMatrixUniform, matrix);
	return *this;
}
