#ifndef Magnum_Box3DIntegration_DebugDraw_h
#define Magnum_Box3DIntegration_DebugDraw_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
                2020, 2021, 2022, 2023, 2024, 2025, 2026
              Vladimír Vondruš <mosra@centrum.cz>
    Copyright © 2026 Igal Alkon <igal@alkontek.com>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include <Corrade/Containers/Array.h>

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Shaders/VertexColorGL.h>

#include <box3d/box3d.h>

namespace Magnum { namespace Box3DIntegration {
/**
 * @brief Utility class for rendering debug geometry, lines, shapes and overlays.
 */
class DebugDraw {
public:
    struct Color {
        float r, g, b, a;
    };

    explicit DebugDraw(std::size_t initialBufferCapacity = 2048);
    explicit DebugDraw(NoCreateT) noexcept;

    DebugDraw(const DebugDraw&) = delete;
    DebugDraw& operator=(const DebugDraw&) = delete;
    ~DebugDraw();

    /** @brief Set the combined projection * view matrix used for drawing */
    DebugDraw& setTransformationProjectionMatrix(const Matrix4& matrix) {
        _transformationProjectionMatrix = matrix;
        return *this;
    }

    /**
     * @brief Builds and returns a b3DebugDraw instance configured with this
     * object's drawing callbacks and default debug visualization flags.
     *
     * @return Configured b3DebugDraw ready for use with the physics world draw API.
     */
    b3DebugDraw debugDraw();

    /**
     * @brief Draws RGB coordinate axes at the given transformation.
     * @param transformation World transform defining the origin and orientation of the axes.
     * @param length Length of each axis segment.
     */
    void drawAxes(const Matrix4& transformation, float length = 0.5f);

    /**
     * @brief Draws a wireframe box with the specified transform, size, and color.
     * @param transformation World transform defining position and orientation of the box.
     * @param halfExtents Half-extents of the box along each axis from its center.
     * @param color Color used for the wireframe edges.
     */
    void drawWireframeBox(const Matrix4& transformation,
                          const Vector3& halfExtents,
                          const Color& color = {0.7f, 0.7f, 0.7f, 1.0f});

    /**
     * @brief Draws a wireframe sphere using three great-circle outlines in the XY, XZ, and YZ planes.
     * @param transformation World transform defining the position and orientation of the sphere.
     * @param radius Radius of the sphere.
     * @param color Color used for the wireframe edges.
     */
    void drawWireframeSphere(const Matrix4& transformation,
                             Float radius,
                             const Color& color = {0.7f, 0.7f, 0.7f, 1.0f});

    void create(std::size_t initialBufferCapacity = 2048);
    void flush();

private:
    struct Vertex {
        Vector3 position;
        Vector4 color;
    };

    /* Static callbacks matching b3DebugDraw function pointers */
    static void drawSegmentCallback(b3Pos p1, b3Pos p2, b3HexColor color, void* context);
    static void drawTransformCallback(b3WorldTransform transform, void* context);
    static void drawPointCallback(b3Pos p, float size, b3HexColor color, void* context);
    static void drawSphereCallback(b3Pos p, float radius, b3HexColor color, float alpha, void* context);
    static void drawCapsuleCallback(b3Pos p1, b3Pos p2, float radius, b3HexColor color, float alpha, void* context);
    static void drawBoundsCallback(b3AABB aabb, b3HexColor color, void* context);
    static void drawBoxCallback(b3Vec3 extents, b3WorldTransform transform, b3HexColor color, void* context);
    static void drawStringCallback(b3Pos p, const char* s, b3HexColor color, void* context);
    static bool drawShapeCallback(void* userShape, b3WorldTransform transform, b3HexColor color, void* context);

    void drawSegment(b3Vec3 p1, b3Vec3 p2, Color color);
    void drawTransform(const b3WorldTransform& xf);
    void drawPoint(b3Vec3 p, float size, Color color);

    static Color fromHex(b3HexColor hex, float alpha = 1.0f);
    static Vector3 fromPos(b3Pos p);
    static Matrix4 fromWorldTransform(const b3WorldTransform& t);

    Matrix4 _transformationProjectionMatrix;
    Shaders::VertexColorGL3D _shader;
    GL::Buffer _buffer;
    GL::Mesh _mesh;
    Containers::Array<Vertex> _bufferData;
};

}}

#endif
