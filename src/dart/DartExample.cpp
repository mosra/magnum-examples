/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 —
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
#include <Corrade/Containers/Reference.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/Directory.h>
#include <Magnum/ResourceManager.h>
#include <Magnum/DartIntegration/ConvertShapeNode.h>
#include <Magnum/DartIntegration/World.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Object.hpp>
#include <Magnum/SceneGraph/SceneGraph.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/PhongMaterialData.h>

#include "configure.h"

#if DART_MAJOR_VERSION == 6
#include <dart/utils/urdf/urdf.hpp>
typedef dart::utils::DartLoader DartLoader;
#else
#include <dart/io/urdf/urdf.hpp>
typedef dart::io::DartLoader DartLoader;
#endif

namespace Magnum { namespace Examples {

namespace {

/* Helper functions */

dart::dynamics::SkeletonPtr createBox(const std::string& name = "box", const Eigen::Vector3d& color = Eigen::Vector3d{0.8, 0.0, 0.0}) {
    /* The size of our box */
    constexpr Double boxSize = 0.06;

    /* Calculate the mass of the box */
    constexpr Double boxDensity = 260; // kg/m^3
    constexpr Double boxMass = boxDensity*Math::pow<3>(boxSize);
    /* Create a Skeleton with the given name */
    dart::dynamics::SkeletonPtr box = dart::dynamics::Skeleton::create(name);

    /* Create a body for the box */
    dart::dynamics::BodyNodePtr body = box
        ->createJointAndBodyNodePair<dart::dynamics::FreeJoint>(nullptr).second;

    /* Create a shape for the box */
    auto boxShape = std::make_shared<dart::dynamics::BoxShape>(
        Eigen::Vector3d{boxSize, boxSize, boxSize});
    auto shapeNode = body->createShapeNodeWith<dart::dynamics::VisualAspect, dart::dynamics::CollisionAspect, dart::dynamics::DynamicsAspect>(boxShape);
    shapeNode->getVisualAspect()->setColor(color);

    /* Set up inertia for the box */
    dart::dynamics::Inertia inertia;
    inertia.setMass(boxMass);
    inertia.setMoment(boxShape->computeInertia(boxMass));
    body->setInertia(inertia);

    /* Setup the center of the box properly */
    box->getDof("Joint_pos_z")->setPosition(boxSize/2.0);

    return box;
}

dart::dynamics::SkeletonPtr createFloor() {
    /* Create a floor */
    dart::dynamics::SkeletonPtr floorSkel = dart::dynamics::Skeleton::create("floor");
    /* Give the floor a body */
    dart::dynamics::BodyNodePtr body =
        floorSkel->createJointAndBodyNodePair<dart::dynamics::WeldJoint>(nullptr).second;

    /* Give the floor a shape */
    constexpr double floorWidth = 10.0;
    constexpr double floorHeight = 0.1;
    auto box = std::make_shared<dart::dynamics::BoxShape>(
        Eigen::Vector3d{floorWidth, floorWidth, floorHeight});
    auto shapeNode = body->createShapeNodeWith<dart::dynamics::VisualAspect, dart::dynamics::CollisionAspect, dart::dynamics::DynamicsAspect>(box);
    shapeNode->getVisualAspect()->setColor(Eigen::Vector3d(0.3, 0.3, 0.4));

    /* Put the floor into position */
    Eigen::Isometry3d tf(Eigen::Isometry3d::Identity());
    tf.translation() = Eigen::Vector3d{0.0, 0.0, -floorHeight/2.0};
    body->getParentJoint()->setTransformFromParentBodyNode(tf);

    return floorSkel;
}

}

using namespace Magnum::Math::Literals;

typedef ResourceManager<GL::Buffer, GL::Mesh, Shaders::Phong> ViewerResourceManager;
typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

struct MaterialData{
    Vector3 ambientColor,
        diffuseColor,
        specularColor;
    Float shininess;
    Vector3 scaling;
};

class DrawableObject: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit DrawableObject(ViewerResourceManager& resourceManager, std::vector<Containers::Reference<GL::Mesh>>&& meshes, std::vector<MaterialData>&& materials, Object3D* parent, SceneGraph::DrawableGroup3D* group);

        DrawableObject& setMeshes(std::vector<Containers::Reference<GL::Mesh>>&& meshes){
            _meshes = std::move(meshes);
            return *this;
        }

        DrawableObject& setMaterials(std::vector<MaterialData>&& materials) {
            _materials = std::move(materials);
            return *this;
        }

