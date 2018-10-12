EAPI=5

EGIT_REPO_URI="git://github.com/mosra/magnum-examples.git"

inherit cmake-utils git-r3

DESCRIPTION="Examples for the Magnum C++11/C++14 graphics engine"
HOMEPAGE="http://magnum.graphics"

LICENSE="public-domain"
SLOT="0"
KEYWORDS="~amd64 ~x86"
IUSE=""

RDEPEND="
	dev-libs/magnum
	dev-libs/magnum-plugins
	dev-libs/magnum-integration
	dev-libs/magnum-extras
	dev-libs/box2d
	media-libs/openal
	sci-physics/bullet
"
DEPEND="${RDEPEND}"

src_configure() {
    # general configuration
	local mycmakeargs=(
		-DCMAKE_INSTALL_PREFIX="${EPREFIX}/usr"
		-DCMAKE_BUILD_TYPE=Debug
		-DWITH_AREALIGHTS_EXAMPLE=ON
		-DWITH_AUDIO_EXAMPLE=ON
		-DWITH_BOX2D_EXAMPLE=ON
		-DWITH_BULLET_EXAMPLE=ON
		-DWITH_CUBEMAP_EXAMPLE=ON
		-DWITH_MOTIONBLUR_EXAMPLE=ON
		-DWITH_MOUSEINTERACTION_EXAMPLE=ON
		-DWITH_PICKING_EXAMPLE=ON
		-DWITH_PRIMITIVES_EXAMPLE=ON
		-DWITH_SHADOWS_EXAMPLE=ON
		-DWITH_TEXT_EXAMPLE=ON
		-DWITH_TEXTUREDTRIANGLE_EXAMPLE=ON
		-DWITH_TRIANGLE_EXAMPLE=ON
		-DWITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=OFF
		-DWITH_TRIANGLE_SOKOL_EXAMPLE=OFF
		-DWITH_VIEWER_EXAMPLE=ON
	)
	cmake-utils_src_configure
}

# kate: replace-tabs off;
