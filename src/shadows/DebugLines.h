/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016 —
            Vladimír Vondruš <mosra@centrum.cz>
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

