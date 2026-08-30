/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025, 2026
             — Vladimír Vondruš <mosra@centrum.cz>
        2026 — Igal Alkon <igal@alkontek.com>

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

#include <Corrade/Containers/GrowableArray.h>

#include <Magnum/Timeline.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Time.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/UVSphere.h>
#include <Magnum/Shaders/PhongGL.h>
#include <Magnum/Trade/MeshData.h>

#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Quaternion.h>
#include <Magnum/Math/Functions.h>

#include <box3d/box3d.h>
#include <thread>
#include <algorithm>

#include "Box3DIntegration/Converters.h"
#include "Box3DIntegration/DebugDraw.h"

namespace Magnum { namespace Examples {

using namespace Math::Literals;

static constexpr Int BodyGridSize = 14;
static constexpr Int BodyInitialBufferCapacity = BodyGridSize*BodyGridSize*BodyGridSize;

static constexpr Float GroundSizeXZ = 16.0f;
static constexpr Float GroundSizeY = 0.5f;
static constexpr Float BoxSizeXYZ = 0.5f;
static constexpr Float SphereRadius = 1.0f;
static constexpr Float BoxMass = 1.0f;

static constexpr Float BoxDensity = 2.75f;
static constexpr Float SphereDensity = 3.5f;
static constexpr Float ShapeFriction = 0.15f;
static constexpr Float ShapeRestitution = 0.05f;

static constexpr Float ShootBoxMass = 1.0f;
static constexpr Float ShootSphereMass = 5.0f;
static constexpr Float ShootSpeed = 80.0f;

static constexpr Float MinZoomIn = 3.0f;
static constexpr Float MaxZoomOut = 100.0f;

static constexpr Float CameraProjectionNear = 0.001f;
static constexpr Float CameraProjectionFar =  1000.0f;

static constexpr Float MaxBodySimDistance = 1000.0f;
static constexpr Float MaxSimulationDt = 1.0f/30.0f;

/* Per-instance data for instanced Phong draws (transform, normals, color) */
struct InstanceData {
    Matrix4 transformationMatrix;
    Matrix3x3 normalMatrix;
    Color3 color;
};

/* Lightweight body record: physics handle + render-only side data.
   Pose is always read from Box3D when needed (no cached transform). */
struct Body {
    b3BodyId id = b3_nullBodyId;
    Matrix4 primitiveTransformation{Math::IdentityInit};
    Color3 color{0xffffff_rgbf};
    bool isBox = true;
};

namespace {

b3BodyId createBody(const Float mass, const b3WorldId worldId,
                    const b3BoxHull* boxHull, const b3Sphere* sphereGeom,
                    const Vector3& position = {}) {
    const bool isStatic = (mass <= 0.0f);

    b3BodyDef bodyDef = b3DefaultBodyDef();
    bodyDef.type = isStatic ? b3_staticBody : b3_dynamicBody;
    bodyDef.position = b3Pos(position);
    bodyDef.rotation = b3Quat_identity;

    const b3BodyId bodyId = b3CreateBody(worldId, &bodyDef);
    CORRADE_INTERNAL_ASSERT(b3Body_IsValid(bodyId));

    b3ShapeDef shapeDef = b3DefaultShapeDef();
    shapeDef.baseMaterial.friction = ShapeFriction;
    shapeDef.baseMaterial.restitution = ShapeRestitution;

    if (!isStatic) {
        shapeDef.density = boxHull ? BoxDensity : SphereDensity;
    }

    if (boxHull) {
        b3CreateHullShape(bodyId, &shapeDef, &boxHull->base);
    } else if(sphereGeom) {
        b3CreateSphereShape(bodyId, &shapeDef, sphereGeom);
    }

    return bodyId;
}

void destroyBody(Body& body) {
    if (b3Body_IsValid(body.id)) {
        b3DestroyBody(body.id);
        body.id = b3_nullBodyId;
    }
}

/* Read pose straight from Box3D. Returns false if the body is invalid. */
bool transformationFromPhysics(const Body& body, Matrix4& outTransformation) {
    if (!b3Body_IsValid(body.id)) {
        return false;
    }

    const b3Pos posB = b3Body_GetPosition(body.id);
    const b3Quat rotB = b3Body_GetRotation(body.id);

    const Vector3 pos{posB};
    const Quaternion rot{rotB};
    outTransformation = Matrix4::from(rot.toMatrix(), pos);
    return true;
}

Body& spawnBody(Containers::Array<Body>& bodies, const Float mass,
                const b3WorldId worldId, const b3BoxHull* boxHull,
                const b3Sphere* sphereGeom, const Matrix4& primitiveTransformation,
                const Color3& color, const Vector3& position = {}) {
    Body& body = arrayAppend(bodies, InPlaceInit);
    body.id = createBody(mass, worldId, boxHull, sphereGeom, position);
    body.isBox = boxHull != nullptr;
    body.color = color;
    body.primitiveTransformation = primitiveTransformation;
    return body;
}

}

class Box3DExample: public Platform::Application {
public:
    virtual ~Box3DExample();
    explicit Box3DExample(const Arguments& arguments);

private:
    void drawEvent() override;
    void keyPressEvent(KeyEvent& event) override;
    void pointerPressEvent(PointerEvent& event) override;
    void pointerReleaseEvent(PointerEvent& event) override;
    void pointerMoveEvent(PointerMoveEvent& event) override;
    void scrollEvent(ScrollEvent& event) override;
    void viewportEvent(ViewportEvent& event) override;

