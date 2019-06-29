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

namespace {
const double default_domino_height = 0.25;
const double default_domino_width = 0.4 * default_domino_height;
const double default_domino_depth = default_domino_width / 5.0;

const double default_domino_density = 260; // kg/m^3
const double default_domino_mass =
    default_domino_density
    * default_domino_height
    * default_domino_width
    * default_domino_depth;

inline dart::dynamics::SkeletonPtr createDomino()
{
    /* Create a Skeleton with the name "domino" */
    dart::dynamics::SkeletonPtr domino = dart::dynamics::Skeleton::create("domino");

    /* Create a body for the domino */
    dart::dynamics::BodyNodePtr body =
        domino->createJointAndBodyNodePair<dart::dynamics::FreeJoint>(nullptr).second;

    /* Create a shape for the domino */
    std::shared_ptr<dart::dynamics::BoxShape> box(
        new dart::dynamics::BoxShape(Eigen::Vector3d(default_domino_depth,
                                        default_domino_width,
                                        default_domino_height)));
    auto shapeNode = body->createShapeNodeWith<dart::dynamics::VisualAspect, dart::dynamics::CollisionAspect, dart::dynamics::DynamicsAspect>(box);
    shapeNode->getVisualAspect()->setColor(Eigen::Vector3d(0.6, 0.6, 0.6));

    /* Set up inertia for the domino */
    dart::dynamics::Inertia inertia;
    inertia.setMass(default_domino_mass);
    inertia.setMoment(box->computeInertia(default_domino_mass));
    body->setInertia(inertia);

    domino->getDof("Joint_pos_z")->setPosition(default_domino_height / 2.0);

    return domino;
}}

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
        void updateManipulator(dart::dynamics::SkeletonPtr skel);
        dart::dynamics::SkeletonPtr createNewDomino(Eigen::Vector6d position, double angle, double& totalAngle);

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
        size_t _dominoId;
        dart::dynamics::SkeletonPtr _manipulator, _dominoSkel;
        dart::simulation::WorldPtr _world;

        /* DART control */
        Eigen::VectorXd _desiredPositions;
        Eigen::MatrixXd _desired_transform;
        const double _pGain = 200.0;
        const double _dGain = 20.0;
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
        // ->translate({0.f, 4.f, 0.f});
    (_cameraObject = new Object3D(_cameraRig));
        // ->translate({0.f, 0.f, 20.f});
    (_camera = new SceneGraph::Camera3D(*_cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(Deg(35.0f), 1.0f, 0.001f, 100.0f))
        .setViewport(GL::defaultFramebuffer.viewport().size());
    // _cameraObject->setTransformation(Magnum::Matrix4::lookAt(Vector3{0.f, 15.f, 2.f}, Vector3{0.f, 0.f, 0.f}, Vector3{0.f, 0.f, 1.f}));
    _cameraObject->setTransformation(Magnum::Matrix4::lookAt(Vector3{0.f, 3.f, 1.f}, Vector3{0.f, 0.f, 0.5f}, Vector3{0.f, 0.f, 1.f}));

    /* DART: Load Skeleton */
    DartLoader loader;
    loader.addPackageDirectory("iiwa_description", std::string(DARTEXAMPLE_DIR) + "/urdf/");
    loader.addPackageDirectory("robotiq_arg85_description", std::string(DARTEXAMPLE_DIR) + "/urdf/");
    std::string filename = std::string(DARTEXAMPLE_DIR) + "/urdf/iiwa14_simple.urdf";
    _manipulator = loader.parseSkeleton(filename);
    for(size_t i = 0; i < _manipulator->getNumJoints(); i++)
        _manipulator->getJoint(i)->setPositionLimitEnforced(true);
    // _manipulator->setPosition(7, 0.04);
    _manipulator->enableSelfCollisionCheck();

    filename = std::string(DARTEXAMPLE_DIR) + "/urdf/robotiq.urdf";
    auto gripper_skel = loader.parseSkeleton(filename);
    dart::dynamics::WeldJoint::Properties properties = dart::dynamics::WeldJoint::Properties();
    properties.mT_ChildBodyToJoint.translation() = Eigen::Vector3d(0, 0, 0);

    // gripper_skel->getRootBodyNode()->moveTo<dart::dynamics::WeldJoint>(_manipulator->getBodyNode("iiwa_link_ee"), properties);
    // _manipulator->setPosition(7, 0.7);

    // /* Position its base in a reasonable way */
    // Eigen::Isometry3d tf2 = Eigen::Isometry3d::Identity();
    // tf2.translation() = Eigen::Vector3d(-1.25, 0.0, 0.0);
    // _manipulator->getJoint(0)->setTransformFromParentBodyNode(tf2);

    /* Get it into a useful configuration */
    // _manipulator->getDof(1)->setPosition(M_PI / 3. + 0.2);
    // _manipulator->getDof(3)->setPosition(-M_PI / 4.);
    // _manipulator->getDof(5)->setPosition(M_PI / 4.);
    _manipulator->disableSelfCollisionCheck();
    // Debug{} << _manipulator->getBodyNode("iiwa_link_ee")->getWorldTransform().matrix();
    _desired_transform = Eigen::MatrixXd(4,4);
    _desired_transform << -0.948097, -7.64642e-12, 0.317981, 0.796367,
                         -8.07922e-12, 1, -4.22101e-14,  5.64684e-12,
                         -0.317981,-2.60896e-12,-0.948097, 0.195868,
                          0, 0, 0, 1;

    _desiredPositions = _manipulator->getPositions();
    // _desiredPositions[7] = 0.7;

    /* Create a floor */
    dart::dynamics::SkeletonPtr floorSkel = dart::dynamics::Skeleton::create("floor");
    /* Give the floor a body */
    dart::dynamics::BodyNodePtr body =
        floorSkel->createJointAndBodyNodePair<dart::dynamics::WeldJoint>(nullptr).second;

    /* Give the floor a shape */
    double floor_width = 10.0;
    double floor_height = 0.1;
    std::shared_ptr<dart::dynamics::BoxShape> box(
            new dart::dynamics::BoxShape(Eigen::Vector3d(floor_width, floor_width, floor_height)));
    auto shapeNode
        = body->createShapeNodeWith<dart::dynamics::VisualAspect, dart::dynamics::CollisionAspect, dart::dynamics::DynamicsAspect>(box);
    shapeNode->getVisualAspect()->setColor(Eigen::Vector3d(0.3, 0.3, 0.4));

    /* Put the floor into position */
    Eigen::Isometry3d tf(Eigen::Isometry3d::Identity());
    tf.translation() = Eigen::Vector3d(0.0, 0.0, -floor_height / 2.0);
    body->getParentJoint()->setTransformFromParentBodyNode(tf);

    /* Create first domino */
    _dominoSkel = createDomino();

    _world = dart::simulation::WorldPtr(new dart::simulation::World);
    _world->getConstraintSolver()->setCollisionDetector(dart::collision::DARTCollisionDetector::create());
    _world->addSkeleton(_manipulator);
    _world->addSkeleton(floorSkel);
    // _world->addSkeleton(_dominoSkel);

    // /* faster simulation step; less accurate simulation/collision detection */
    // _world->setTimeStep(0.015);
    /* slower simulation step; better accuracy */
    _world->setTimeStep(0.001);

    auto dartObj = new Object3D{&_scene};
    _dartWorld.reset(new DartIntegration::World(*dartObj, *_world));

    /* Phong shader instance */
    _resourceManager.set("color", new Shaders::Phong);

    /* Add more dominoes for fun! */
    const double default_angle = 20 * M_PI / 180.0;

    Eigen::Vector6d positions = _dominoSkel->getPositions();
    double totalAngle = 0.;
    _dominoId = 0;

    for (int i = 0; i < 10; i++) {
        auto domino = createNewDomino(positions, default_angle, totalAngle);
        // _world->addSkeleton(domino);
        positions = domino->getPositions();
    }

    /* Loop at 60 Hz max */
    setSwapInterval(1);
    setMinimalLoopPeriod(16);
    _timeline.start();

    redraw();
}

