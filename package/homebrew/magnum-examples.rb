class MagnumExamples < Formula
  desc "Examples for the Magnum C++11 graphics engine"
  homepage "https://magnum.graphics"
  # git describe <hash>, except the `v` prefix
  version "2020.06-305-gaa850fe0"
  # There's no version.h for examples, can get just an archive. Can't really
  # automate the SHA1 retrieval in order to be able to calculate the checlsum.
  url "https://github.com/mosra/magnum-examples/archive/aa850fe0.tar.gz"
  # wget https://github.com/mosra/magnum-examples/archive/aa850fe0.tar.gz -O - | sha256sum
  sha256 "aff595b25a40f520648a229b72119c66f6af9b5382ac78280da47d562654d9f5"
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
        "-DMAGNUM_WITH_ANIMATED_GIF_EXAMPLE=ON",
        "-DMAGNUM_WITH_ARCBALL_EXAMPLE=ON",
        "-DMAGNUM_WITH_AREALIGHTS_EXAMPLE=ON",
        "-DMAGNUM_WITH_AUDIO_EXAMPLE=ON",
        "-DMAGNUM_WITH_BOX2D_EXAMPLE=OFF",
        "-DMAGNUM_WITH_BULLET_EXAMPLE=ON",
        "-DMAGNUM_WITH_CUBEMAP_EXAMPLE=ON",
        "-DMAGNUM_WITH_DART_EXAMPLE=#{(build.with? 'dartsim') ? 'ON' : 'OFF'}",
        "-DMAGNUM_WITH_FLUIDSIMULATION2D_EXAMPLE=OFF",
        "-DMAGNUM_WITH_FLUIDSIMULATION3D_EXAMPLE=OFF",
        "-DMAGNUM_WITH_IMGUI_EXAMPLE=ON",
        "-DMAGNUM_WITH_MOTIONBLUR_EXAMPLE=ON",
        "-DMAGNUM_WITH_MOUSEINTERACTION_EXAMPLE=ON",
        "-DMAGNUM_WITH_PICKING_EXAMPLE=ON",
        "-DMAGNUM_WITH_PRIMITIVES_EXAMPLE=ON",
        "-DMAGNUM_WITH_RAYTRACING_EXAMPLE=ON",
        "-DMAGNUM_WITH_SHADOWS_EXAMPLE=ON",
        "-DMAGNUM_WITH_TEXT_EXAMPLE=ON",
        "-DMAGNUM_WITH_TEXTUREDQUAD_EXAMPLE=ON",
        "-DMAGNUM_WITH_TRIANGLE_EXAMPLE=ON",
        "-DMAGNUM_WITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=OFF",
        "-DMAGNUM_WITH_TRIANGLE_SOKOL_EXAMPLE=OFF",
        "-DMAGNUM_WITH_VIEWER_EXAMPLE=ON",
        ".."
      system "cmake", "--build", "."
      system "cmake", "--build", ".", "--target", "install"
    end
  end
end
