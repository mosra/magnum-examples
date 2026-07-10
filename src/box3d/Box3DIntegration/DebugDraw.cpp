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

#include "DebugDraw.h"

#include <Corrade/Containers/GrowableArray.h>
#include <Magnum/Math/Functions.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Quaternion.h>

namespace Magnum { namespace Box3DIntegration {

using namespace Math::Literals;

DebugDraw::DebugDraw(const std::size_t initialBufferCapacity)
    : _shader{NoCreate}, _buffer{NoCreate}, _mesh{NoCreate}
{
    create(initialBufferCapacity);
}

DebugDraw::DebugDraw(NoCreateT) noexcept
    : _shader{NoCreate}, _buffer{NoCreate}, _mesh{NoCreate} {}

DebugDraw::~DebugDraw() = default;

DebugDraw::Color DebugDraw::fromHex(const b3HexColor hex, const float alpha) {
    return {
        ((hex >> 16) & 0xff) / 255.0f,
        ((hex >>  8) & 0xff) / 255.0f,
        ( hex        & 0xff) / 255.0f,
        alpha
    };
}

Vector3 DebugDraw::fromPos(const b3Pos p) {
    /* b3Pos uses double for large-world support */
    return {Float(p.x), Float(p.y), Float(p.z)};
}

Matrix4 DebugDraw::fromWorldTransform(const b3WorldTransform& t) {
    /* b3WorldTransform = { b3Pos p; b3Quat q; } */
    const Quaternion rot{{Float(t.q.v.x), Float(t.q.v.y), Float(t.q.v.z)}, Float(t.q.s)};
    const Vector3 pos = fromPos(t.p);
    return Matrix4::from(rot.toMatrix(), pos);
}

b3DebugDraw DebugDraw::debugDraw() {
    b3DebugDraw d = b3DefaultDebugDraw();

    d.DrawShapeFcn     = drawShapeCallback;
    d.DrawSegmentFcn   = drawSegmentCallback;
    d.DrawTransformFcn = drawTransformCallback;
    d.DrawPointFcn     = drawPointCallback;
    d.DrawSphereFcn    = drawSphereCallback;
    d.DrawCapsuleFcn   = drawCapsuleCallback;
    d.DrawBoundsFcn    = drawBoundsCallback;
    d.DrawBoxFcn       = drawBoxCallback;
    d.DrawStringFcn    = drawStringCallback;

    d.context = this;

    d.drawShapes          = true;
    d.drawJoints          = true;
    d.drawJointExtras     = false;
    d.drawBounds          = true;
    d.drawMass            = false;
    d.drawBodyNames       = false;
    d.drawContacts        = false;
    d.drawContactNormals  = false;
    d.drawContactForces   = false;
    d.drawFrictionForces  = false;
    d.drawIslands         = true;
    d.drawGraphColors     = false;
    d.drawContactFeatures = true;
    d.forceScale          = 0.35f;
    d.jointScale          = 1.0f;

    return d;
}

void DebugDraw::drawAxes(const Matrix4& transformation, const float length) {
    const Vector3 o = transformation.translation();
    const Matrix3x3 rot = transformation.rotationScaling();

    const b3Vec3 origin = {o.x(), o.y(), o.z()};
    const b3Vec3 xAxis  = {o.x() + rot[0][0] * length,
                           o.y() + rot[0][1] * length,
                           o.z() + rot[0][2] * length};
    const b3Vec3 yAxis  = {o.x() + rot[1][0] * length,
                           o.y() + rot[1][1] * length,
                           o.z() + rot[1][2] * length};
    const b3Vec3 zAxis  = {o.x() + rot[2][0] * length,
                           o.y() + rot[2][1] * length,
                           o.z() + rot[2][2] * length};

    drawSegment(origin, xAxis, {1.0f, 0.0f, 0.0f, 1.0f}); // Red X
    drawSegment(origin, yAxis, {0.0f, 1.0f, 0.0f, 1.0f}); // Green Y
    drawSegment(origin, zAxis, {0.0f, 0.0f, 1.0f, 1.0f}); // Blue Z
}

void DebugDraw::drawWireframeBox(const Matrix4& transformation,
                                 const Vector3& halfExtents,
                                 const Color& color)
{
    const Vector3 min = -halfExtents;
    const Vector3 max =  halfExtents;

    const Vector3 corners[8] = {
        {min.x(), min.y(), min.z()},
        {max.x(), min.y(), min.z()},
        {max.x(), max.y(), min.z()},
        {min.x(), max.y(), min.z()},
        {min.x(), min.y(), max.z()},
        {max.x(), min.y(), max.z()},
        {max.x(), max.y(), max.z()},
        {min.x(), max.y(), max.z()},
    };

    b3Vec3 world[8];
    for(int i = 0; i < 8; ++i) {
        const Vector3 w = (transformation * Vector4{corners[i], 1.0f}).xyz();
        world[i] = {w.x(), w.y(), w.z()};
    }

    // Bottom face
    drawSegment(world[0], world[1], color);
    drawSegment(world[1], world[2], color);
    drawSegment(world[2], world[3], color);
    drawSegment(world[3], world[0], color);

    // Top face
    drawSegment(world[4], world[5], color);
    drawSegment(world[5], world[6], color);
    drawSegment(world[6], world[7], color);
    drawSegment(world[7], world[4], color);

    // Vertical edges
    drawSegment(world[0], world[4], color);
    drawSegment(world[1], world[5], color);
    drawSegment(world[2], world[6], color);
    drawSegment(world[3], world[7], color);
}

void DebugDraw::drawWireframeSphere(const Matrix4& transformation,
                                    const Float radius,
                                    const Color& color)
{
    /* Three great circles (XY, XZ, YZ) give a clear sphere outline. */
    constexpr Int segments = 24;
    constexpr Float step = Constants::pi()*2.0f/static_cast<Float>(segments);

    auto toWorld = [&](const Vector3& local) {
        const Vector3 w = (transformation*Vector4{local, 1.0f}).xyz();
        return b3Vec3{w.x(), w.y(), w.z()};
    };

    for(Int i = 0; i != segments; ++i) {
        const Rad a0{static_cast<Float>(i)*step};
        const Rad a1{static_cast<Float>(i + 1)*step};
        const Float c0 = Math::cos(a0), s0 = Math::sin(a0);
        const Float c1 = Math::cos(a1), s1 = Math::sin(a1);

        /* XY plane */
        drawSegment(toWorld(Vector3{radius*c0, radius*s0, 0.0f}),
                    toWorld(Vector3{radius*c1, radius*s1, 0.0f}), color);
        /* XZ plane */
        drawSegment(toWorld(Vector3{radius*c0, 0.0f, radius*s0}),
                    toWorld(Vector3{radius*c1, 0.0f, radius*s1}), color);
        /* YZ plane */
        drawSegment(toWorld(Vector3{0.0f, radius*c0, radius*s0}),
                    toWorld(Vector3{0.0f, radius*c1, radius*s1}), color);
    }
}

void DebugDraw::create(const std::size_t initialBufferCapacity) {
    _mesh = GL::Mesh{GL::MeshPrimitive::Lines};
    _buffer = GL::Buffer{};
    _shader = Shaders::VertexColorGL3D{};

    _mesh.addVertexBuffer(_buffer, 0,
                          Shaders::VertexColorGL3D::Position{},
                          Shaders::VertexColorGL3D::Color4{});

    arrayReserve(_bufferData, initialBufferCapacity);
}

void DebugDraw::flush() {
    if(_bufferData.isEmpty()) return;

    _buffer.setData(_bufferData, GL::BufferUsage::DynamicDraw);
    _mesh.setCount(_bufferData.size());

    _shader.setTransformationProjectionMatrix(_transformationProjectionMatrix)
           .draw(_mesh);

    arrayResize(_bufferData, 0);
}

void DebugDraw::drawSegment(const b3Vec3 p1, const b3Vec3 p2, const Color color) {
    const Vector4 c{color.r, color.g, color.b, color.a};

    arrayAppend(_bufferData, {
                    Vertex{{p1.x, p1.y, p1.z}, c},
                    Vertex{{p2.x, p2.y, p2.z}, c}
                });
}

void DebugDraw::drawTransform(const b3WorldTransform& xf) {
    constexpr float len = 0.5f;

    const Vector3 origin = fromPos(xf.p);

    /* Reconstruct rotation matrix from quaternion */
    const Quaternion q{{(xf.q.v.x), (xf.q.v.y), (xf.q.v.z)}, (xf.q.s)};
    const Matrix3x3 rot = q.toMatrix();

    const b3Vec3 o = {origin.x(), origin.y(), origin.z()};
    const b3Vec3 xAxis = {o.x + rot[0][0]*len,
                          o.y + rot[0][1]*len,
                          o.z + rot[0][2]*len};
    const b3Vec3 yAxis = {o.x + rot[1][0]*len,
                          o.y + rot[1][1]*len,
                          o.z + rot[1][2]*len};
    const b3Vec3 zAxis = {o.x + rot[2][0]*len,
                          o.y + rot[2][1]*len,
                          o.z + rot[2][2]*len};

    drawSegment(o, xAxis, {1.0f, 0.0f, 0.0f, 1.0f}); // Red X
    drawSegment(o, yAxis, {0.0f, 1.0f, 0.0f, 1.0f}); // Green Y
    drawSegment(o, zAxis, {0.0f, 0.0f, 1.0f, 1.0f}); // Blue Z
}

