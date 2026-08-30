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
#include <Corrade/Containers/Pointer.h>

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

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Quaternion.h>

#include <Magnum/JoltIntegration/Integration.h>
#include <Magnum/JoltIntegration/DebugDraw.h>

namespace Magnum { namespace Examples {

using namespace Math::Literals;

namespace {

struct InstanceData {
    Matrix4 transformationMatrix;
    Matrix3x3 normalMatrix;
    Color3 color;
};

enum class ShapeType { Box, Sphere };

struct BodyInstance {
    JPH::BodyID id;
    ShapeType shape;
    Color3 color;
    Vector3 scale;
};

struct AllCollideBroadPhaseLayerInterface: JPH::BroadPhaseLayerInterface {
    JPH::uint GetNumBroadPhaseLayers() const override { return 1; }
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer) const override {
        return JPH::BroadPhaseLayer{0};
    }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer) const override {
        return "All";
    }
#endif
};

struct AllCollideObjectVsBroadPhaseLayerFilter: JPH::ObjectVsBroadPhaseLayerFilter {
    bool ShouldCollide(JPH::ObjectLayer, JPH::BroadPhaseLayer) const override {
        return true;
    }
};

struct AllCollideObjectLayerPairFilter: JPH::ObjectLayerPairFilter {
    bool ShouldCollide(JPH::ObjectLayer, JPH::ObjectLayer) const override {
        return true;
    }
};

}

namespace {
    class JoltExample: public Platform::Application {
    public:
        explicit JoltExample(const Arguments& arguments);

        virtual ~JoltExample();

    private:
        void drawEvent() override;
        void viewportEvent(ViewportEvent& event) override;
        void keyPressEvent(KeyEvent& event) override;
        void pointerPressEvent(PointerEvent& event) override;
        void pointerReleaseEvent(PointerEvent& event) override;
        void pointerMoveEvent(PointerMoveEvent& event) override;
        void scrollEvent(ScrollEvent& event) override;

        /* size = full extents (box edge lengths / sphere diameter) */
        void addBody(Float mass, ShapeType shape, const Vector3& position,
                     const Vector3& size, const Color3& color,
                     const Vector3& velocity = {});

        Matrix4 cameraObjectMatrix() const;
        void updateCameraMatrices();

        GL::Mesh _box{NoCreate}, _sphere{NoCreate};
        GL::Buffer _boxInstanceBuffer{NoCreate}, _sphereInstanceBuffer{NoCreate};
        Shaders::PhongGL _shader{NoCreate};
        Containers::Array<InstanceData> _boxInstanceData, _sphereInstanceData;
        Containers::Array<BodyInstance> _bodies;

        Matrix4 _projectionMatrix, _viewMatrix;
        Vector3 _cameraTarget{0.0f, 3.0f, 0.0f};
        Rad _cameraYaw{40.0_degf};
        Rad _cameraPitch{-25.0_degf};
        Float _cameraDistance = 75.0f;
        Vector2 _previousPointerPosition;
        bool _orbiting = false;

        Timeline _timeline;

        JPH::JobSystemThreadPool _jobSystem{};
        Containers::Pointer<JPH::TempAllocatorImpl> _tmpAllocator;
        Containers::Pointer<JPH::PhysicsSystem> _physicsSystem;

        AllCollideBroadPhaseLayerInterface _broadPhaseLayerInterface;
        AllCollideObjectVsBroadPhaseLayerFilter _objectVsBroadPhaseLayerFilter;
        AllCollideObjectLayerPairFilter _objectLayerPairFilter;

        Containers::Pointer<JoltIntegration::DebugDraw> _debugDraw;

        bool _drawCubes{true}, _drawDebug{true}, _shootBox{false};
    };
}

Matrix4 JoltExample::cameraObjectMatrix() const {
    /* Orbit camera: T(target) * Ry * Rx * T(distance on +Z), looks along -Z */
    return Matrix4::translation(_cameraTarget)*
           Matrix4::rotationY(_cameraYaw)*
           Matrix4::rotationX(_cameraPitch)*
           Matrix4::translation(Vector3::zAxis(_cameraDistance));
}

void JoltExample::updateCameraMatrices() {
    _viewMatrix = cameraObjectMatrix().invertedRigid();
}