        DrawableObject& setSoftBodies(std::vector<bool>&& softBody) {
            _isSoftBody = std::move(softBody);
            return *this;
        }

        DrawableObject& setTextures(std::vector<GL::Texture2D*>&& textures) {
            _textures = std::move(textures);
            return *this;
        }

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Resource<Shaders::Phong> _colorShader;
        Resource<Shaders::Phong> _textureShader;
        std::vector<Containers::Reference<GL::Mesh>> _meshes;
        std::vector<MaterialData> _materials;
        std::vector<bool> _isSoftBody;
        std::vector<GL::Texture2D*> _textures;
};

class DartExample: public Platform::Application {
    public:
        /* home: go to home position, active: go to active box */
        enum class State { Home, Active };

        explicit DartExample(const Arguments& arguments);

    private:
        void viewportEvent(ViewportEvent& event) override;
        void drawEvent() override;
        void keyPressEvent(KeyEvent& event) override;

        void updateManipulator();

        ViewerResourceManager _resourceManager;

        Scene3D _scene;
        SceneGraph::DrawableGroup3D _drawables;
        SceneGraph::Camera3D* _camera;

        Object3D *_cameraRig, *_cameraObject;

        /* DART */
        std::unique_ptr<DartIntegration::World> _dartWorld;
        std::unordered_map<DartIntegration::Object*, DrawableObject*> _drawableObjects;
        std::vector<Object3D*> _dartObjs;
        dart::dynamics::SkeletonPtr _manipulator, _model, _redBoxSkel, _greenBoxSkel, _blueBoxSkel;
        dart::simulation::WorldPtr _world;
        Eigen::VectorXd _redInitPosition, _greenInitPosition, _blueInitPosition;

        /* DART control */
        Eigen::VectorXd _desiredPosition;
        Eigen::MatrixXd _desiredOrientation;
        Double _gripperDesiredPosition;
        const Double _pGripperGain = 100.0;
        const Double _pLinearGain = 500.0;
        const Double _dLinearGain = 200.0;
        const Double _pOrientationGain = 100.0;
        const Double _dOrientationGain = 50.0;
        const Double _dRegularization = 10.0;
        const Double _pGain = 2.25;
        const Double _dGain = 5.0;