    GL::Mesh _box{NoCreate}, _sphere{NoCreate};
    GL::Buffer _boxInstanceBuffer{NoCreate}, _sphereInstanceBuffer{NoCreate};
    Shaders::PhongGL _shader{NoCreate};
    Containers::Array<InstanceData> _boxInstanceData, _sphereInstanceData;

    /* Orbit Camera */
    Vector3 _cameraTarget{0.0f, 3.0f, 0.0f};
    Deg _cameraYaw = 45.0_degf;
    Deg _cameraPitch = -30.0_degf;
    Float _cameraDistance = 65.0f;
    Matrix4 _projectionMatrix;
    Matrix4 _cameraMatrix;
    Vector2 _projectionSize;

    bool _dragging = false;
    Vector2 _lastPointerPosition;

    Timeline _timeline;

    /* Box3D world identifier */
    b3WorldId _worldId = b3_nullWorldId;

    /* Reusable shape templates */
    b3BoxHull _boxHull{};
    b3Sphere  _sphereGeom{};
    b3BoxHull _groundHull{};

    Box3DIntegration::DebugDraw _debugDraw{NoCreate};

    Containers::Array<Body> _bodies;

    bool _drawCubes{true}, _drawDebug{false}, _shootBox{false};

    Matrix4 cameraAbsoluteTransformation() const {
        /* Classic orbit camera: translate to target -> yaw -> pitch -> push out along local +Z. */
        return Matrix4::translation(_cameraTarget)
             * Matrix4::rotationY(_cameraYaw)
             * Matrix4::rotationX(_cameraPitch)
             * Matrix4::translation(Vector3::zAxis(_cameraDistance));
    }

