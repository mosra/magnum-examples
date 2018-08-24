# kate: indent-width 2;

class MagnumExamples < Formula
  desc "Examples for the Magnum C++11/C++14 graphics engine"
  homepage "https://github.com/mosra/magnum"
  url "https://github.com/mosra/magnum-examples/archive/v2018.02.tar.gz"
  sha256 "b3666c9725d257ab802e2246c15aecad4b3a95eeed74c9a1abea66f9b2aa99ef"
  head "git://github.com/mosra/magnum-examples.git"

  depends_on "cmake"
  depends_on "magnum"
  depends_on "magnum-plugins"
  depends_on "magnum-integration"
  depends_on "magnum-extras"
  depends_on "box2d"

  def install
    system "mkdir build"
    cd "build" do
      system "cmake", "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_INSTALL_PREFIX=#{prefix}", "-DWITH_AREALIGHTS_EXAMPLE=ON", "-DWITH_AUDIO_EXAMPLE=ON", "-DWITH_BOX2D_EXAMPLE=ON", "-DWITH_BULLET_EXAMPLE=ON", "-DWITH_CUBEMAP_EXAMPLE=ON", "-DWITH_MOTIONBLUR_EXAMPLE=ON", "-DWITH_PICKING_EXAMPLE=ON", "-DWITH_PRIMITIVES_EXAMPLE=ON", "-DWITH_SHADOWS_EXAMPLE=ON", "-DWITH_TEXT_EXAMPLE=ON", "-DWITH_TEXTUREDTRIANGLE_EXAMPLE=ON", "-DWITH_TRIANGLE_EXAMPLE=ON", "-DWITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=OFF", "-DWITH_TRIANGLE_SOKOL_EXAMPLE=OFF", "-DWITH_VIEWER_EXAMPLE=ON", ".."
      system "cmake", "--build", "."
      system "cmake", "--build", ".", "--target", "install"
    end
  end
end