        /* Simple State Machine */
        State _state = State::Home;
};

DartExample::DartExample(const Arguments& arguments): Platform::Application{arguments, NoCreate} {
    /* Try to locate the models, first in the source directory, then in the
       installation directory and as a fallback next to the executable */
    std::string resPath;
    if(Utility::Directory::exists(DART_EXAMPLE_DIR))
        resPath = DART_EXAMPLE_DIR;
    else if(Utility::Directory::exists(DART_EXAMPLE_INSTALL_DIR))
        resPath = DART_EXAMPLE_INSTALL_DIR;
    else
        resPath = Utility::Directory::join(Utility::Directory::path(Utility::Directory::executableLocation()), "urdf");

    /* Finally, provide a way for the user to override the model directory */
    Utility::Arguments args;
    args.addFinalOptionalArgument("urdf", resPath)
            .setHelp("urdf", "directory where is iiwa14_simple.urdf")
        .addSkippedPrefix("magnum", "engine-specific options")
        .setGlobalHelp("Controls a robotic manipulator with DART")
        .parse(arguments.argc, arguments.argv);
    /* DART can't handle relative paths, so prepend CWD to them if needed */
    resPath = Utility::Directory::join(Utility::Directory::current(), args.value("urdf"));

    /* Try 8x MSAA */
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
        .setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.001f, 100.0f))
        .setViewport(GL::defaultFramebuffer.viewport().size());
    /* DART has +Z-axis as up direction*/
    _cameraObject->setTransformation(Matrix4::lookAt(
        {0.0f, 3.0f, 1.5f}, {0.0f, 0.0f, 0.5f}, {0.0f, 0.0f, 1.0f}));

    /* DART: Load Skeletons/Robots */
    DartLoader loader;
    /* Add packages (needed for URDF loading) */
    loader.addPackageDirectory("iiwa14", Utility::Directory::join(resPath, "iiwa14"));
    loader.addPackageDirectory("robotiq", Utility::Directory::join(resPath, "robotiq"));
    std::string filename = Utility::Directory::join(resPath, "iiwa14_simple.urdf");

    /* First load the KUKA manipulator */
    _manipulator = loader.parseSkeleton(filename);
    if(!_manipulator) {
        Error{} << "Failed to load" << filename << Debug::nospace << ", exiting.";
        Error{} << "Use the --urdf option to specify where this file is located.";
        exit(1);
        return;
    }
    for(std::size_t i = 0; i < _manipulator->getNumJoints(); ++i)
        _manipulator->getJoint(i)->setPositionLimitEnforced(true);

    _manipulator->disableSelfCollisionCheck();

    /* Clone the KUKA manipulator to use it as model in a model-based controller */
    #if DART_VERSION_AT_LEAST(6, 7, 2)
    _model = _manipulator->cloneSkeleton();
    #else
    _model = _manipulator->clone();
    #endif

    /* Load the Robotiq 2-finger gripper */
    filename = Utility::Directory::join(resPath, "robotiq.urdf");
    auto gripper_skel = loader.parseSkeleton(filename);
    if(!gripper_skel) {
        Error{} << "Failed to load" << filename << Debug::nospace << ", exiting.";
        Error{} << "Use the --urdf option to specify where this file is located.";
        exit(1);
        return;
    }
    /* The gripper is controlled in velocity mode: servo actuator */
    gripper_skel->getJoint("finger_joint")->setActuatorType(dart::dynamics::Joint::SERVO);

    /* Attach gripper to manipulator */
    dart::dynamics::WeldJoint::Properties properties = dart::dynamics::WeldJoint::Properties();
    properties.mT_ChildBodyToJoint.translation() = Eigen::Vector3d(0, 0, 0.0001);
    gripper_skel->getRootBodyNode()->moveTo<dart::dynamics::WeldJoint>(_manipulator->getBodyNode("iiwa_link_ee"), properties);

    /* Create floor */
    auto floorSkel = createFloor();

    /* Create red box */
    _redBoxSkel = createBox("red", Eigen::Vector3d{0.8, 0.0, 0.0});
    /* Position it at (0.5, 0.) [x,y] */
    _redBoxSkel->setPosition(3, 0.5);
    /* Save initial position for resetting */
    _redInitPosition = _redBoxSkel->getPositions();
    /* Create green box */
    _greenBoxSkel = createBox("green", Eigen::Vector3d{0.0, 0.8, 0.0});
    /* Position it at (0.5, 0.2) */
    _greenBoxSkel->setPosition(3, 0.5);
    _greenBoxSkel->setPosition(4, 0.2);
    /* Save initial position for resetting */
    _greenInitPosition = _greenBoxSkel->getPositions();
    /* Create blue box */
    _blueBoxSkel = createBox("blue", Eigen::Vector3d{0.0, 0.0, 0.8});
    /* Position it at (0.5, -0.2) */
    _blueBoxSkel->setPosition(3, 0.5);
    _blueBoxSkel->setPosition(4, -0.2);
    /* Save initial position for resetting */
    _blueInitPosition = _blueBoxSkel->getPositions();

    /* Create the DART world */
    _world = dart::simulation::WorldPtr{new dart::simulation::World};
    /* Use a simple but very fast collision detector. This plays the most
       important role for having a fast simulation. */
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
    _dartWorld.reset(new DartIntegration::World{*dartObj, *_world});

    /* Phong shader instances */
    _resourceManager.set("color", new Shaders::Phong{{}, 2});
    _resourceManager.set("texture", new Shaders::Phong(Shaders::Phong::Flag::DiffuseTexture, 2));

    /* Loop at 60 Hz max */
    setSwapInterval(1);
    setMinimalLoopPeriod(16);

    redraw();
}

void DartExample::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});

    _camera->setViewport(event.framebufferSize());
}

