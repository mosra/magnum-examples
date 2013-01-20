#include <Math/Constants.h>
#include <Math/Quaternion.h>
#include <DefaultFramebuffer.h>
#include <Renderer.h>
#include <DebugTools/ObjectRenderer.h>
#include <DebugTools/ResourceManager.h>
#include <Platform/GlutApplication.h>
#include <SceneGraph/Camera3D.h>
#include <SceneGraph/MatrixTransformation3D.h>
#include <SceneGraph/Scene.h>

namespace Magnum { namespace Examples {

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D<>> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D<>> Scene3D;

class Interpolation: public Platform::GlutApplication {
    public:
        explicit Interpolation(int& argc, char** argv);

        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;
        void keyPressEvent(KeyEvent& event) override;

    private:
        DebugTools::ResourceManager manager;
        Scene3D scene;
        SceneGraph::DrawableGroup3D<> drawables;
        SceneGraph::Camera3D<>* camera;

        Object3D *aObject, *bObject, *lerp, *slerp;
        Quaternion a, b;
        GLfloat t;
};

Interpolation::Interpolation(int& argc, char** argv): GlutApplication(argc, argv, "Transformation interpolation techniques"), t(0.0f) {
    Renderer::setClearColor(Color3<>(0.15f));

    /* Object renderer configuration */
    manager.set("small", (new DebugTools::ObjectRendererOptions)->setSize(0.5f));
    manager.set("medium", (new DebugTools::ObjectRendererOptions)->setSize(1.0f));
    manager.set("large", (new DebugTools::ObjectRendererOptions)->setSize(1.2f));

    /* Configure camera */
    Object3D* cameraObject = new Object3D(&scene);
    cameraObject->translate(Vector3::zAxis(10.0f))
        ->rotateX(deg(-30.0f));
    (camera = new SceneGraph::Camera3D<>(cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        ->setPerspective(deg(35.0f), 1.0f, 0.001f, 100.0f);

    /* Initial object rotations */
    a = Quaternion::fromRotation(deg(23.0f), Vector3(0.3f, -1.0f, 2.5f).normalized());
    b = Quaternion::fromRotation(deg(119.0f), Vector3(-0.8f, 1.2f, -2.5f).normalized());

    /* Objects at corner positions */
    (aObject = new Object3D(&scene))
        ->setTransformation(Matrix4::from(a.matrix(), Vector3::xAxis(-2.5f)));
    new DebugTools::ObjectRenderer3D(aObject, "small", &drawables);

    (bObject = new Object3D(&scene))
        ->setTransformation(Matrix4::from(b.matrix(), Vector3::xAxis(2.5f)));
    new DebugTools::ObjectRenderer3D(bObject, "small", &drawables);

    /* Interpolated objects */
    (lerp = new Object3D(&scene))
        ->setTransformation(aObject->transformation());
    new DebugTools::ObjectRenderer3D(lerp, "medium", &drawables);

    (slerp = new Object3D(&scene))
        ->setTransformation(aObject->transformation());
    new DebugTools::ObjectRenderer3D(slerp, "large", &drawables);
}

void Interpolation::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});

    camera->setViewport(size);
}

void Interpolation::drawEvent() {
    defaultFramebuffer.clear(DefaultFramebuffer::Clear::Color);

    camera->draw(drawables);

    swapBuffers();
}

void Interpolation::keyPressEvent(KeyEvent& event) {
    if(event.key() == KeyEvent::Key::Left)
        t -= 0.05f;
    else if(event.key() == KeyEvent::Key::Right)
        t += 0.05f;
    else return;

    t = Math::clamp(t, 0.0f, 1.0f);

    Vector3 translationLerp = Vector3::lerp(aObject->transformation().translation(),
                                            bObject->transformation().translation(), t);
    lerp->setTransformation(Matrix4::from(Quaternion::lerp(a, b, t).matrix(), translationLerp));
    slerp->setTransformation(Matrix4::from(Quaternion::slerp(a, b, t).matrix(), translationLerp));

    event.setAccepted();
    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::Interpolation)