    void updateCameraMatrices() {
        const Matrix4 absolute = cameraAbsoluteTransformation();
        _cameraMatrix = absolute.inverted();
    }
};

Box3DExample::Box3DExample(const Arguments& arguments) :
    Platform::Application(arguments, NoCreate)
{
    {
        const Vector2 dpiScaling = this->dpiScaling({});
        Configuration conf;
        conf.setTitle("Magnum Box3D Example")
            .setSize(conf.size(), dpiScaling);
        GLConfiguration glConf;
        glConf.setSampleCount(dpiScaling.max() < 2.0f ? 8 : 2);
        if (!tryCreate(conf, glConf))
            create(conf, glConf.setSampleCount(0));
    }

    _debugDraw.create(BodyInitialBufferCapacity);

    /* Manual camera setup */
    const Vector2i viewport = GL::defaultFramebuffer.viewport().size();
    const Float aspect = Vector2{viewport}.aspectRatio();
    constexpr Rad fov = 35.0_degf;
    _projectionMatrix = Matrix4::perspectiveProjection(fov, aspect, CameraProjectionNear, CameraProjectionFar);
    const Float halfHeightAt1 = Math::tan(fov*0.5f);
    _projectionSize = {2.0f * aspect * halfHeightAt1, 2.0f * halfHeightAt1};
    updateCameraMatrices();

    /* Instanced Phong shader */
    _shader = Shaders::PhongGL{
        Shaders::PhongGL::Configuration{}
            .setFlags(Shaders::PhongGL::Flag::VertexColor |
                      Shaders::PhongGL::Flag::InstancedTransformation)
    };

    /* Global/world lighting */
    _shader.setAmbientColor(0x333333_rgbf)
           .setSpecularColor(0x222222_rgbf)
           .setLightColors({0xffffff_rgbf});

    /* Meshes and instance buffers */
    _box = MeshTools::compile(Primitives::cubeSolid());
    _sphere = MeshTools::compile(Primitives::uvSphereSolid(16, 32));

    _boxInstanceBuffer = GL::Buffer{};
    _sphereInstanceBuffer = GL::Buffer{};

    _box.addVertexBufferInstanced(_boxInstanceBuffer, 1, 0,
        Shaders::PhongGL::TransformationMatrix{},
        Shaders::PhongGL::NormalMatrix{},
        Shaders::PhongGL::Color3{});

    _sphere.addVertexBufferInstanced(_sphereInstanceBuffer, 1, 0,
        Shaders::PhongGL::TransformationMatrix{},
        Shaders::PhongGL::NormalMatrix{},
        Shaders::PhongGL::Color3{});

    /* Growable arrays: reserve once so shooting / filling instances stays cheap */
    arrayReserve(_bodies, BodyInitialBufferCapacity + 1);
    arrayReserve(_boxInstanceData, BodyInitialBufferCapacity + 1);
    arrayReserve(_sphereInstanceData, 64);

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::enable(GL::Renderer::Feature::PolygonOffsetFill);
    GL::Renderer::setPolygonOffset(2.0f, 0.5f);

    /* Box3D Initialization */
    b3WorldDef worldDef = b3DefaultWorldDef();
    worldDef.gravity = {0.0f, -9.81f, 0.0f};

    // Workers as the [number of cores] - 1
    const unsigned workers = std::max(1u, std::thread::hardware_concurrency()-1);
    worldDef.workerCount = workers;
    worldDef.enableSleep = true;
    _worldId = b3CreateWorld(&worldDef);
    CORRADE_INTERNAL_ASSERT(b3World_IsValid(_worldId));

    /* Precompute reusable shape geometries (half-extents / radius) */
    _boxHull    = b3MakeBoxHull(BoxSizeXYZ, BoxSizeXYZ, BoxSizeXYZ);
    _groundHull = b3MakeBoxHull(GroundSizeXZ, GroundSizeY, GroundSizeXZ);
    _sphereGeom = {{0.0f, 0.0f, 0.0f}, SphereRadius};

    /* Ground (static) */
    spawnBody(_bodies, 0.0f, _worldId, &_groundHull, nullptr,
              Matrix4::scaling({GroundSizeXZ, GroundSizeY, GroundSizeXZ}),
              0xffffff_rgbf);

    /* Stack of dynamic boxes */
    constexpr Int boxGridSize = BodyGridSize;
    constexpr Float boxGridOffset = (boxGridSize - 1)*BoxSizeXYZ;
    auto hue = 42.0_degf;
    for(Int i = 0; i != boxGridSize; ++i) {
        for(Int j = 0; j != boxGridSize; ++j) {
            for(Int k = 0; k != boxGridSize; ++k) {
                const Vector3 pos{
                    static_cast<Float>(i) - boxGridOffset,
                    static_cast<Float>(j) + 4.0f,
                    static_cast<Float>(k) - boxGridOffset
                };
                spawnBody(_bodies, BoxMass, _worldId, &_boxHull, nullptr,
                    Matrix4::scaling(Vector3{BoxSizeXYZ}),
                    Color3::fromHsv({hue += 137.5_degf, 0.75f, 0.9f}),
                    pos);
            }
        }
    }

    setSwapInterval(1);
    setMinimalLoopPeriod(16.0_msec);
    _timeline.start();
}

Box3DExample::~Box3DExample() {
    for(Body& body: _bodies) {
        destroyBody(body);
    }

    arrayClear(_bodies);

    if(b3World_IsValid(_worldId)) {
        b3DestroyWorld(_worldId);
        _worldId = b3_nullWorldId;
    }
}

void Box3DExample::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    const Vector2i size = event.framebufferSize();
    const Float aspect = Vector2{size}.aspectRatio();
    constexpr Rad fov = 35.0_degf;
    _projectionMatrix = Matrix4::perspectiveProjection(fov, aspect, CameraProjectionNear, CameraProjectionFar);