void DartExample::viewportEvent(const Vector2i& size) {
    GL::defaultFramebuffer.setViewport({{}, size});

    _camera->setViewport(size);
}

void DartExample::drawEvent() {
    GL::defaultFramebuffer.clear(
        GL::FramebufferClear::Color|GL::FramebufferClear::Depth);
    double dt = _timeline.previousFrameDuration();
    UnsignedInt steps = std::ceil(dt / _world->getTimeStep());

    /* Step DART simulation */
    for (UnsignedInt i = 0; i < steps; i++) {
        updateManipulator(_manipulator);
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
    else if(event.key() == KeyEvent::Key::P)
    {
        _desiredPositions(3) = -M_PI / 3. + 0.2;
    }
    else return;

    event.setAccepted();
}

void DartExample::updateGraphics() {
    /* We refresh the graphical models only once per 60Hz */
    _dartWorld->refresh();

    for(auto& object : _dartWorld->updatedShapeObjects()){
        MaterialData mat;
        mat._ambientColor = object.get().drawData().materials[0].ambientColor().rgb();
        mat._diffuseColor = object.get().drawData().materials[0].diffuseColor().rgb();
        mat._specularColor = object.get().drawData().materials[0].specularColor().rgb();
        mat._shininess = object.get().drawData().materials[0].shininess();
        mat._scaling = object.get().drawData().scaling;

        // Debug{} << "Ambient: " << mat._ambientColor;
        // Debug{} << "Diffuse: " << mat._diffuseColor;
        // Debug{} << "Specular: " << mat._specularColor;
        // Debug{} << "Shininess: " << mat._shininess;

        // if(mat._ambientColor == Vector3{1.f, 1.f, 1.f}) {
        //     mat._ambientColor = Vector3{0.f, 0.f, 0.f};
        //     mat._diffuseColor = Vector3{0.6f, 0.6f, 0.6f};
        // }
        // mat._specularColor = Vector3{1.f, 1.f, 1.f};

        // if (mat._shininess < 1e-4f)
        //     mat._shininess = 80.f;

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

void DartExample::updateManipulator(dart::dynamics::SkeletonPtr skel) {
    // Eigen::VectorXd q = skel->getPositions();
    // Eigen::VectorXd dq = skel->getVelocities();

    // q += dq * skel->getTimeStep();

    // Eigen::VectorXd q_err = _desiredPositions - q;
    // Eigen::VectorXd dq_err = -dq;

    // const Eigen::MatrixXd& M = skel->getMassMatrix();
    // const Eigen::VectorXd& Cg = skel->getCoriolisAndGravityForces();

    // Eigen::VectorXd forces = M * (_pGain * q_err + _dGain * dq_err) + Cg;

    // skel->setForces(forces);
    // TO-DO: This properly
    Eigen::VectorXd desired_position = _desired_transform.block(0, 3, 3, 1);
    // desired_position[2] += 0.2;
    // desired_position[1] += 0.4;
    // desired_position[0] -= 0.1;
    Eigen::MatrixXd J = skel->getBodyNode("iiwa_link_ee")->getLinearJacobian();
    Eigen::MatrixXd pinv_J = J.transpose() * (J * J.transpose() + 0.0025 * Eigen::Matrix3d::Identity()).inverse();
    Eigen::MatrixXd mass_matrix = skel->getMassMatrix();
    Eigen::MatrixXd L = pinv_J.transpose() * mass_matrix * pinv_J;

    Eigen::MatrixXd current_tr = skel->getBodyNode("iiwa_link_ee")->getWorldTransform().matrix();;
    Eigen::VectorXd current_pos = current_tr.block(0, 3, 3, 1);
    Eigen::VectorXd current_vel = skel->getBodyNode("iiwa_link_ee")->getLinearVelocity();

    double Kp = 10.;
    double Kd = 10.;
    Eigen::VectorXd acc = Kp * (desired_position - current_pos) - Kd * current_vel;
    // Debug{} << (desired_position - current_pos).transpose();

    Eigen::VectorXd forces = J.transpose() * L * acc + skel->getCoriolisAndGravityForces();
    skel->setCommands(forces);
}

dart::dynamics::SkeletonPtr DartExample::createNewDomino(Eigen::Vector6d position, double angle, double& totalAngle)
{
    const double default_distance = default_domino_height / 2.0;
    /* Create a new domino */
#if DART_VERSION_AT_LEAST(6, 7, 2)
    dart::dynamics::SkeletonPtr newDomino = _dominoSkel->cloneSkeleton();
#else
    dart::dynamics::SkeletonPtr newDomino = _dominoSkel->clone();
#endif
    newDomino->setName("domino #" + std::to_string(_dominoId++));

    /* Compute the position for the new domino */
    Eigen::Vector3d dx = default_distance * Eigen::Vector3d(
            std::cos(totalAngle), std::sin(totalAngle), 0.0);

    Eigen::Vector6d x = position;
    x.tail<3>() += dx;

    /* Adjust the angle for the new domino */
    x[2] = totalAngle + angle;
    totalAngle += angle;

    newDomino->setPositions(x);

    return newDomino;
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
        .setLightPosition(camera.cameraMatrix().transformPoint({0.f, 2.f, 3.f}))
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