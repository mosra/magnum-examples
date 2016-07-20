/* magnum-shadows - A Cascading/Parallel-Split Shadow Mapping example
 * Written in 2016 by Bill Robinson airbaggins@gmail.com
 *
 * To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to
 * this software to the public domain worldwide. This software is distributed without any warranty.
 *
 * See <http://creativecommons.org/publicdomain/zero/1.0/>.
 *
 * Credit is appreciated, but not required.
 * */

#include "DebugLines.h"
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/Renderer.h>
#include "ShadowLight.h"


DebugLines::DebugLines()
:   mesh(Magnum::MeshPrimitive::Lines)
{
    buffer.setLabel("debug lines buffer");
    mesh.addVertexBuffer(buffer, 0, Shader::Position(), Shader::Color());
}

void DebugLines::reset()
{
    lines.clear();
    buffer.invalidateData();
}

void DebugLines::draw(const Magnum::Matrix4& transformationProjectionMatrix)
{
    if (!lines.empty()) {
        Magnum::Renderer::disable(Magnum::Renderer::Feature::DepthTest);
        buffer.setData(lines, Magnum::BufferUsage::StreamDraw);
        mesh.setCount(lines.size());
        shader.setTransformationProjectionMatrix(transformationProjectionMatrix);
        mesh.draw(shader);
        Magnum::Renderer::enable(Magnum::Renderer::Feature::DepthTest);
    }
}

void DebugLines::addFrustum(const Matrix4 &imvp, Color3 col) {
    addFrustum(imvp, col, 1.0f, -1.0f);
}

void DebugLines::addFrustum(const Matrix4 &imvp, const Color3 &col, float z0, float z1) {
    auto worldPointsToCover = ShadowLight::getFrustumCorners(imvp, z0, z1);

    auto nearMid = (worldPointsToCover[0] +
                    worldPointsToCover[1] +
                    worldPointsToCover[3] +
                    worldPointsToCover[2]) * 0.25f;

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