    /* Keep projectionSize in sync (plane at distance 1) */
    const Float halfHeightAt1 = Math::tan(fov*0.5f);
    _projectionSize = {2.0f * aspect * halfHeightAt1, 2.0f * halfHeightAt1};
}

void Box3DExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    /* Upper-bounds the physics step so a hitch won't feed Box3D a huge delta time. */
    const Float dt = Math::min(_timeline.previousFrameDuration(), MaxSimulationDt);
    b3World_Step(_worldId, dt, 4);

    /* Update view matrix */
    updateCameraMatrices();

    if(_drawCubes) {
        arrayClear(_boxInstanceData);
        arrayClear(_sphereInstanceData);
    }

    if(_drawDebug) {
        _debugDraw.setTransformationProjectionMatrix(
            _projectionMatrix*_cameraMatrix);
    }

    /*
     * Single pass over all bodies:
     * - Read pose from physics; remove invalid bodies
     * - Destroy and drop bodies that flew too far
     * - Build instanced box/sphere draw data when enabled
     * - Emit debug axes / wireframes when enabled
     */
    for(std::size_t i = 0; i != _bodies.size();) {
        Body& body = _bodies[i];

        Matrix4 transformation;
        if(!transformationFromPhysics(body, transformation)) {
            arrayRemoveUnordered(_bodies, i);
            continue;
        }

        const Vector3 pos = transformation.translation();
        if(pos.dot() > MaxBodySimDistance*MaxBodySimDistance) {
            destroyBody(body);
            arrayRemoveUnordered(_bodies, i);
            continue;
        }

        if(_drawCubes) {
            const Matrix4 t =
                _cameraMatrix*transformation*body.primitiveTransformation;
            arrayAppend(body.isBox ? _boxInstanceData : _sphereInstanceData,
                        InPlaceInit, t, t.normalMatrix(), body.color);
        }

        if(_drawDebug) {
            _debugDraw.drawAxes(transformation, 0.6f);

            const Vector3 scale =
                Math::abs(body.primitiveTransformation.scaling());
            if(body.isBox)
                _debugDraw.drawWireframeBox(transformation, scale);
            else
                _debugDraw.drawWireframeSphere(transformation, scale.x());
        }

        ++i;
    }

    if(_drawCubes) {
        _shader.setProjectionMatrix(_projectionMatrix);
        _shader.setLightPositions({
            _cameraMatrix*Vector4{10.0f, 15.0f, 5.0f, 0.0f}
        });

        _boxInstanceBuffer.setData(_boxInstanceData, GL::BufferUsage::DynamicDraw);
        _box.setInstanceCount(_boxInstanceData.size());
        _shader.draw(_box);

        _sphereInstanceBuffer.setData(_sphereInstanceData, GL::BufferUsage::DynamicDraw);
        _sphere.setInstanceCount(_sphereInstanceData.size());
        _shader.draw(_sphere);
    }

