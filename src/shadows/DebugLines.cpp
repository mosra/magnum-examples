/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022 — Vladimír Vondruš <mosra@centrum.cz>
        2016 — Bill Robinson <airbaggins@gmail.com>

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

#include "DebugLines.h"

#include <Corrade/Containers/ArrayViewStl.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/SceneGraph/Camera.h>

#include "ShadowLight.h"

namespace Magnum { namespace Examples {

DebugLines::DebugLines(): _mesh{GL::MeshPrimitive::Lines} {
    _mesh.addVertexBuffer(_buffer, 0,
        Shaders::VertexColorGL3D::Position{},
        Shaders::VertexColorGL3D::Color3{});
}

void DebugLines::reset() {
    _lines.clear();
    _buffer.invalidateData();
}

void DebugLines::draw(const Matrix4& transformationProjectionMatrix) {
    if(!_lines.empty()) {
        GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
        _buffer.setData(_lines, GL::BufferUsage::StreamDraw);
        _mesh.setCount(_lines.size());
        _shader
            .setTransformationProjectionMatrix(transformationProjectionMatrix)
            .draw(_mesh);
        GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    }
}

void DebugLines::addFrustum(const Matrix4& imvp, const Color3& col) {
    addFrustum(imvp, col, 1.0f, -1.0f);
}

void DebugLines::addFrustum(const Matrix4& imvp, const Color3& col, const Float z0, const Float z1) {
    auto worldPointsToCover = ShadowLight::frustumCorners(imvp, z0, z1);

    auto nearMid = (worldPointsToCover[0] +
                    worldPointsToCover[1] +
                    worldPointsToCover[3] +
                    worldPointsToCover[2])*0.25f;

    addLine(nearMid, worldPointsToCover[1], col);
    addLine(nearMid, worldPointsToCover[3], col);
    addLine(nearMid, worldPointsToCover[2], col);
    addLine(nearMid, worldPointsToCover[0], col);

    addLine(worldPointsToCover[0], worldPointsToCover[1], col);
    addLine(worldPointsToCover[1], worldPointsToCover[3], col);
    addLine(worldPointsToCover[3], worldPointsToCover[2], col);
    addLine(worldPointsToCover[2], worldPointsToCover[0], col);

    addLine(worldPointsToCover[0], worldPointsToCover[4], col);
    addLine(worldPointsToCover[1], worldPointsToCover[5], col);
    addLine(worldPointsToCover[2], worldPointsToCover[6], col);
    addLine(worldPointsToCover[3], worldPointsToCover[7], col);

    addLine(worldPointsToCover[4], worldPointsToCover[5], col);
    addLine(worldPointsToCover[5], worldPointsToCover[7], col);
    addLine(worldPointsToCover[7], worldPointsToCover[6], col);
    addLine(worldPointsToCover[6], worldPointsToCover[4], col);
}

}}
