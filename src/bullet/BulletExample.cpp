/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 —
            Vladimír Vondruš <mosra@centrum.cz>
        2013 — Jan Dupal <dupal.j@gmail.com>
        2019 — Max Schwarz <max.schwarz@online.de>

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

#include <btBulletDynamicsCommon.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Pointer.h>
#include <Magnum/Timeline.h>
#include <Magnum/BulletIntegration/Integration.h>
#include <Magnum/BulletIntegration/MotionState.h>
#include <Magnum/BulletIntegration/DebugDraw.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Constants.h>
#include <Magnum/Math/Color.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/Transform.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/UVSphere.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/MeshData.h>

namespace Magnum { namespace Examples {

using namespace Math::Literals;

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

struct InstanceData {
    Matrix4 transformationMatrix;
    Matrix3x3 normalMatrix;
    Color3 color;
};

class BulletExample: public Platform::Application {
    public:
        explicit BulletExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void keyPressEvent(KeyEvent& event) override;
        void mousePressEvent(MouseEvent& event) override;

        GL::Mesh _box{NoCreate}, _sphere{NoCreate};
        GL::Buffer _boxInstanceBuffer{NoCreate}, _sphereInstanceBuffer{NoCreate};
        Shaders::Phong _shader{NoCreate};
        BulletIntegration::DebugDraw _debugDraw{NoCreate};
        Containers::Array<InstanceData> _boxInstanceData, _sphereInstanceData;

        btDbvtBroadphase _bBroadphase;
        btDefaultCollisionConfiguration _bCollisionConfig;
        btCollisionDispatcher _bDispatcher{&_bCollisionConfig};
        btSequentialImpulseConstraintSolver _bSolver;

        /* The world has to live longer than the scene because RigidBody
           instances have to remove themselves from it on destruction */
        btDiscreteDynamicsWorld _bWorld{&_bDispatcher, &_bBroadphase, &_bSolver, &_bCollisionConfig};

        Scene3D _scene;
        SceneGraph::Camera3D* _camera;
        SceneGraph::DrawableGroup3D _drawables;
        Timeline _timeline;

        Object3D *_cameraRig, *_cameraObject;

        btBoxShape _bBoxShape{{0.5f, 0.5f, 0.5f}};
        btSphereShape _bSphereShape{0.25f};
        btBoxShape _bGroundShape{{4.0f, 0.5f, 4.0f}};

        bool _drawCubes{true}, _drawDebug{true}, _shootBox{true};
};

class ColoredDrawable: public SceneGraph::Drawable3D {
    public:
        explicit ColoredDrawable(Object3D& object, Containers::Array<InstanceData>& instanceData, const Color3& color, const Matrix4& primitiveTransformation, SceneGraph::DrawableGroup3D& drawables): SceneGraph::Drawable3D{object, &drawables}, _instanceData(instanceData), _color{color}, _primitiveTransformation{primitiveTransformation} {}

    private:
        void draw(const Matrix4& transformation, SceneGraph::Camera3D&) override {
            const Matrix4 t = transformation*_primitiveTransformation;
            arrayAppend(_instanceData, Containers::InPlaceInit,
                t, t.normalMatrix(), _color);
        }

        Containers::Array<InstanceData>& _instanceData;
        Color3 _color;
        Matrix4 _primitiveTransformation;
};

class RigidBody: public Object3D {
    public:
        RigidBody(Object3D* parent, Float mass, btCollisionShape* bShape, btDynamicsWorld& bWorld): Object3D{parent}, _bWorld(bWorld) {
            /* Calculate inertia so the object reacts as it should with
               rotation and everything */
            btVector3 bInertia(0.0f, 0.0f, 0.0f);
            if(!Math::TypeTraits<Float>::equals(mass, 0.0f))
                bShape->calculateLocalInertia(mass, bInertia);

            /* Bullet rigid body setup */
            auto* motionState = new BulletIntegration::MotionState{*this};
            _bRigidBody.emplace(btRigidBody::btRigidBodyConstructionInfo{
                mass, &motionState->btMotionState(), bShape, bInertia});
            _bRigidBody->forceActivationState(DISABLE_DEACTIVATION);
            bWorld.addRigidBody(_bRigidBody.get());
        }

        ~RigidBody() {
            _bWorld.removeRigidBody(_bRigidBody.get());
        }

        btRigidBody& rigidBody() { return *_bRigidBody; }

