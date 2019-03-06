class MagnumExamples < Formula
  desc "Examples for the Magnum C++11/C++14 graphics engine"
  homepage "https://github.com/mosra/magnum"
  url "https://github.com/mosra/magnum-examples/archive/v2019.01.tar.gz"
  # wget https://github.com/mosra/magnum-examples/archive/v2019.01.tar.gz -O - | sha256sum
  sha256 "260f63b88f703c8bdf458a76b1b1b5da1bc3e4182ac6c52308b958d16f2b9522"
  head "git://github.com/mosra/magnum-examples.git"

  depends_on "cmake"
  depends_on "magnum"
  depends_on "magnum-plugins"
  depends_on "magnum-integration"
  depends_on "magnum-extras"

  def install
    system "mkdir build"
    cd "build" do
      # Box2D is not enabled because of
      # https://github.com/Homebrew/homebrew-core/pull/4482 and nothing
      # happened with https://github.com/erincatto/Box2D/issues/431 yet
      system "cmake", "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_INSTALL_PREFIX=#{prefix}", "-DWITH_AREALIGHTS_EXAMPLE=ON", "-DWITH_AUDIO_EXAMPLE=ON", "-DWITH_BOX2D_EXAMPLE=OFF", "-DWITH_BULLET_EXAMPLE=ON", "-DWITH_CUBEMAP_EXAMPLE=ON", "-DWITH_IMGUI_EXAMPLE=OFF", "-DWITH_MOTIONBLUR_EXAMPLE=ON", "-DWITH_MOUSEINTERACTION_EXAMPLE=ON", "-DWITH_PICKING_EXAMPLE=ON", "-DWITH_PRIMITIVES_EXAMPLE=ON", "-DWITH_SHADOWS_EXAMPLE=ON", "-DWITH_TEXT_EXAMPLE=ON", "-DWITH_TEXTUREDTRIANGLE_EXAMPLE=ON", "-DWITH_TRIANGLE_EXAMPLE=ON", "-DWITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=OFF", "-DWITH_TRIANGLE_SOKOL_EXAMPLE=OFF", "-DWITH_VIEWER_EXAMPLE=ON", ".."
      system "cmake", "--build", "."
      system "cmake", "--build", ".", "--target", "install"
    end
  end
end
