# kate: indent-width 2;

class MagnumExamples < Formula
  desc "Examples for Magnum graphics engine"
  homepage "https://github.com/mosra/magnum"
  head "git://github.com/mosra/magnum-examples.git"

  depends_on "cmake"
  depends_on "magnum"
  depends_on "magnum-plugins"
  depends_on "magnum-integration"

  def install
    system "mkdir build"
    cd "build" do
      system "cmake", "-DCMAKE_BUILD_TYPE=Release", "-DCMAKE_INSTALL_PREFIX=#{prefix}", "-DWITH_AUDIO_EXAMPLE=ON", "-DWITH_BULLET_EXAMPLE=ON", "-DWITH_CUBEMAP_EXAMPLE=ON", "-DWITH_MOTIONBLUR_EXAMPLE=ON", "-DWITH_PICKING_EXAMPLE=ON", "-DWITH_PRIMITIVES_EXAMPLE=ON", "-DWITH_SHADOWS_EXAMPLE=ON", "-DWITH_TEXT_EXAMPLE=ON", "-DWITH_TEXTUREDTRIANGLE_EXAMPLE=ON", "-DWITH_TRIANGLE_EXAMPLE=ON", "-DWITH_TRIANGLE_PLAIN_GLFW_EXAMPLE=OFF", "-DWITH_VIEWER_EXAMPLE=ON", ".."
      system "cmake", "--build", "."
      system "cmake", "--build", ".", "--target", "install"
    end
  end
end
