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