    void DebugDraw::drawPoint(const b3Vec3 p, const float size, const Color color) {
    const float s  = size * 0.55f;
    const float s2 = s * 0.65f;
    const float d  = s2 * 0.70710678f;   // 1/√2

    // main cross
    drawSegment({p.x-s, p.y, p.z}, {p.x+s, p.y, p.z}, color);
    drawSegment({p.x, p.y-s, p.z}, {p.x, p.y+s, p.z}, color);
    drawSegment({p.x, p.y, p.z-s}, {p.x, p.y, p.z+s}, color);

    // 45° secondary cross → star / diamond look
    drawSegment({p.x-d, p.y-d, p.z}, {p.x+d, p.y+d, p.z}, color);
    drawSegment({p.x-d, p.y+d, p.z}, {p.x+d, p.y-d, p.z}, color);
    drawSegment({p.x-d, p.y, p.z-d}, {p.x+d, p.y, p.z+d}, color);
    drawSegment({p.x-d, p.y, p.z+d}, {p.x+d, p.y, p.z-d}, color);
    drawSegment({p.x, p.y-d, p.z-d}, {p.x, p.y+d, p.z+d}, color);
    drawSegment({p.x, p.y-d, p.z+d}, {p.x, p.y+d, p.z-d}, color);
}

/* ==== Static Callbacks ==================================================== */

void DebugDraw::drawSegmentCallback(const b3Pos p1, const b3Pos p2, const b3HexColor color, void* context) {
    auto* dd = static_cast<DebugDraw*>(context);
    const b3Vec3 a{Float(p1.x), Float(p1.y), Float(p1.z)};
    const b3Vec3 b{Float(p2.x), Float(p2.y), Float(p2.z)};
    const Color c = fromHex(color);

    dd->drawSegment(a, b, c);
}

void DebugDraw::drawTransformCallback(b3WorldTransform transform, void* context) {
    auto* dd = static_cast<DebugDraw*>(context);
    dd->drawTransform(transform);
}

