# Copyright 1999-2014 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI=5

EGIT_REPO_URI="git://github.com/mosra/magnum-examples.git"

inherit cmake-utils git-r3

DESCRIPTION="Examples for Magnum OpenGL graphics engine"
HOMEPAGE="http://mosra.cz/blog/magnum.php"

LICENSE="MIT"
SLOT="0"
KEYWORDS="~amd64 ~x86"
IUSE=""

RDEPEND="
	dev-libs/magnum
	dev-libs/magnum-integration
	dev-libs/magnum-plugins
	sci-physics/bullet
"
DEPEND="${RDEPEND}"

src_configure() {
    # general configuration
	local mycmakeargs=(
		-DCMAKE_INSTALL_PREFIX="${EPREFIX}/usr"
		-DCMAKE_BUILD_TYPE=Debug
		-DWITH_BULLET=ON
		-DWITH_CUBEMAP=ON
		-DWITH_MOTIONBLUR=ON
		-DWITH_PICKING=ON
		-DWITH_PRIMITIVES=ON
		-DWITH_TEXT=ON
		-DWITH_TEXTUREDTRIANGLE=ON
		-DWITH_TRIANGLE=ON
		-DWITH_VIEWER=ON
	)
	cmake-utils_src_configure
}

# kate: replace-tabs off;
