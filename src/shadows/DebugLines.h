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

#pragma once

#include <Magnum/Shaders/VertexColor.h>
#include <Magnum/Buffer.h>
#include <Magnum/Mesh.h>
#include <Magnum/SceneGraph/SceneGraph.h>

using namespace Magnum;

class DebugLines
{
public:
    typedef Shaders::VertexColor3D Shader;

    DebugLines();

    struct Point {
        Vector3 position;
        Color3 color;
    };

    void reset();

    void addLine(Point&& p0, Point&& p1) {
        lines.push_back(std::move(p0));
        lines.push_back(std::move(p1));
    }

    void addLine(Vector3 p0, Vector3 p1, Color3 col) {
        addLine({p0,col},{p1,col});
    }

    void addFrustum(const Matrix4& imvp, Color3 col);
    void addFrustum(const Matrix4 &imvp, const Color3 &col, float z0, float z1);

    void draw(const Magnum::Matrix4& transformationProjectionMatrix);



protected:

    std::vector<Point> lines;
    Magnum::Buffer buffer;
    Magnum::Mesh mesh;
    Shader shader;

};