        /* needed after changing the pose from Magnum side */
        void syncPose() {
            _bRigidBody->setWorldTransform(btTransform(transformationMatrix()));
        }

    private:
        btDynamicsWorld& _bWorld;
        Containers::Pointer<btRigidBody> _bRigidBody;
};

BulletExample::BulletExample(const Arguments& arguments): Platform::Application(arguments, NoCreate) {
    /* Try 8x MSAA, fall back to zero samples if not possible. Enable only 2x
       MSAA if we have enough DPI. */
    {
        const Vector2 dpiScaling = this->dpiScaling({});
        Configuration conf;
        conf.setTitle("Magnum Bullet Integration Example")
            .setSize(conf.size(), dpiScaling);
        GLConfiguration glConf;
        glConf.setSampleCount(dpiScaling.max() < 2.0f ? 8 : 2);
        if(!tryCreate(conf, glConf))
            create(conf, glConf.setSampleCount(0));
    }

    /* Camera setup */
    (*(_cameraRig = new Object3D{&_scene}))
        .translate(Vector3::yAxis(3.0f))
        .rotateY(40.0_degf);
    (*(_cameraObject = new Object3D{_cameraRig}))
        .translate(Vector3::zAxis(20.0f))
        .rotateX(-25.0_degf);
    (_camera = new SceneGraph::Camera3D(*_cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.001f, 100.0f))
        .setViewport(GL::defaultFramebuffer.viewport().size());

    /* Create an instanced shader */
    _shader = Shaders::Phong{
        Shaders::Phong::Flag::VertexColor|
        Shaders::Phong::Flag::InstancedTransformation};
    _shader.setAmbientColor(0x111111_rgbf)
           .setSpecularColor(0x330000_rgbf)
           .setLightPositions({{10.0f, 15.0f, 5.0f, 0.0f}});

    /* Box and sphere mesh, with an (initially empty) instance buffer */
    _box = MeshTools::compile(Primitives::cubeSolid());
    _sphere = MeshTools::compile(Primitives::uvSphereSolid(16, 32));
    _boxInstanceBuffer = GL::Buffer{};
    _sphereInstanceBuffer = GL::Buffer{};
    _box.addVertexBufferInstanced(_boxInstanceBuffer, 1, 0,
        Shaders::Phong::TransformationMatrix{},
        Shaders::Phong::NormalMatrix{},
        Shaders::Phong::Color3{});
    _sphere.addVertexBufferInstanced(_sphereInstanceBuffer, 1, 0,
        Shaders::Phong::TransformationMatrix{},
        Shaders::Phong::NormalMatrix{},
        Shaders::Phong::Color3{});

    /* Setup the renderer so we can draw the debug lines on top */
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    GL::Renderer::enable(GL::Renderer::Feature::PolygonOffsetFill);
    GL::Renderer::setPolygonOffset(2.0f, 0.5f);

    /* Bullet setup */
    _debugDraw = BulletIntegration::DebugDraw{};
    _debugDraw.setMode(BulletIntegration::DebugDraw::Mode::DrawWireframe);
    _bWorld.setGravity({0.0f, -10.0f, 0.0f});
    _bWorld.setDebugDrawer(&_debugDraw);

    /* Create the ground */
    auto* ground = new RigidBody{&_scene, 0.0f, &_bGroundShape, _bWorld};
    new ColoredDrawable{*ground, _boxInstanceData, 0xffffff_rgbf,
        Matrix4::scaling({4.0f, 0.5f, 4.0f}), _drawables};

    /* Create boxes with random colors */
    Deg hue = 42.0_degf;
    for(Int i = 0; i != 5; ++i) {
        for(Int j = 0; j != 5; ++j) {
            for(Int k = 0; k != 5; ++k) {
                auto* o = new RigidBody{&_scene, 1.0f, &_bBoxShape, _bWorld};
                o->translate({i - 2.0f, j + 4.0f, k - 2.0f});
                o->syncPose();
                new ColoredDrawable{*o, _boxInstanceData,
                    Color3::fromHsv({hue += 137.5_degf, 0.75f, 0.9f}),
                    Matrix4::scaling(Vector3{0.5f}), _drawables};
            }
        }
    }

    /* Loop at 60 Hz max */
    setSwapInterval(1);
    setMinimalLoopPeriod(16);
    _timeline.start();
}

void BulletExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    /* Housekeeping: remove any objects which are far away from the origin */
    for(Object3D* obj = _scene.children().first(); obj; )
    {
        Object3D* next = obj->nextSibling();
        if(obj->transformation().translation().dot() > 100*100)
            delete obj;

        obj = next;
    }

