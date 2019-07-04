/*
    This file is part of Magnum.
    Original authors — credit is appreciated but not required:
        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
            Vladimír Vondruš <mosra@centrum.cz>
        2019 — Konstantinos Chatzilygeroudis <costashatz@gmail.com>
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

#include "DartExampleHelpers.h"

#include <dart/collision/dart/DARTCollisionDetector.hpp>
#include <dart/constraint/ConstraintSolver.hpp>
#include <dart/dynamics/BallJoint.hpp>
#include <dart/dynamics/BodyNode.hpp>
#include <dart/dynamics/BoxShape.hpp>
#include <dart/dynamics/CylinderShape.hpp>
#include <dart/dynamics/EllipsoidShape.hpp>
#include <dart/dynamics/FreeJoint.hpp>
#include <dart/dynamics/MeshShape.hpp>
#include <dart/dynamics/RevoluteJoint.hpp>
#include <dart/dynamics/SoftBodyNode.hpp>
#include <dart/dynamics/SoftMeshShape.hpp>
#include <dart/dynamics/Skeleton.hpp>
#include <dart/dynamics/WeldJoint.hpp>
#include <dart/simulation/World.hpp>

#include <Corrade/Utility/Directory.h>

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/OpenGLTester.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/ResourceManager.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Object.hpp>
#include <Magnum/SceneGraph/SceneGraph.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Timeline.h>
#include <Magnum/Trade/PhongMaterialData.h>

#include <Magnum/DartIntegration/ConvertShapeNode.h>
#include <Magnum/DartIntegration/World.h>

#include <configure.h>

#if DART_MAJOR_VERSION == 6
    #include <dart/utils/urdf/urdf.hpp>
    #define DartLoader dart::utils::DartLoader
#else
    #include <dart/io/urdf/urdf.hpp>
    #define DartLoader dart::io::DartLoader
#endif

namespace Magnum { namespace Examples {

using namespace Magnum::Math::Literals;

typedef ResourceManager<GL::Buffer, GL::Mesh, Shaders::Phong> ViewerResourceManager;
typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

struct MaterialData{
    Vector3 _ambientColor,
            _diffuseColor,
            _specularColor;
    Float _shininess;
    Vector3 _scaling;
};

class ColoredObject: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit ColoredObject(GL::Mesh* mesh, const MaterialData& material, Object3D* parent, SceneGraph::DrawableGroup3D* group);

        ColoredObject& setMesh(GL::Mesh* mesh);
        ColoredObject& setMaterial(const MaterialData& material);
        ColoredObject& setSoftBody(bool softBody = true);

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        GL::Mesh* _mesh;
        Resource<Shaders::Phong> _shader;
        MaterialData _material;
        bool _isSoftBody = false;
};

class DartExample: public Platform::Application {
    public:
        explicit DartExample(const Arguments& arguments);

    private:
        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;
        void keyPressEvent(KeyEvent& event) override;

        void updateGraphics();
        void updateManipulator();

        ViewerResourceManager _resourceManager;

        Scene3D _scene;
        SceneGraph::DrawableGroup3D _drawables;
        SceneGraph::Camera3D* _camera;
        Timeline _timeline;

        Object3D *_cameraRig, *_cameraObject;

         /* DART */
        std::unique_ptr<Magnum::DartIntegration::World> _dartWorld;
        std::unordered_map<DartIntegration::Object*, ColoredObject*> _coloredObjects;
        std::vector<Object3D*> _dartObjs;
        dart::dynamics::SkeletonPtr _manipulator, _model, _redBoxSkel, _greenBoxSkel, _blueBoxSkel;
        dart::simulation::WorldPtr _world;
        Eigen::VectorXd _redInitPosition, _greenInitPosition, _blueInitPosition;

        /* DART control */
        Eigen::VectorXd _desiredPosition;
        Eigen::MatrixXd _desiredOrientation;
        double _gripperDesiredPosition;
        const double _pGripperGain = 100.;
        const double _pLinearGain = 500.;
        const double _dLinearGain = 200.;
        const double _pOrientationGain = 100.;
        const double _dOrientationGain = 50.;
        const double _dRegularization = 10.;
        const double _pGain = 2.25;
        const double _dGain = 5.;

        /* Simple State Machine */
        size_t state = 0; /* 0: go to home position, 1: go to active box */
};

