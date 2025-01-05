/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>
        2019 — Nghia Truong <nghiatruong.vn@gmail.com>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "DrawableObjects/ParticleGroup2D.h"

#include <Corrade/Utility/Assert.h>
#include <Corrade/Containers/ArrayView.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Shaders/GenericGL.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/Trade/MeshData.h>

namespace Magnum { namespace Examples {

using namespace Math::Literals;

ParticleGroup2D::ParticleGroup2D(const std::vector<Vector2>& points, Float particleRadius):
    /* With {}, GCC 4.8 warns that "a temporary bound to '_points' only
       persists until the constructor exits" (?!) */
    _points(points),
    _particleRadius{particleRadius},
    _meshParticles{GL::MeshPrimitive::Points}
{
    _meshParticles.addVertexBuffer(_bufferParticles, 0, Shaders::GenericGL2D::Position{});
    _particleShader.reset(new ParticleSphereShader2D);
}

ParticleGroup2D& ParticleGroup2D::draw(Containers::Pointer<SceneGraph::Camera2D>& camera, Int screenHeight, Int projectionHeight) {
    if(_points.empty()) return *this;

    if(_dirty) {
        Containers::ArrayView<const float> data(reinterpret_cast<const float*>(&_points[0]), _points.size() * 2);
        _bufferParticles.setData(data);
        _meshParticles.setCount(Int(_points.size()));
        _dirty = false;
    }

    (*_particleShader)
        /* particle data */
        .setNumParticles(Int(_points.size()))
        .setParticleRadius(_particleRadius)
        /* sphere render data */
        .setColorMode(_colorMode)
        .setColor(_color)
        /* view/prj matrices and size */
        .setViewProjectionMatrix(camera->projectionMatrix()*camera->cameraMatrix())
        .setScreenHeight(screenHeight)
        .setDomainHeight(projectionHeight)
        .draw(_meshParticles);

    return *this;
}
} }