void JoltExample::addBody(const Float mass, const ShapeType shape,
    const Vector3& position, const Vector3& size, const Color3& color,
    const Vector3& velocity)
{
    JPH::RefConst<JPH::Shape> joltShape;
    Vector3 scale;

    if(shape == ShapeType::Box) {
        scale = size*0.5f; /* half-extents == unit-cube mesh scale */
        joltShape = new JPH::BoxShape{JPH::Vec3{scale.x(), scale.y(), scale.z()}};
    } else {
        const Float radius = size.x()*0.5f;
        scale = Vector3{radius};
        joltShape = new JPH::SphereShape{radius};
    }

    const JPH::EMotionType motionType =
        mass == 0.0f ? JPH::EMotionType::Static : JPH::EMotionType::Dynamic;

    JPH::BodyCreationSettings settings{
        joltShape,
        JPH::Vec3{position.x(), position.y(), position.z()},
        JPH::Quat::sIdentity(),
        motionType,
        JPH::ObjectLayer{0}
    };
    settings.mFriction = 0.075f;
    settings.mRestitution = 0.35f;

    if(mass > 0.0f) {
        const JPH::MassProperties base = joltShape->GetMassProperties();
        if(base.mMass > 0.0f) {
            const Float density = mass/base.mMass;
            settings.mOverrideMassProperties =
                JPH::EOverrideMassProperties::MassAndInertiaProvided;
            settings.mMassPropertiesOverride.mMass = mass;
            settings.mMassPropertiesOverride.mInertia = base.mInertia*density;
        }
    }

    auto& bodyInterface = _physicsSystem->GetBodyInterface();
    const JPH::BodyID id = bodyInterface.CreateAndAddBody(
        settings, JPH::EActivation::Activate);

    if(velocity != Vector3{})
        bodyInterface.SetLinearVelocity(id,
            JPH::Vec3{velocity.x(), velocity.y(), velocity.z()});

    arrayAppend(_bodies, InPlaceInit, id, shape, color, scale);
}

JoltExample::JoltExample(const Arguments& arguments):
    Platform::Application{arguments, NoCreate}
{
    /* Try 8x MSAA, fall back to zero samples if not possible. Enable only 2x
       MSAA if we have enough DPI. */
    {
        const Vector2 dpiScaling = this->dpiScaling({});
        Configuration conf;
        conf.setTitle("Magnum Jolt Physics Integration Example")
            .setSize(conf.size(), dpiScaling);
        GLConfiguration glConf;
        glConf.setSampleCount(dpiScaling.max() < 2.0f ? 8 : 2);
        if(!tryCreate(conf, glConf))
            create(conf, glConf.setSampleCount(0));
    }

    /* Camera */
    const auto size = Vector2{windowSize()};
    _projectionMatrix = Matrix4::perspectiveProjection(35.0_degf,
        size.x()/size.y(), 0.01f, 100.0f);
    updateCameraMatrices();

    /* Instanced Phong shader */
    _shader = Shaders::PhongGL{Shaders::PhongGL::Configuration{}
        .setFlags(Shaders::PhongGL::Flag::VertexColor|
                  Shaders::PhongGL::Flag::InstancedTransformation)};
    _shader.setAmbientColor(0x111111_rgbf)
           .setSpecularColor(0x330000_rgbf)
           .setLightPositions({{10.0f, 15.0f, 5.0f, 0.0f}});

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

    /* Depth + polygon offset for debug lines */
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::enable(GL::Renderer::Feature::PolygonOffsetFill);
    GL::Renderer::setPolygonOffset(2.0f, 0.5f);

    /* Jolt init */
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory;
    JPH::RegisterTypes();

    _debugDraw.emplace();
    _debugDraw->setMode(static_cast<UnsignedInt>(JoltIntegration::DebugDraw::Mode::DrawShape |
                                                 JoltIntegration::DebugDraw::Mode::DrawShapeWireframe |
                                                 JoltIntegration::DebugDraw::Mode::DrawBoundingBox |
                                                 JoltIntegration::DebugDraw::Mode::DrawVelocity
                                                 ));

    _tmpAllocator.emplace(256 * 1024 * 1024); // 256 MB; use 512 MB if Update still asserts

    _jobSystem.Init(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
        std::thread::hardware_concurrency() - 1);

    _physicsSystem.emplace();
    /* Init jolt with buffer up to 32x32x32 bodies */
    _physicsSystem->Init(
    65536,             /* maxBodies: 32^3 + ground + shots */
    64,           /* bodyMutexes (power of 2) */
    524288,          /* maxBodyPairs — dense 32^3 stack */
    262144,   /* maxContactConstraints */
    _broadPhaseLayerInterface,
    _objectVsBroadPhaseLayerFilter,
    _objectLayerPairFilter);
    _physicsSystem->SetGravity(JPH::Vec3{0.0f, -9.81f, 0.0f});

    constexpr Vector3 boxSize{1.0f};
    constexpr Vector3 groundSize{35.0f, 1.0f, 35.0f};

    addBody(0.0f, ShapeType::Box, {0.0f, -0.5f, 0.0f}, groundSize, 0xffffff_rgbf);

    Deg hue = 42.0_degf;
    constexpr Int cubeSize = 16;
    constexpr Float cubeOffset = (static_cast<Float>(cubeSize) - 1.0f)*0.5f;

    for(Int i = 0; i != cubeSize; ++i)
        for(Int j = 0; j != cubeSize; ++j)
            for(Int k = 0; k != cubeSize; ++k)
                addBody(1.0f, ShapeType::Box,
                    {
                        static_cast<Float>(i) - cubeOffset,
                        static_cast<Float>(j) + 4.0f,
                        static_cast<Float>(k) - cubeOffset
                    },
                    boxSize,
                    Color3::fromHsv({hue += 137.5_degf, 0.75f, 0.9f}));

    setSwapInterval(1);
    setMinimalLoopPeriod(16.0_msec);
    _timeline.start();
}