DartExample::DartExample(const Arguments& arguments): Platform::Application(arguments, NoCreate) {
    /* Try 16x MSAA */
    Configuration conf;
    GLConfiguration glConf;
    conf.setTitle("Magnum Dart Integration Example");
    glConf.setSampleCount(8);
    if(!tryCreate(conf, glConf))
        create(conf, glConf.setSampleCount(0));

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    /* Camera setup */
    (_cameraRig = new Object3D(&_scene));
    (_cameraObject = new Object3D(_cameraRig));
    (_camera = new SceneGraph::Camera3D(*_cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(Deg(35.0f), 1.0f, 0.001f, 100.0f))
        .setViewport(GL::defaultFramebuffer.viewport().size());
    /* DART has +Z-axis as up direction*/
    _cameraObject->setTransformation(Magnum::Matrix4::lookAt(Vector3{0.f, 3.f, 1.5f}, Vector3{0.f, 0.f, 0.5f}, Vector3{0.f, 0.f, 1.f}));

    /* DART: Load Skeletons/Robots */
    DartLoader loader;
    /* Add packages (need for URDF loading) */
    loader.addPackageDirectory("iiwa_description", std::string(DARTEXAMPLE_DIR) + "/urdf/");
    loader.addPackageDirectory("robotiq_arg85_description", std::string(DARTEXAMPLE_DIR) + "/urdf/");
    std::string filename = std::string(DARTEXAMPLE_DIR) + "/urdf/iiwa14_simple.urdf";

    /* First load the KUKA manipulator */
    _manipulator = loader.parseSkeleton(filename);
    for(size_t i = 0; i < _manipulator->getNumJoints(); i++)
        _manipulator->getJoint(i)->setPositionLimitEnforced(true);

    _manipulator->disableSelfCollisionCheck();

    /* Clone the KUKA manipulator to use it as model in a model-based controller */
#if DART_VERSION_AT_LEAST(6, 7, 2)
    _model = _manipulator->cloneSkeleton();
#else
    _model = _manipulator->clone();
#endif

    /* Load the Robotiq 2-finger gripper */
    filename = std::string(DARTEXAMPLE_DIR) + "/urdf/robotiq.urdf";
    auto gripper_skel = loader.parseSkeleton(filename);
    /* The gripper is controlled in velocity mode: servo actuator */
    gripper_skel->getJoint("finger_joint")->setActuatorType(dart::dynamics::Joint::SERVO);

    /* Attach gripper to manipulator */
    dart::dynamics::WeldJoint::Properties properties = dart::dynamics::WeldJoint::Properties();
    properties.mT_ChildBodyToJoint.translation() = Eigen::Vector3d(0, 0, 0.0001);
    gripper_skel->getRootBodyNode()->moveTo<dart::dynamics::WeldJoint>(_manipulator->getBodyNode("iiwa_link_ee"), properties);

    /* Create floor */
    auto floorSkel = createFloor();

    /* Create red box */
    _redBoxSkel = createBox("red", Eigen::Vector3d(0.8, 0., 0.));
    /* Position it at (0.5, 0.) [x,y] */
    _redBoxSkel->setPosition(3, 0.5);
    /* Save initial position for resetting */
    _redInitPosition = _redBoxSkel->getPositions();
    /* Create green box */
    _greenBoxSkel = createBox("green", Eigen::Vector3d(0., 0.8, 0.));
    /* Position it at (0.5, 0.2) */
    _greenBoxSkel->setPosition(3, 0.5);
    _greenBoxSkel->setPosition(4, 0.2);
    /* Save initial position for resetting */
    _greenInitPosition = _greenBoxSkel->getPositions();
    /* Create blue box */
    _blueBoxSkel = createBox("blue", Eigen::Vector3d(0., 0., 0.8));
    /* Position it at (0.5, -0.2) */
    _blueBoxSkel->setPosition(3, 0.5);
    _blueBoxSkel->setPosition(4, -0.2);
    /* Save initial position for resetting */
    _blueInitPosition = _blueBoxSkel->getPositions();

    /* Create the DART world */
    _world = dart::simulation::WorldPtr(new dart::simulation::World);
    /* Use a simple but very fast collision detector. This plays the most important role for having a fast simulation. */
    _world->getConstraintSolver()->setCollisionDetector(dart::collision::DARTCollisionDetector::create());
    /* Add the robot/objects in our DART world */
    _world->addSkeleton(_manipulator);
    _world->addSkeleton(floorSkel);
    _world->addSkeleton(_redBoxSkel);
    _world->addSkeleton(_greenBoxSkel);
    _world->addSkeleton(_blueBoxSkel);

    /* Setup desired locations/orientations */
    _desiredOrientation = Eigen::MatrixXd(3, 3); /* 3D rotation matrix */
    /* Looking towards -Z direction (towards the floor) */
    _desiredOrientation << 1, 0, 0,
                           0, -1, 0,
                           0, 0, -1;

    /* Default desired position is the red box position */
    _desiredPosition = _redBoxSkel->getPositions().tail(3);
    _desiredPosition[2] += 0.17;

    /* Gripper default desired position: stay in the initial configuration (i.e., open) */
    _gripperDesiredPosition = _manipulator->getPosition(7);

    /* faster simulation step; less accurate simulation/collision detection */
    /* _world->setTimeStep(0.015); */
    /* slower simulation step; better accuracy */
    _world->setTimeStep(0.001);

    /* Create our DARTIntegration object/world */
    auto dartObj = new Object3D{&_scene};
    _dartWorld.reset(new DartIntegration::World(*dartObj, *_world));

    /* Phong shader instance */
    _resourceManager.set("color", new Shaders::Phong({}, 2));

    /* Loop at 60 Hz max */
    setSwapInterval(1);
    setMinimalLoopPeriod(16);
    _timeline.start();

    redraw();

    Debug{} << "DART Integration Example";
    Debug{} << "========================";
    Debug{} << "Press 'r' to go above the red box (switches to control mode)";
    Debug{} << "Press 'g' to go above the green box (switches to control mode)";
    Debug{} << "Press 'b' to go above the blue box (switches to control mode)";
    Debug{} << "Press 'c' to close the gripper";
    Debug{} << "Press 'o' to open the gripper";
    Debug{} << "Press 'd' to go further down (only in control mode)";
    Debug{} << "Press 'u' to go further up (only in control mode)";
    Debug{} << "Press 'z' or 'x' to move sideways (only in control mode)";
    Debug{} << "Press 'h' to go to home position (switches to idle mode)";
    Debug{} << "Press 'space' to reset the world";
}

void DartExample::viewportEvent(const Vector2i& size) {
    GL::defaultFramebuffer.setViewport({{}, size});

    _camera->setViewport(size);
}

void DartExample::drawEvent() {
    GL::defaultFramebuffer.clear(
        GL::FramebufferClear::Color|GL::FramebufferClear::Depth);
    /* Measure time spent and make as many simulation steps */
    double dt = _timeline.previousFrameDuration();
    UnsignedInt steps = std::ceil(dt / _world->getTimeStep());

    /* Step DART simulation */
    for (UnsignedInt i = 0; i < steps; i++) {
        updateManipulator();
        _dartWorld->step();
    }

    /* Update graphic meshes/materials and render */
    updateGraphics();
    _camera->draw(_drawables);

    swapBuffers();
    _timeline.nextFrame();
    redraw();
}

void DartExample::keyPressEvent(KeyEvent& event) {
    if(event.key() == KeyEvent::Key::Down)
        _cameraObject->rotateX(Deg(5.0f));
    else if(event.key() == KeyEvent::Key::Up)
        _cameraObject->rotateX(Deg(-5.0f));
    else if(event.key() == KeyEvent::Key::Left)
        _cameraRig->rotateY(Deg(-5.0f));
    else if(event.key() == KeyEvent::Key::Right)
        _cameraRig->rotateY(Deg(5.0f));
    else if(event.key() == KeyEvent::Key::C)
    {
        _gripperDesiredPosition = 0.3;
    }
    else if(event.key() == KeyEvent::Key::O)
    {
        _gripperDesiredPosition = 0.;
    }
    else if(event.key() == KeyEvent::Key::U && state == 1)
    {
        _desiredPosition[2] += 0.1;
    }
    else if(event.key() == KeyEvent::Key::D && state == 1)
    {
        _desiredPosition[2] -= 0.1;
    }
    else if(event.key() == KeyEvent::Key::Z && state == 1)
    {
        _desiredPosition[1] += 0.2;
    }
    else if(event.key() == KeyEvent::Key::X && state == 1)
    {
        _desiredPosition[1] -= 0.2;
    }
    else if(event.key() == KeyEvent::Key::R)
    {
        _desiredPosition = _redBoxSkel->getPositions().tail(3);
        _desiredPosition[2] += 0.25;
        state = 1;
    }
    else if(event.key() == KeyEvent::Key::G)
    {
        _desiredPosition = _greenBoxSkel->getPositions().tail(3);
        _desiredPosition[2] += 0.25;
        state = 1;
    }
    else if(event.key() == KeyEvent::Key::B)
    {
        _desiredPosition = _blueBoxSkel->getPositions().tail(3);
        _desiredPosition[2] += 0.25;
        state = 1;
    }
    else if(event.key() == KeyEvent::Key::H)
    {
        state = 0;
    }
    else if(event.key() == KeyEvent::Key::Space)
    {
        /* Reset state machine */
        state = 0;
        /* Reset manipulator */
        _manipulator->resetPositions();
        _manipulator->resetVelocities();
        _gripperDesiredPosition = 0.;

        /* Reset boxes */
        /* Red box */
        _redBoxSkel->resetVelocities();
        _redBoxSkel->setPositions(_redInitPosition);
        /* Green box */
        _greenBoxSkel->resetVelocities();
        _greenBoxSkel->setPositions(_greenInitPosition);
        /* Blue box */
        _blueBoxSkel->resetVelocities();
        _blueBoxSkel->setPositions(_blueInitPosition);
    }
    else return;

    event.setAccepted();
}

void DartExample::updateGraphics() {
    /* We refresh the graphical models at 60Hz */
    _dartWorld->refresh();

    for(auto& object : _dartWorld->updatedShapeObjects()){
        MaterialData mat;
        mat._ambientColor = object.get().drawData().materials[0].ambientColor().rgb();
        mat._diffuseColor = object.get().drawData().materials[0].diffuseColor().rgb();
        mat._specularColor = object.get().drawData().materials[0].specularColor().rgb();
        mat._shininess = object.get().drawData().materials[0].shininess();
        mat._scaling = object.get().drawData().scaling;

        GL::Mesh* mesh = &object.get().drawData().meshes[0];

        auto it = _coloredObjects.insert(std::make_pair(&object.get(), nullptr));
        if(it.second){
            auto coloredObj = new ColoredObject(mesh, mat, static_cast<Object3D*>(&(object.get().object())), &_drawables);
            if(object.get().shapeNode()->getShape()->getType() == dart::dynamics::SoftMeshShape::getStaticType())
                coloredObj->setSoftBody();
            it.first->second = coloredObj;
        }
        else {
            it.first->second->setMesh(mesh).setMaterial(mat);
        }
    }

    _dartWorld->clearUpdatedShapeObjects();
}

void DartExample::updateManipulator() {
    Eigen::VectorXd forces(7);
    /* Update our model with manipulator's measured joint positions and velocities */
    _model->setPositions(_manipulator->getPositions().head(7));
    _model->setVelocities(_manipulator->getVelocities().head(7));

    if (state == 0)
    {
        /* Go to zero (home) position */
        Eigen::VectorXd q = _model->getPositions();
        Eigen::VectorXd dq = _model->getVelocities();
        forces = -_pGain * q - _dGain * dq + _manipulator->getCoriolisAndGravityForces().head(7);
    }
    else
    {
        /* Get joint velocities of manipulator */
        Eigen::VectorXd dq = _model->getVelocities();

        /* Get full Jacobian of our end-effector */
        Eigen::MatrixXd J = _model->getBodyNode("iiwa_link_ee")->getWorldJacobian();

        /* Get current state of the end-effector */
        Eigen::MatrixXd currentWorldTransformation = _model->getBodyNode("iiwa_link_ee")->getWorldTransform().matrix();
        Eigen::VectorXd currentWorldPosition = currentWorldTransformation.block(0, 3, 3, 1);
        Eigen::MatrixXd currentWorldOrientation = currentWorldTransformation.block(0, 0, 3, 3);
        Eigen::VectorXd currentWorldSpatialVelocity = _model->getBodyNode("iiwa_link_ee")->getSpatialVelocity(dart::dynamics::Frame::World(), dart::dynamics::Frame::World());

        /* Compute desired forces and torques */
        Eigen::VectorXd linearError = _desiredPosition - currentWorldPosition;
        Eigen::VectorXd desiredForces = _pLinearGain * linearError - _dLinearGain * currentWorldSpatialVelocity.tail(3);

        Eigen::VectorXd orientationError = rotation_error(_desiredOrientation, currentWorldOrientation);
        Eigen::VectorXd desiredTorques = _pOrientationGain * orientationError - _dOrientationGain * currentWorldSpatialVelocity.head(3);

        /* Combine forces and torques in one vector */
        Eigen::VectorXd tau(6);
        tau.head(3) = desiredTorques;
        tau.tail(3) = desiredForces;

        /* Compute final forces + gravity compensation + regularization */
        forces = J.transpose() * tau + _model->getCoriolisAndGravityForces() - _dRegularization * dq;
    }

    /* Compute final command signal */
    Eigen::VectorXd commands = _manipulator->getCommands();
    commands.setZero();
    commands.head(7) = forces;

    /* Compute command for gripper */
    commands[7] = _pGripperGain * (_gripperDesiredPosition - _manipulator->getPosition(7));
    _manipulator->setCommands(commands);
}

ColoredObject::ColoredObject(GL::Mesh* mesh, const MaterialData& material, Object3D* parent, SceneGraph::DrawableGroup3D* group):
    Object3D{parent}, SceneGraph::Drawable3D{*this, group},
    _mesh{mesh}, _shader{ViewerResourceManager::instance().get<Shaders::Phong>("color")}, _material(material) {}

void ColoredObject::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    Matrix4 scalingMatrix = Matrix4::scaling(_material._scaling);
    _shader->setAmbientColor(_material._ambientColor)
        .setDiffuseColor(_material._diffuseColor)
        .setSpecularColor(_material._specularColor)
        .setShininess(_material._shininess)
        .setLightPosition(0, camera.cameraMatrix().transformPoint({0.f, 2.f, 3.f}))
        .setLightPosition(1, camera.cameraMatrix().transformPoint({0.f, -2.f, 3.f}))
        .setTransformationMatrix(transformationMatrix * scalingMatrix)
        .setNormalMatrix((transformationMatrix * scalingMatrix).rotation())
        .setProjectionMatrix(camera.projectionMatrix());

    if(_isSoftBody)
        GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);
    _mesh->draw(*_shader);
    if(_isSoftBody)
        GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
}


ColoredObject& ColoredObject::setMesh(GL::Mesh* mesh){
    _mesh = mesh;
    return *this;
}
ColoredObject& ColoredObject::setMaterial(const MaterialData& material){
    _material = material;
    return *this;
}
ColoredObject& ColoredObject::setSoftBody(bool softBody){
    _isSoftBody = softBody;
    return *this;
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::DartExample)