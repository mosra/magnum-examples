class MagnumExamples < Formula
  desc "Examples for the Magnum C++11/C++14 graphics engine"
  homepage "https://github.com/mosra/magnum"
  url "https://github.com/mosra/magnum-examples/archive/v2020.06.tar.gz"
  # wget https://github.com/mosra/magnum-examples/archive/v2020.06.tar.gz -O - | sha256sum
  sha256 "d93c4fa034667f92d83459c06254e70619865f8568e46d9d72bb06aac29b7153"
  head "git://github.com/mosra/magnum-examples.git"

  depends_on "cmake"
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
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_INSTALL_PREFIX=#{prefix}",
        "-DWITH_ANIMATED_GIF_EXAMPLE=ON",
        "-DWITH_ARCBALL_EXAMPLE=ON",
        "-DWITH_AREALIGHTS_EXAMPLE=ON",
        "-DWITH_AUDIO_EXAMPLE=ON",
        "-DWITH_BOX2D_EXAMPLE=OFF",
        "-DWITH_BULLET_EXAMPLE=ON",
        "-DWITH_CUBEMAP_EXAMPLE=ON",
        "-DWITH_DART_EXAMPLE=#{(build.with? 'dartsim') ? 'ON' : 'OFF'}",
        "-DWITH_FLUIDSIMULATION2D_EXAMPLE=OFF",
        "-DWITH_FLUIDSIMULATION3D_EXAMPLE=OFF",
        "-DWITH_IMGUI_EXAMPLE=ON",
        "-DWITH_MOTIONBLUR_EXAMPLE=ON",
        "-DWITH_MOUSEINTERACTION_EXAMPLE=ON",
        "-DWITH_PICKING_EXAMPLE=ON",
        "-DWITH_PRIMITIVES_EXAMPLE=ON",
        "-DWITH_RAYTRACING_EXAMPLE=ON",
        "-DWITH_SHADOWS_EXAMPLE=ON",
        "-DWITH_TEXT_EXAMPLE=ON",
        "-DWITH_TEXTUREDTRIANGLE_EXAMPLE=ON",
        "-DWITH_TRIANGLE_EXAMPLE=ON",
        "-DWITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=OFF",
        "-DWITH_TRIANGLE_SOKOL_EXAMPLE=OFF",
        "-DWITH_VIEWER_EXAMPLE=ON",
        ".."
      system "cmake", "--build", "."
      system "cmake", "--build", ".", "--target", "install"
    end
  end
end