JoltExample::~JoltExample() {
    /* Destroy bodies before the physics system */
    if(_physicsSystem) {
        auto& bodyInterface = _physicsSystem->GetBodyInterface();
        for(const BodyInstance& body: _bodies) {
            bodyInterface.RemoveBody(body.id);
            bodyInterface.DestroyBody(body.id);
        }
        arrayClear(_bodies);
    }

    _physicsSystem = nullptr;
    _tmpAllocator = nullptr;
    _debugDraw = nullptr;

    if(JPH::Factory::sInstance) {
        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }
}

void JoltExample::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});
    const auto size = Vector2{event.windowSize()};
    _projectionMatrix = Matrix4::perspectiveProjection(35.0_degf,
        size.x()/size.y(), 0.01f, 100.0f);
}

void JoltExample::drawEvent() {
    GL::defaultFramebuffer.clear(
        GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    const Float dt = Math::min(_timeline.previousFrameDuration(), 1.0f / 60.0f);
    _physicsSystem->Update(dt, 1, _tmpAllocator.get(), &_jobSystem);

    auto& bodyInterface = _physicsSystem->GetBodyInterface();

    arrayClear(_boxInstanceData);
    arrayClear(_sphereInstanceData);

    for(std::size_t i = 0; i < _bodies.size(); ) {
        JPH::Vec3 positionJ{};
        JPH::Quat rotationJ{};
        bodyInterface.GetPositionAndRotation(_bodies[i].id, positionJ, rotationJ);

        const Vector3 position{positionJ};
        if(position.dot() > 100.0f*100.0f) {
            bodyInterface.RemoveBody(_bodies[i].id);
            bodyInterface.DestroyBody(_bodies[i].id);
            arrayRemove(_bodies, i);
            continue;
        }

        if(_drawCubes) {
            const Quaternion rotation{rotationJ};
            const Matrix4 world = Matrix4::from(rotation.toMatrix(), position)*
                                  Matrix4::scaling(_bodies[i].scale);

            const Matrix4 mv = _viewMatrix*world;

            Containers::Array<InstanceData>& data =
                _bodies[i].shape == ShapeType::Box ?
                    _boxInstanceData : _sphereInstanceData;
            arrayAppend(data, InPlaceInit, mv, mv.normalMatrix(), _bodies[i].color);
        }

        ++i;
    }

    if(_drawCubes) {
        _shader.setProjectionMatrix(_projectionMatrix);

        _boxInstanceBuffer.setData(_boxInstanceData, GL::BufferUsage::DynamicDraw);
        _box.setInstanceCount(_boxInstanceData.size());
        _shader.draw(_box);

        _sphereInstanceBuffer.setData(_sphereInstanceData, GL::BufferUsage::DynamicDraw);
        _sphere.setInstanceCount(_sphereInstanceData.size());
        _shader.draw(_sphere);
    }

    if(_drawDebug && _debugDraw) {
        if(_drawCubes)
            GL::Renderer::setDepthFunction(GL::Renderer::DepthFunction::LessOrEqual);

        _debugDraw->setTransformationProjectionMatrix(_projectionMatrix*_viewMatrix);
        _physicsSystem->DrawBodies(_debugDraw->drawSettings(), _debugDraw.get());
        _debugDraw->flush();

        if(_drawCubes)
            GL::Renderer::setDepthFunction(GL::Renderer::DepthFunction::Less);
    }

    swapBuffers();
    _timeline.nextFrame();
    redraw();
}

void JoltExample::keyPressEvent(KeyEvent& event) {
    if(event.key() == Key::D) {
        if(_drawCubes && _drawDebug) {
            _drawDebug = false;
        } else if(_drawCubes && !_drawDebug) {
            _drawCubes = false;
            _drawDebug = true;
        } else {
            _drawCubes = true;
            _drawDebug = true;
        }
    } else if(event.key() == Key::S) {
        _shootBox ^= true;
    } else if(event.key() == Key::Esc) {
        exit();
    } else return;

    event.setAccepted();
}

void JoltExample::pointerPressEvent(PointerEvent& event) {
    if(!event.isPrimary())
        return;

    /* RMB: start orbit */
    if(event.pointer() & Pointer::MouseRight) {
        _orbiting = true;
        _previousPointerPosition = event.position();
        event.setAccepted();
        return;
    }

    if(!(event.pointer() & (Pointer::MouseLeft|Pointer::Finger)))
        return;

    const Vector2 position = event.position()*
        Vector2{framebufferSize()}/Vector2{windowSize()};
    const Matrix4 cameraAbsolute = cameraObjectMatrix();
    const Vector2 ndc =
        Vector2::yScale(-1.0f)*
        (position/Vector2{framebufferSize()} - Vector2{0.5f})*2.0f;
    const Vector4 camNear =
        _projectionMatrix.inverted()*Vector4{ndc.x(), ndc.y(), -1.0f, 1.0f};
    const Vector3 dirCam = (camNear.xyz()/camNear.w()).normalized();
    const Vector3 direction =
        (cameraAbsolute.rotationScaling()*dirCam).normalized();

    constexpr Vector3 boxSize{1.0f};
    constexpr Vector3 sphereSize{1.5f};

    addBody(_shootBox ? 1.0f : 5.0f,
            _shootBox ? ShapeType::Box : ShapeType::Sphere,
            cameraAbsolute.translation(),
            _shootBox ? boxSize : sphereSize,
            _shootBox ? 0x880000_rgbf : 0x220000_rgbf,
            direction*75.0f);

    event.setAccepted();
}

void JoltExample::pointerReleaseEvent(PointerEvent& event) {
    if(!event.isPrimary())
        return;

    if(event.pointer() & Pointer::MouseRight) {
        _orbiting = false;
        event.setAccepted();
    }
}

void JoltExample::pointerMoveEvent(PointerMoveEvent& event) {
    if(!event.isPrimary())
        return;

    /* Also start orbit if RMB went down outside the window */
    if(!_orbiting && (event.pointers() & Pointer::MouseRight)) {
        _orbiting = true;
        _previousPointerPosition = event.position();
    }

    if(!_orbiting)
        return;

    const Vector2 delta = event.position() - _previousPointerPosition;
    _previousPointerPosition = event.position();

    /* Drag right -> yaw right; drag up -> pitch up (tweak signs to taste) */
    _cameraYaw -= Rad{Deg{delta.x()*0.25f}};
    _cameraPitch -= Rad{Deg{delta.y()*0.25f}};

    /* Avoid flipping over the poles */
    _cameraPitch = Math::clamp(_cameraPitch, Rad{-89.0_degf}, Rad{89.0_degf});

    updateCameraMatrices();
    event.setAccepted();
}

void JoltExample::scrollEvent(ScrollEvent& event) {
    if(!event.offset().y())
        return;

    _cameraDistance = Math::clamp(
        _cameraDistance*Math::pow(0.9f, event.offset().y()),
        2.0f, 85.0f);

    updateCameraMatrices();
    event.setAccepted();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::JoltExample)