    if(_drawDebug) {
        b3DebugDraw draw = _debugDraw.debugDraw();
        draw.drawShapes = false;
        b3World_Draw(_worldId, &draw, UINT64_MAX);
        _debugDraw.flush();
    }

    swapBuffers();
    _timeline.nextFrame();
    redraw();
}

void Box3DExample::keyPressEvent(KeyEvent& event) {
    if (event.key() == Key::D) {
        if (_drawCubes && _drawDebug) {
            _drawDebug = false;
        } else if (_drawCubes && !_drawDebug) {
            _drawCubes = false;
            _drawDebug = true;
        } else if (!_drawCubes && _drawDebug) {
            _drawCubes = true;
        }
    } else if (event.key() == Key::S) {
        _shootBox ^= true;
    } else if (event.key() == Key::Esc) {
        exit();
    } else {
        return;
    }
    event.setAccepted();
}

void Box3DExample::pointerPressEvent(PointerEvent& event) {
    if (!event.isPrimary())
        return;

    /* Right mouse / finger starts orbit drag */
    if (event.pointer() & (Pointer::MouseRight | Pointer::Finger)) {
        _dragging = true;
        _lastPointerPosition = event.position();
        event.setAccepted();
        return;
    }

    /* Left mouse shoots (keeps left free for comfortable orbiting) */
    if (!(event.pointer() & Pointer::MouseLeft))
        return;

    const Vector2 position = event.position() * Vector2{framebufferSize()} / Vector2{windowSize()};
    const Vector2 clickPoint = Vector2::yScale(-1.0f) *
        (position / Vector2{framebufferSize()} - Vector2{0.5f}) *
        _projectionSize;
    const Matrix4 absolute = cameraAbsoluteTransformation();
    const Vector3 direction =
        (absolute.rotationScaling() *
         Vector3{clickPoint, -1.0f}).normalized();

    const bool shootBox = _shootBox;
    const Body& object = spawnBody(
        _bodies,
        shootBox ? ShootBoxMass : ShootSphereMass,
        _worldId,
        shootBox ? &_boxHull : nullptr,
        shootBox ? nullptr : &_sphereGeom,
        Matrix4::scaling(Vector3{shootBox ? BoxSizeXYZ : SphereRadius}),
        shootBox ? 0x880000_rgbf : 0xff4444_rgbf,
        absolute.translation());

    b3Body_SetLinearVelocity(object.id, b3Vec3(direction*ShootSpeed));

    event.setAccepted();
}

void Box3DExample::pointerReleaseEvent(PointerEvent& event) {
    if (event.pointer() & (Pointer::MouseRight | Pointer::Finger)) {
        _dragging = false;
        event.setAccepted();
    }
}

void Box3DExample::pointerMoveEvent(PointerMoveEvent& event) {
    if (!_dragging)
        return;

    const Vector2 delta = event.position() - _lastPointerPosition;
    _lastPointerPosition = event.position();

    /* Sensitivity tuned for comfortable orbit */
    constexpr Float sensitivity = 0.3f;
    _cameraYaw   -= Deg{delta.x() * sensitivity};
    _cameraPitch -= Deg{delta.y() * sensitivity};

    /* Clamp pitch so we don't flip upside-down (min, max) */
    _cameraPitch = Math::clamp(_cameraPitch, Deg{-89.0f}, Deg{89.0f});

    event.setAccepted();
}

void Box3DExample::scrollEvent(ScrollEvent& event) {
    /* Zoom in/out */
    _cameraDistance *= (event.offset().y() > 0 ? 0.9f : 1.1f);
    _cameraDistance = Math::clamp(_cameraDistance, MinZoomIn, MaxZoomOut);
    event.setAccepted();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::Box3DExample)
