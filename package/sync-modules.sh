#!/bin/bash

set -e

dir=$(dirname "$0")
cp $dir/../../corrade/modules/FindCorrade.cmake $dir/../modules/
cp $dir/../../magnum/modules/FindMagnum.cmake $dir/../modules/
cp $dir/../../magnum-extras/modules/FindMagnumExtras.cmake $dir/../modules/
cp $dir/../../magnum-integration/modules/FindMagnumIntegration.cmake $dir/../modules/