    /* Step bullet simulation */
    _bWorld.stepSimulation(_timeline.previousFrameDuration(), 5);

    if(_drawCubes) {
        /* Populate instance data with transformations and colors */
        arrayResize(_boxInstanceData, 0);
        arrayResize(_sphereInstanceData, 0);
        _camera->draw(_drawables);

        _shader.setProjectionMatrix(_camera->projectionMatrix());

        /* Upload instance data to the GPU (orphaning the previous buffer
           contents) and draw all cubes in one call, and all spheres (if any)
           in another call */
        _boxInstanceBuffer.setData(_boxInstanceData, GL::BufferUsage::DynamicDraw);
        _box.setInstanceCount(_boxInstanceData.size());
        _shader.draw(_box);

        _sphereInstanceBuffer.setData(_sphereInstanceData, GL::BufferUsage::DynamicDraw);
        _sphere.setInstanceCount(_sphereInstanceData.size());
        _shader.draw(_sphere);
    }

    /* Debug draw. If drawing on top of cubes, avoid flickering by setting
       depth function to <= instead of just <. */
    if(_drawDebug) {
        if(_drawCubes)
            GL::Renderer::setDepthFunction(GL::Renderer::DepthFunction::LessOrEqual);

        _debugDraw.setTransformationProjectionMatrix(
            _camera->projectionMatrix()*_camera->cameraMatrix());
        _bWorld.debugDrawWorld();

        if(_drawCubes)
            GL::Renderer::setDepthFunction(GL::Renderer::DepthFunction::Less);
    }

    swapBuffers();
    _timeline.nextFrame();
    redraw();
}

void BulletExample::keyPressEvent(KeyEvent& event) {
    /* Movement */
    if(event.key() == KeyEvent::Key::Down) {
        _cameraObject->rotateX(5.0_degf);
    } else if(event.key() == KeyEvent::Key::Up) {
        _cameraObject->rotateX(-5.0_degf);
    } else if(event.key() == KeyEvent::Key::Left) {
        _cameraRig->rotateY(-5.0_degf);
    } else if(event.key() == KeyEvent::Key::Right) {
        _cameraRig->rotateY(5.0_degf);

    /* Toggling draw modes */
    } else if(event.key() == KeyEvent::Key::D) {
        if(_drawCubes && _drawDebug) {
            _drawDebug = false;
        } else if(_drawCubes && !_drawDebug) {
            _drawCubes = false;
            _drawDebug = true;
        } else if(!_drawCubes && _drawDebug) {
            _drawCubes = true;
            _drawDebug = true;
        }

    /* What to shoot */
    } else if(event.key() == KeyEvent::Key::S) {
        _shootBox ^= true;
    } else return;

    event.setAccepted();
}

void BulletExample::mousePressEvent(MouseEvent& event) {
    /* Shoot an object on click */
    if(event.button() == MouseEvent::Button::Left) {
        /* First scale the position from being relative to window size to being
           relative to framebuffer size as those two can be different on HiDPI
           systems */
        const Vector2i position = event.position()*Vector2{framebufferSize()}/Vector2{windowSize()};
        const Vector2 clickPoint = Vector2::yScale(-1.0f)*(Vector2{position}/Vector2{framebufferSize()} - Vector2{0.5f})*_camera->projectionSize();
        const Vector3 direction = (_cameraObject->absoluteTransformation().rotationScaling()*Vector3{clickPoint, -1.0f}).normalized();

        auto* object = new RigidBody{
            &_scene,
            _shootBox ? 1.0f : 5.0f,
            _shootBox ? static_cast<btCollisionShape*>(&_bBoxShape) : &_bSphereShape,
            _bWorld};
        object->translate(_cameraObject->absoluteTransformation().translation());
        /* Has to be done explicitly after the translate() above, as Magnum ->
           Bullet updates are implicitly done only for kinematic bodies */
        object->syncPose();

        /* Create either a box or a sphere */
        new ColoredDrawable{*object,
            _shootBox ? _boxInstanceData : _sphereInstanceData,
            _shootBox ? 0x880000_rgbf : 0x220000_rgbf,
            Matrix4::scaling(Vector3{_shootBox ? 0.5f : 0.25f}), _drawables};

        /* Give it an initial velocity */
        object->rigidBody().setLinearVelocity(btVector3{direction*25.f});

        event.setAccepted();
    }
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::BulletExample)
