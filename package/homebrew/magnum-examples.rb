class MagnumExamples < Formula
  desc "Examples for the Magnum C++11/C++14 graphics engine"
  homepage "https://github.com/mosra/magnum"
  url "https://github.com/mosra/magnum-examples/archive/v2020.06.tar.gz"
  # wget https://github.com/mosra/magnum-examples/archive/v2020.06.tar.gz -O - | sha256sum
  sha256 "d93c4fa034667f92d83459c06254e70619865f8568e46d9d72bb06aac29b7153"
  head "https://github.com/mosra/magnum-examples.git"

  depends_on "cmake" => :build
  depends_on "magnum"
  depends_on "magnum-plugins"
  depends_on "magnum-extras"

  depends_on "dartsim" => :optional
  if build.with? "dartsim"
      depends_on "magnum-integration" => "with-dartsim"
  else
      depends_on "magnum-integration"
  end

  def install
    # 2020.06 has the options unprefixed, current master has them prefixed.
    # Options not present in 2020.06 are prefixed always.
    option_prefix = build.head? ? 'MAGNUM_' : ''

    system "mkdir build"
    cd "build" do
      # Box2D is not enabled because of
      # https://github.com/Homebrew/homebrew-core/pull/4482 and nothing
      # happened with https://github.com/erincatto/Box2D/issues/431 yet
      system "cmake",
      *std_cmake_args,
        # Without this, ARM builds will try to look for dependencies in
        # /usr/local/lib and /usr/lib (which are the default locations) instead
        # of /opt/homebrew/lib which is dedicated for ARM binaries. Please
        # complain to Homebrew about this insane non-obvious filesystem layout.
        "-DCMAKE_INSTALL_NAME_DIR:STRING=#{lib}",
        "-D#{option_prefix}WITH_ANIMATED_GIF_EXAMPLE=ON",
        "-D#{option_prefix}WITH_ARCBALL_EXAMPLE=ON",
        "-D#{option_prefix}WITH_AREALIGHTS_EXAMPLE=ON",
        "-D#{option_prefix}WITH_AUDIO_EXAMPLE=ON",
        "-D#{option_prefix}WITH_BOX2D_EXAMPLE=OFF",
        "-D#{option_prefix}WITH_BULLET_EXAMPLE=ON",
        "-D#{option_prefix}WITH_CUBEMAP_EXAMPLE=ON",
        "-D#{option_prefix}WITH_DART_EXAMPLE=#{(build.with? 'dartsim') ? 'ON' : 'OFF'}",
        "-D#{option_prefix}WITH_FLUIDSIMULATION2D_EXAMPLE=OFF",
        "-D#{option_prefix}WITH_FLUIDSIMULATION3D_EXAMPLE=OFF",
        "-D#{option_prefix}WITH_IMGUI_EXAMPLE=ON",
        "-D#{option_prefix}WITH_MOTIONBLUR_EXAMPLE=ON",
        "-D#{option_prefix}WITH_MOUSEINTERACTION_EXAMPLE=ON",
        "-D#{option_prefix}WITH_PICKING_EXAMPLE=ON",
        "-D#{option_prefix}WITH_PRIMITIVES_EXAMPLE=ON",
        "-D#{option_prefix}WITH_RAYTRACING_EXAMPLE=ON",
        "-D#{option_prefix}WITH_SHADOWS_EXAMPLE=ON",
        "-D#{option_prefix}WITH_TEXT_EXAMPLE=ON",
        "-D#{build.head? ? 'MAGNUM_WITH_TEXTUREDQUAD' : 'WITH_TEXTURED_TRIANGLE'}_EXAMPLE=ON",
        "-D#{option_prefix}WITH_TRIANGLE_EXAMPLE=ON",
        "-D#{option_prefix}WITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=OFF",
        "-D#{option_prefix}WITH_TRIANGLE_SOKOL_EXAMPLE=OFF",
        "-D#{option_prefix}WITH_VIEWER_EXAMPLE=ON",
        ".."
      system "cmake", "--build", "."
      system "cmake", "--build", ".", "--target", "install"
    end
  end
end
