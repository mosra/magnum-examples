#include <array>
#include <PluginManager/PluginManager.h>
#include <Math/Constants.h>
#include <Math/Algorithms/GramSchmidt.h>
#include <DefaultFramebuffer.h>
#include <Renderer.h>
#include <Timeline.h>
#include <Platform/GlutApplication.h>
#include <SceneGraph/AnimableGroup.h>
#include <SceneGraph/Camera3D.h>
#include <SceneGraph/MatrixTransformation3D.h>
#include <SceneGraph/Scene.h>
#include <Trade/AbstractImporter.h>

#include "Quad.h"
#include "Ducks.h"

#include "configure.h"

namespace Magnum { namespace Examples {

class Pool: public Platform::GlutApplication {
    public:
        Pool(int& argc, char** argv);

    protected:
        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;
        void keyPressEvent(KeyEvent& event) override;

    private:
        Corrade::PluginManager::PluginManager<Trade::AbstractImporter> manager;
        Timeline timeline;
        Scene3D scene;
        SceneGraph::DrawableGroup3D<> drawables;
        SceneGraph::AnimableGroup3D<> animables;
        Object3D* cameraObject;
        SceneGraph::Camera3D<>* camera;
        Ducks* ducks;
};

Pool::Pool(int& argc, char** argv): Platform::GlutApplication(argc, argv, "Pool"), manager(PLUGIN_IMPORTER_DIR), ducks(nullptr) {
    Renderer::setFeature(Renderer::Feature::DepthTest, true);
    Renderer::setClearColor({0.9f, 0.95f, 1.0f});

    /* Every scene needs a camera */
    (cameraObject = new Object3D(&scene))
        ->translate({0.0f, 0.0f, 3.5f})
        ->rotateX(deg(-15.0f))
        ->rotateY(deg(25.0f));
    (camera = new SceneGraph::Camera3D<>(cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        ->setPerspective(deg(35.0f), 1.0f, 0.1f, 100.0f);

    /* Light */
    std::array<Point3D, PoolShader::LightCount> lights{{
        Vector3::yAxis(-3.0f),
        Vector3::yAxis(2.5f)
    }};

    /* Add quad and duck */
    new Quad(&manager, lights, &scene, &drawables, &animables);
    ducks = Ducks::tryCreate(&manager, lights[1], &scene, &drawables, &animables);

    timeline.setMinimalFrameTime(1/60.0f);
    timeline.start();
}

void Pool::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});

    camera->setViewport(size);
}

void Pool::drawEvent() {
    defaultFramebuffer.clear(DefaultFramebuffer::Clear::Color|DefaultFramebuffer::Clear::Depth);

    animables.step(timeline.previousFrameTime(), timeline.previousFrameDuration());
    camera->draw(drawables);

    swapBuffers();
    timeline.nextFrame();
    redraw();
}

void Pool::keyPressEvent(KeyEvent& event) {
    /* Fix floating-point drifting */
    cameraObject->setTransformation(Matrix4::from(Math::Algorithms::gramSchmidt(
        cameraObject->transformation().rotationScaling()),
        cameraObject->transformation().translation()));

    if(event.key() == KeyEvent::Key::Left)
        cameraObject->rotateY(deg(5.0f));
    else if(event.key() == KeyEvent::Key::Right)
        cameraObject->rotateY(deg(-5.0f));

    else if(event.key() == KeyEvent::Key::Up)
        cameraObject->rotate(deg(-5.0f), cameraObject->transformation().right());
    else if(event.key() == KeyEvent::Key::Down)
        cameraObject->rotate(deg(5.0f), cameraObject->transformation().right());

    else if(event.key() == KeyEvent::Key::PageUp)
        cameraObject->translate(Vector3::zAxis(-0.2f), SceneGraph::TransformationType::Local);
    else if(event.key() == KeyEvent::Key::PageDown)
        cameraObject->translate(Vector3::zAxis(0.2f), SceneGraph::TransformationType::Local);

    else if(event.key()== KeyEvent::Key::End)
        ducks->flip();

    event.setAccepted();
    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::Pool)