void DartExample::drawEvent() {
    GL::defaultFramebuffer.clear(
        GL::FramebufferClear::Color|GL::FramebufferClear::Depth);
    /* We want around 60Hz display rate */
    UnsignedInt steps = Math::round(1.0/(60.0*_world->getTimeStep()));

    /* Step DART simulation */
    for(UnsignedInt i = 0; i < steps; ++i) {
        /* Compute control signals for manipulator */
        updateManipulator();
        /* Step the simulated world */
        _dartWorld->step();
    }

    /* Update graphic meshes/materials and render */
    _dartWorld->refresh();

    /* For each updated object */
    for(DartIntegration::Object& object : _dartWorld->updatedShapeObjects()) {
        /* Get material information */
        std::vector<MaterialData> materials;
        std::vector<Containers::Reference<GL::Mesh>> meshes;
        std::vector<bool> isSoftBody;
        std::vector<GL::Texture2D*> textures;
        for(std::size_t i = 0; i < object.drawData().meshes.size(); ++i) {
            bool isColor = true;
            GL::Texture2D* texture = nullptr;
            if(object.drawData().materials[i].hasAttribute(Trade::MaterialAttribute::DiffuseTexture)) {
                Containers::Optional<GL::Texture2D>& entry = object.drawData().textures[object.drawData().materials[i].diffuseTexture()];
                if(entry) {
                    texture = &*entry;
                    isColor = false;
                }
            }

            textures.push_back(texture);

            MaterialData mat;
            mat.ambientColor = object.drawData().materials[i].ambientColor().rgb();
            if(isColor)
                mat.diffuseColor = object.drawData().materials[i].diffuseColor().rgb();
            mat.specularColor = object.drawData().materials[i].specularColor().rgb();
            mat.shininess = object.drawData().materials[i].shininess();
            mat.scaling = object.drawData().scaling;

            /* Get the modified mesh */
            meshes.push_back(object.drawData().meshes[i]);
            materials.push_back(mat);
            isSoftBody.push_back(object.shapeNode()->getShape()->getType() ==
                dart::dynamics::SoftMeshShape::getStaticType());
        }

        /* Check if we already have it and then either add a new one or update
           the existing. We don't need the mesh / material / texture vectors
           anywhere else anymore, so move them in to avoid copies. */
        auto it = _drawableObjects.insert(std::make_pair(&object, nullptr));
        if(it.second) {
            auto drawableObj = new DrawableObject{_resourceManager,
                std::move(meshes), std::move(materials),
                static_cast<Object3D*>(&(object.object())), &_drawables};
            drawableObj->setSoftBodies(std::move(isSoftBody));
            drawableObj->setTextures(std::move(textures));
            it.first->second = drawableObj;
        } else {
            (*it.first->second)
                .setMeshes(std::move(meshes))
                .setMaterials(std::move(materials))
                .setSoftBodies(std::move(isSoftBody))
                .setTextures(std::move(textures));
        }
    }

    _dartWorld->clearUpdatedShapeObjects();

    _camera->draw(_drawables);

    swapBuffers();
    redraw();
}

void DartExample::keyPressEvent(KeyEvent& event) {
    if(event.key() == KeyEvent::Key::Down) {
        _cameraObject->rotateX(5.0_degf);
    } else if(event.key() == KeyEvent::Key::Up) {
        _cameraObject->rotateX(-5.0_degf);
    } else if(event.key() == KeyEvent::Key::Left) {
        _cameraRig->rotateY(-5.0_degf);
    } else if(event.key() == KeyEvent::Key::Right) {
        _cameraRig->rotateY(5.0_degf);
    } else if(event.key() == KeyEvent::Key::C) {
        _gripperDesiredPosition = 0.3;
    } else if(event.key() == KeyEvent::Key::O) {
        _gripperDesiredPosition = 0.;
    } else if(event.key() == KeyEvent::Key::U && _state == State::Active) {
        _desiredPosition[2] += 0.1;
    } else if(event.key() == KeyEvent::Key::D && _state == State::Active) {
        _desiredPosition[2] -= 0.1;
    } else if(event.key() == KeyEvent::Key::Z && _state == State::Active) {
        _desiredPosition[1] += 0.2;
    } else if(event.key() == KeyEvent::Key::X && _state == State::Active) {
        _desiredPosition[1] -= 0.2;
    } else if(event.key() == KeyEvent::Key::R) {
        _desiredPosition = _redBoxSkel->getPositions().tail(3);
        _desiredPosition[2] += 0.25;
        _state = State::Active;
    } else if(event.key() == KeyEvent::Key::G) {
        _desiredPosition = _greenBoxSkel->getPositions().tail(3);
        _desiredPosition[2] += 0.25;
        _state = State::Active;
    } else if(event.key() == KeyEvent::Key::B) {
        _desiredPosition = _blueBoxSkel->getPositions().tail(3);
        _desiredPosition[2] += 0.25;
        _state = State::Active;
    } else if(event.key() == KeyEvent::Key::H) {
        _state = State::Home;
    } else if(event.key() == KeyEvent::Key::Space) {
        /* Reset _state machine */
        _state = State::Home;
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
    } else return;

    event.setAccepted();
}

