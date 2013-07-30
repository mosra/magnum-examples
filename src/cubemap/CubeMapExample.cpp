/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013 Vladimír Vondruš <mosra@centrum.cz>

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

#include <PluginManager/Manager.h>
#include <AbstractShaderProgram.h>
#include <CubeMapTexture.h>
#include <DefaultFramebuffer.h>
#include <Extensions.h>
#include <Mesh.h>
#include <Renderer.h>
#include <Texture.h>
#include <Platform/GlutApplication.h>
#include <SceneGraph/Drawable.h>
#include <SceneGraph/Scene.h>
#include <SceneGraph/Camera3D.h>
#include <Trade/AbstractImporter.h>

#include "CubeMap.h"
#include "Reflector.h"
#include "Types.h"
#include "configure.h"

namespace Magnum { namespace Examples {

class CubeMapExample: public Platform::GlutApplication {
    public:
        CubeMapExample(const Arguments& arguments);

    protected:
        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;
        void keyPressEvent(KeyEvent& event) override;

    private:
        CubeMapResourceManager resourceManager;
        Scene3D scene;
        SceneGraph::DrawableGroup3D drawables;
        Object3D* cameraObject;
        SceneGraph::Camera3D* camera;
};

CubeMapExample::CubeMapExample(const Arguments& arguments): GlutApplication(arguments, Configuration().setTitle("Cube map example")) {
    MAGNUM_ASSERT_EXTENSION_SUPPORTED(Extensions::GL::ARB::texture_storage);
    MAGNUM_ASSERT_EXTENSION_SUPPORTED(Extensions::GL::ARB::invalidate_subdata);

    Renderer::setFeature(Renderer::Feature::DepthTest, true);
    Renderer::setFeature(Renderer::Feature::FaceCulling, true);

    /* Set up perspective camera */
    (cameraObject = new Object3D(&scene))
        ->translate(Vector3::zAxis(3.0f));
    (camera = new SceneGraph::Camera3D(cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        ->setPerspective(55.0_degf, 1.0f, 0.001f, 100.0f);

    /* Load TGA importer plugin */
    PluginManager::Manager<Trade::AbstractImporter> manager(MAGNUM_PLUGINS_IMPORTER_DIR);
    Trade::AbstractImporter* importer;
    if(manager.load("JpegImporter") != PluginManager::LoadState::Loaded || !(importer = manager.instance("JpegImporter"))) {
        Error() << "Cannot load JpegImporter plugin from" << manager.pluginDirectory();
        std::exit(1);
    }
    resourceManager.set<Trade::AbstractImporter>("jpeg-importer", importer, ResourceDataState::Final, ResourcePolicy::Manual);

    /* Add objects to scene */
    (new CubeMap(arguments.argc == 2 ? arguments.argv[1] : "", &scene, &drawables))
        ->scale(Vector3(20.0f));

    (new Reflector(&scene, &drawables))
        ->scale(Vector3(0.5f))
        ->translate(Vector3::xAxis(-0.5f));

    (new Reflector(&scene, &drawables))
        ->scale(Vector3(0.3f))
        ->rotate(37.0_degf, Vector3::xAxis())
        ->translate(Vector3::xAxis(0.3f));

    /* We don't need the importer anymore */
    resourceManager.free<Trade::AbstractImporter>();
}

void CubeMapExample::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});
    camera->setViewport(size);
}

void CubeMapExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Depth);
    defaultFramebuffer.invalidate({DefaultFramebuffer::InvalidationAttachment::Color});

    camera->draw(drawables);
    swapBuffers();
}

void CubeMapExample::keyPressEvent(KeyEvent& event) {
    if(event.key() == KeyEvent::Key::Up)
        cameraObject->rotate(-10.0_degf, cameraObject->transformation().right().normalized());

    else if(event.key() == KeyEvent::Key::Down)
        cameraObject->rotate(10.0_degf, cameraObject->transformation().right().normalized());

    else if(event.key() == KeyEvent::Key::Left || event.key() == KeyEvent::Key::Right) {
        Float translationY = cameraObject->transformation().translation().y();
        cameraObject->translate(Vector3::yAxis(-translationY))
            ->rotateY(event.key() == KeyEvent::Key::Left ? 10.0_degf : -10.0_degf)
            ->translate(Vector3::yAxis(translationY));

    } else return;

    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::CubeMapExample)
