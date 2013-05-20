/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013 Vladimír Vondruš <mosra@centrum.cz>
    Copyright © 2013 Jan Dupal <dupal.j@gmail.com>

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

#include <btBulletDynamicsCommon.h>
#include <BulletIntegration/ConvertShape.h>
#include <BulletIntegration/Integration.h>
#include <BulletIntegration/MotionState.h>
#include <DefaultFramebuffer.h>
#include <Math/Constants.h>
#include <Mesh.h>
#include <DebugTools/ShapeRenderer.h>
#include <DebugTools/ResourceManager.h>
#include <Shapes/Box.h>
#include <Shapes/Shape.h>
#include <Shapes/ShapeGroup.h>
#include <Platform/GlutApplication.h>
#include <Renderer.h>
#include <SceneGraph/Camera3D.h>
#include <SceneGraph/MatrixTransformation3D.h>
#include <SceneGraph/Scene.h>
#include <Timeline.h>

namespace Magnum { namespace Examples {

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D<>> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D<>> Scene3D;

class BulletExample: public Platform::Application {
    public:
        explicit BulletExample(const Arguments& arguments);

        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;
        void keyPressEvent(KeyEvent& event) override;
        void mousePressEvent(MouseEvent& event) override;

    private:
        btRigidBody* createRigidBody(Float mass, Object3D* object, btCollisionShape* bShape, ResourceKey renderOptions);
        void shootBox(Vector3& direction);

        DebugTools::ResourceManager manager;
        Scene3D scene;
        SceneGraph::DrawableGroup3D<> drawables;
        Shapes::ShapeGroup3D shapes;
        SceneGraph::Camera3D<>* camera;
        Timeline timeline;

        Object3D *cameraRig, *cameraObject, *ground;
        btDiscreteDynamicsWorld* bWord;
        btCollisionShape* bBoxShape;
        btRigidBody* bGround;
};

BulletExample::BulletExample(const Arguments& arguments): Platform::Application(arguments, nullptr) {
    /* Try 16x MSAA */
    auto conf = new Configuration;
    conf->setTitle("Bullet Integration Example")
        ->setSampleCount(16);
    if(!tryCreateContext(conf))
        createContext(conf->setSampleCount(0));
    else delete conf;

    Renderer::setClearColor(Color3<>(0.15f));

    /* Camera setup */
    (cameraRig = new Object3D(&scene))
        ->translate({0.f, 4.f, 0.f});
    (cameraObject = new Object3D(cameraRig))
        ->translate({0.f, 0.f, 20.f});
    (camera = new SceneGraph::Camera3D<>(cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        ->setPerspective(35.0_degf, 1.0f, 0.001f, 100.0f);

    /* Debug draw setup */
    manager.set("ground", (new DebugTools::ShapeRendererOptions)->setColor(Color3<>(0.45f)));
    manager.set("box", (new DebugTools::ShapeRendererOptions)->setColor(Color3<>(0.85f)));
    manager.set("redbox", (new DebugTools::ShapeRendererOptions)->setColor({0.9f, 0.0f, 0.0f}));

    /* Bullet setup */
    btBroadphaseInterface* broadphase = new btDbvtBroadphase();
    btDefaultCollisionConfiguration* collisionConfiguration = new btDefaultCollisionConfiguration();
    btCollisionDispatcher* dispatcher = new btCollisionDispatcher(collisionConfiguration);
    btSequentialImpulseConstraintSolver* solver = new btSequentialImpulseConstraintSolver;

    (bWord = new btDiscreteDynamicsWorld(dispatcher,broadphase,solver,collisionConfiguration))
        ->setGravity({0, -10, 0});

    /* Create ground */
    ground = new Object3D(&scene);
    btCollisionShape* bGroundShape = new btBoxShape({4.f, .5f, 4.f});
    bGround = createRigidBody(0.f, ground, bGroundShape, "ground");

    /* Create boxes */
    bBoxShape = new btBoxShape({.5f, .5f, .5f});
    for(Int i = 0; i < 5; i++) {
        for(Int j = 0; j < 5; j++) {
            for(Int k = 0; k < 5; k++) {
                Object3D* box = new Object3D(&scene);
                box->translate({i - 2.f, j + 4.f, k - 2.f});
                createRigidBody(1.f, box, bBoxShape, "box");
            }
        }
    }

    timeline.setMinimalFrameTime(1/120.0f);
    timeline.start();

    redraw();
}

btRigidBody* BulletExample::createRigidBody(Float mass, Object3D* object, btCollisionShape* bShape, ResourceKey renderOptions) {
    btVector3 bInertia(0,0,0);
    if(mass != 0.f)
        bShape->calculateLocalInertia(mass, bInertia);

    /* Bullet rigid body setup */
    BulletIntegration::MotionState* motionState = new BulletIntegration::MotionState(object);
    btRigidBody::btRigidBodyConstructionInfo bRigidBodyCI(mass, motionState->btMotionState(), bShape, bInertia);
    btRigidBody* bRigidBody = new btRigidBody(bRigidBodyCI);
    bRigidBody->forceActivationState(DISABLE_DEACTIVATION);
    bWord->addRigidBody(bRigidBody);

    /* Debug draw */
    new DebugTools::ShapeRenderer3D(BulletIntegration::convertShape(object, bShape, &shapes),
                                    renderOptions, &drawables);

    return bRigidBody;
}

void BulletExample::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});

    camera->setViewport(size);
}

void BulletExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color);

    /* Step bullet simulation */
    bWord->stepSimulation(timeline.previousFrameDuration(), 5);

    /* Update positions and render */
    shapes.setClean();
    camera->draw(drawables);

    swapBuffers();
    timeline.nextFrame();
    redraw();
}

void BulletExample::keyPressEvent(KeyEvent& event) {
    if(event.key() == KeyEvent::Key::Down)
        cameraObject->rotateX(5.0_degf);
    else if(event.key() == KeyEvent::Key::Up)
        cameraObject->rotateX(-5.0_degf);
    else if(event.key() == KeyEvent::Key::Left)
        cameraRig->rotateY(-5.0_degf);
    else if(event.key() == KeyEvent::Key::Right)
        cameraRig->rotateY(5.0_degf);
    else return;

    event.setAccepted();
}

void BulletExample::mousePressEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left) {
        Vector2 clickPoint = Vector2::yScale(-1.0f)*(Vector2(event.position())/defaultFramebuffer.viewport().size()-Vector2(0.5f))*camera->projectionSize();
        Vector3 direction = (cameraObject->absoluteTransformation().rotationScaling() * Vector3(clickPoint, -1.f)).normalized();
        shootBox(direction);
        event.setAccepted();
    }
}

void BulletExample::shootBox(Vector3& dir) {
    Object3D* box = new Object3D(&scene);
    box->translate(cameraObject->absoluteTransformation().translation());

    createRigidBody(1.f, box, bBoxShape, "redbox")->setLinearVelocity(btVector3(dir*50.f));
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::BulletExample)