void DartExample::updateManipulator() {
    Eigen::VectorXd forces(7);
    /* Update our model with manipulator's measured joint positions and velocities */
    _model->setPositions(_manipulator->getPositions().head(7));
    _model->setVelocities(_manipulator->getVelocities().head(7));

    if(_state == State::Home) {
        /* Go to zero (home) position */
        Eigen::VectorXd q = _model->getPositions();
        Eigen::VectorXd dq = _model->getVelocities();
        forces = -_pGain * q - _dGain * dq + _manipulator->getCoriolisAndGravityForces().head(7);
    } else {
        /* Get joint velocities of manipulator */
        Eigen::VectorXd dq = _model->getVelocities();

        /* Get full Jacobian of our end-effector */
        Eigen::MatrixXd J = _model->getBodyNode("iiwa_link_ee")->getWorldJacobian();

        /* Get current _state of the end-effector */
        Eigen::MatrixXd currentWorldTransformation = _model->getBodyNode("iiwa_link_ee")->getWorldTransform().matrix();
        Eigen::VectorXd currentWorldPosition = currentWorldTransformation.block(0, 3, 3, 1);
        Eigen::MatrixXd currentWorldOrientation = currentWorldTransformation.block(0, 0, 3, 3);
        Eigen::VectorXd currentWorldSpatialVelocity = _model->getBodyNode("iiwa_link_ee")->getSpatialVelocity(dart::dynamics::Frame::World(), dart::dynamics::Frame::World());

        /* Compute desired forces and torques */
        Eigen::VectorXd linearError = _desiredPosition - currentWorldPosition;
        Eigen::VectorXd desiredForces = _pLinearGain * linearError - _dLinearGain * currentWorldSpatialVelocity.tail(3);

        Eigen::VectorXd orientationError = dart::math::logMap(_desiredOrientation * currentWorldOrientation.transpose());
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

DrawableObject::DrawableObject(ViewerResourceManager& resourceManager, std::vector<Containers::Reference<GL::Mesh>>&& meshes, std::vector<MaterialData>&& materials,
Object3D* parent, SceneGraph::DrawableGroup3D* group):
    Object3D{parent}, SceneGraph::Drawable3D{*this, group},
    _colorShader{resourceManager.get<Shaders::Phong>("color")},
    _textureShader{resourceManager.get<Shaders::Phong>("texture")},
    _meshes{std::move(meshes)},
    _materials{std::move(materials)}
{
    CORRADE_INTERNAL_ASSERT(_materials.size() >= meshes.size());
    _isSoftBody.resize(_meshes.size(), false);
    _textures.resize(_meshes.size());
}

void DrawableObject::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    for(std::size_t i = 0; i < _meshes.size(); ++i) {
        GL::Mesh& mesh = _meshes[i];
        Matrix4 scalingMatrix = Matrix4::scaling(_materials[i].scaling);

        if(_isSoftBody[i])
            GL::Renderer::disable(GL::Renderer::Feature::FaceCulling);

        if(!_textures[i]) {
            (*_colorShader)
                .setAmbientColor(_materials[i].ambientColor)
                .setDiffuseColor(_materials[i].diffuseColor)
                .setSpecularColor(_materials[i].specularColor)
                .setShininess(_materials[i].shininess)
                .setLightPositions({
                    {camera.cameraMatrix().transformPoint({0.0f, 2.0f, 3.0f}), 0.0f},
                    {camera.cameraMatrix().transformPoint({0.0f, -2.0f, 3.0f}), 0.0f}
                })
                .setTransformationMatrix(transformationMatrix*scalingMatrix)
                .setNormalMatrix((transformationMatrix*scalingMatrix).normalMatrix())
                .setProjectionMatrix(camera.projectionMatrix())
                .draw(mesh);
        } else {
            (*_textureShader)
                .setAmbientColor(_materials[i].ambientColor)
                .bindDiffuseTexture(*_textures[i])
                .setSpecularColor(_materials[i].specularColor)
                .setShininess(_materials[i].shininess)
                .setLightPositions({
                    {camera.cameraMatrix().transformPoint({0.0f, 2.0f, 3.0f}), 0.0f},
                    {camera.cameraMatrix().transformPoint({0.0f, -2.0f, 3.0f}), 0.0f}
                })
                .setTransformationMatrix(transformationMatrix*scalingMatrix)
                .setNormalMatrix((transformationMatrix*scalingMatrix).normalMatrix())
                .setProjectionMatrix(camera.projectionMatrix())
                .draw(mesh);
        }

        if(_isSoftBody[i])
            GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);
    }
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::DartExample)
