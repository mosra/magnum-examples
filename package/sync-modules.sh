#!/bin/bash

set -e

dir=$(dirname "$0")
cp $dir/../../corrade/modules/FindCorrade.cmake $dir/../modules/
cp $dir/../../magnum/modules/FindMagnum.cmake $dir/../modules/
# Used by the triangle-plain-glfw example (not through GlfwApplication)
cp $dir/../../magnum/modules/FindGLFW.cmake $dir/../modules/
cp $dir/../../magnum-extras/modules/FindMagnumExtras.cmake $dir/../modules/
cp $dir/../../magnum-integration/modules/FindMagnumIntegration.cmake $dir/../modules/
