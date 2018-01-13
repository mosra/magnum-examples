/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 —
            Vladimír Vondruš <mosra@centrum.cz>
        2013 — Jan Dupal <dupal.j@gmail.com>

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
#include <Magnum/BulletIntegration/ConvertShape.h>
#include <Magnum/BulletIntegration/Integration.h>
#include <Magnum/BulletIntegration/MotionState.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Math/Constants.h>
#include <Magnum/Mesh.h>
#include <Magnum/DebugTools/ShapeRenderer.h>
#include <Magnum/DebugTools/ResourceManager.h>
#include <Magnum/Shapes/Box.h>
#include <Magnum/Shapes/Shape.h>
#include <Magnum/Shapes/ShapeGroup.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Renderer.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Timeline.h>

namespace Magnum { namespace Examples {

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

class BulletExample: public Platform::Application {
    public:
        explicit BulletExample(const Arguments& arguments);

    private:
        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;
        void keyPressEvent(KeyEvent& event) override;
        void mousePressEvent(MouseEvent& event) override;

        btRigidBody* createRigidBody(Float mass, Object3D& object, btCollisionShape& bShape, ResourceKey renderOptions);
        void shootBox(Vector3& direction);

        DebugTools::ResourceManager _manager;
        Scene3D _scene;
        SceneGraph::DrawableGroup3D _drawables;
        Shapes::ShapeGroup3D _shapes;
        SceneGraph::Camera3D* _camera;
        Timeline _timeline;

        Object3D *_cameraRig, *_cameraObject, *_ground;
        btDiscreteDynamicsWorld* _bWord;
        btCollisionShape* _bBoxShape;
        btRigidBody* _bGround;
};

BulletExample::BulletExample(const Arguments& arguments): Platform::Application(arguments, NoCreate) {
    /* Try 16x MSAA */
    Configuration conf;
    conf.setTitle("Magnum Bullet Integration Example")
        .setSampleCount(8);
    if(!tryCreateContext(conf))
        createContext(conf.setSampleCount(0));

    /* Camera setup */
    (_cameraRig = new Object3D(&_scene))
        ->translate({0.f, 4.f, 0.f});
    (_cameraObject = new Object3D(_cameraRig))
        ->translate({0.f, 0.f, 20.f});
    (_camera = new SceneGraph::Camera3D(*_cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(Deg(35.0f), 1.0f, 0.001f, 100.0f))
        .setViewport(defaultFramebuffer.viewport().size());

    /* Debug draw setup */
    /** @todo Revert it back to oneliners when setters return rvalue refs properly */
    auto groundOptions = new DebugTools::ShapeRendererOptions;
    groundOptions->setColor(Color3(0.45f));
    auto boxOptions = new DebugTools::ShapeRendererOptions;
    boxOptions->setColor(Color3(0.85f));
    auto redBoxOptions = new DebugTools::ShapeRendererOptions;
    redBoxOptions->setColor({0.9f, 0.0f, 0.0f});
    _manager.set("ground", groundOptions)
        .set("box", boxOptions)
        .set("redbox", redBoxOptions);

    /* Bullet setup */
    btBroadphaseInterface* broadphase = new btDbvtBroadphase();
    btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
    btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);
    btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;

    (_bWord = new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collisionConfiguration))
        ->setGravity({0, -10, 0});

    /* Create ground */
    _ground = new Object3D(&_scene);
    btCollisionShape* bGroundShape = new btBoxShape({4.f, .5f, 4.f});
    _bGround = createRigidBody(0.f, *_ground, *bGroundShape, "ground");

    /* Create boxes */
    _bBoxShape = new btBoxShape({.5f, .5f, .5f});
    for(Int i = 0; i < 5; i++) {
        for(Int j = 0; j < 5; j++) {
            for(Int k = 0; k < 5; k++) {
                Object3D* box = new Object3D(&_scene);
                box->translate({i - 2.f, j + 4.f, k - 2.f});
                createRigidBody(1.f, *box, *_bBoxShape, "box");
            }
        }
    }

    /* Loop at 60 Hz max */
    setSwapInterval(1);
    #ifndef CORRADE_TARGET_EMSCRIPTEN
    setMinimalLoopPeriod(16);
    #endif
    _timeline.start();

    redraw();
}

btRigidBody* BulletExample::createRigidBody(Float mass, Object3D& object, btCollisionShape& bShape, ResourceKey renderOptions) {
    btVector3 bInertia(0,0,0);
    if(mass != 0.f)
        bShape.calculateLocalInertia(mass, bInertia);

    /* Bullet rigid body setup */
    BulletIntegration::MotionState* motionState = new BulletIntegration::MotionState(object);
    btRigidBody::btRigidBodyConstructionInfo bRigidBodyCI(mass, &motionState->btMotionState(), &bShape, bInertia);
    btRigidBody* bRigidBody = new btRigidBody(bRigidBodyCI);
    bRigidBody->forceActivationState(DISABLE_DEACTIVATION);
    _bWord->addRigidBody(bRigidBody);

    /* Debug draw */
    auto shape = BulletIntegration::convertShape(object, bShape, &_shapes);
    CORRADE_INTERNAL_ASSERT(shape);
    new DebugTools::ShapeRenderer3D(*shape, renderOptions, &_drawables);

    return bRigidBody;
}

void BulletExample::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});

    _camera->setViewport(size);
}

void BulletExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color);

    /* Step bullet simulation */
    _bWord->stepSimulation(_timeline.previousFrameDuration(), 5);

    /* Update positions and render */
    _shapes.setClean();
    _camera->draw(_drawables);

    swapBuffers();
    _timeline.nextFrame();
    redraw();
}

void BulletExample::keyPressEvent(KeyEvent& event) {
    if(event.key() == KeyEvent::Key::Down)
        _cameraObject->rotateX(Deg(5.0f));
    else if(event.key() == KeyEvent::Key::Up)
        _cameraObject->rotateX(Deg(-5.0f));
    else if(event.key() == KeyEvent::Key::Left)
        _cameraRig->rotateY(Deg(-5.0f));
    else if(event.key() == KeyEvent::Key::Right)
        _cameraRig->rotateY(Deg(5.0f));
    else return;

    event.setAccepted();
}

void BulletExample::mousePressEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left) {
        Vector2 clickPoint = Vector2::yScale(-1.0f)*(Vector2(event.position())/Vector2(defaultFramebuffer.viewport().size())-Vector2(0.5f))* _camera->projectionSize();
        Vector3 direction = (_cameraObject->absoluteTransformation().rotationScaling() * Vector3(clickPoint, -1.f)).normalized();
        shootBox(direction);
        event.setAccepted();
    }
}

void BulletExample::shootBox(Vector3& dir) {
    Object3D* box = new Object3D(&_scene);
    box->translate(_cameraObject->absoluteTransformation().translation());

    createRigidBody(1.f, *box, *_bBoxShape, "redbox")->setLinearVelocity(btVector3(dir*50.f));
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::BulletExample)