    void DebugDraw::drawPointCallback(const b3Pos p, const float size, const b3HexColor color, void* context) {
    auto* dd = static_cast<DebugDraw*>(context);

    Color c = fromHex(color);

    // brighten
    c.r = Math::min(1.0f, c.r * 1.25f + 0.15f);
    c.g = Math::min(1.0f, c.g * 1.25f + 0.15f);
    c.b = Math::min(1.0f, c.b * 1.25f + 0.15f);
    c.a = 1.0f;

    const float visualSize = Math::max(size * 1.8f, 0.12f);
    dd->drawPoint({Float(p.x), Float(p.y), Float(p.z)}, visualSize, c);
}

void DebugDraw::drawSphereCallback(const b3Pos p, const float radius, const b3HexColor color, const float alpha, void* context) {
    auto* dd = static_cast<DebugDraw*>(context);
    const Matrix4 t = Matrix4::translation(fromPos(p));
    dd->drawWireframeSphere(t, radius, fromHex(color, alpha));
}

void DebugDraw::drawCapsuleCallback(const b3Pos p1, const b3Pos p2, const float radius, const b3HexColor color, const float alpha, void* context) {
    auto* dd = static_cast<DebugDraw*>(context);
    const Color c = fromHex(color, alpha);
    const Vector3 a = fromPos(p1);
    const Vector3 b = fromPos(p2);

    /* Simple wireframe: two spheres + a few longitudinal lines */
    dd->drawWireframeSphere(Matrix4::translation(a), radius, c);
    dd->drawWireframeSphere(Matrix4::translation(b), radius, c);

    const Vector3 dir = (b - a).normalized();
    /* Three meridians */
    Vector3 u = Math::cross(dir, Vector3::yAxis());
    if(u.dot() < 1.0e-6f)
        u = Math::cross(dir, Vector3::xAxis());
    u = u.normalized()*radius;
    const Vector3 v = Math::cross(dir, u);

    for(int i = 0; i < 4; ++i) {
        const Float ang = static_cast<Float>(i)*Constants::pi()*0.5f;
        const Vector3 offset = u*Math::cos(Rad{ang}) + v*Math::sin(Rad{ang});
        dd->drawSegment(
            {a.x()+offset.x(), a.y()+offset.y(), a.z()+offset.z()},
            {b.x()+offset.x(), b.y()+offset.y(), b.z()+offset.z()},
            c);
    }
}

void DebugDraw::drawBoundsCallback(b3AABB aabb, b3HexColor color, void* context) {
    auto* dd = static_cast<DebugDraw*>(context);
    const Vector3 min{(aabb.lowerBound.x), (aabb.lowerBound.y), (aabb.lowerBound.z)};
    const Vector3 max{(aabb.upperBound.x), (aabb.upperBound.y), (aabb.upperBound.z)};
    const Vector3 center = (min + max)*0.5f;
    const Vector3 half   = (max - min)*0.5f;
    dd->drawWireframeBox(Matrix4::translation(center), half, fromHex(color));
}

void DebugDraw::drawBoxCallback(const b3Vec3 extents, b3WorldTransform transform, const b3HexColor color, void* context) {
    auto* dd = static_cast<DebugDraw*>(context);
    const Matrix4 t = fromWorldTransform(transform);
    const Vector3 half{(extents.x), (extents.y), (extents.z)};
    dd->drawWireframeBox(t, half, fromHex(color));
}

void DebugDraw::drawStringCallback(b3Pos, const char*, b3HexColor, void*) {
    /* Text rendering is left to the application */
}

bool DebugDraw::drawShapeCallback(void* /*userShape*/, b3WorldTransform transform,
                                  b3HexColor /*color*/, void* context) {
    /* When createDebugShape / destroyDebugShape are not used, Box3D still
       calls DrawShapeFcn for every shape.  We fall back to drawing a small
       transform (axes) so the user at least sees body locations.  For full
       solid/wire shape rendering, the application should supply the
       creation/destruction callbacks and store Magnum meshes as userShape. */
    auto* dd = static_cast<DebugDraw*>(context);
    dd->drawTransform(transform);
    /* Returning true continues drawing subsequent shapes */
    return true;
}

}}
