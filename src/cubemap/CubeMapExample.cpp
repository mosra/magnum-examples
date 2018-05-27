/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 —
            Vladimír Vondruš <mosra@centrum.cz>

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

#include <Corrade/PluginManager/Manager.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/CubeMapTexture.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/Trade/AbstractImporter.h>

#include "CubeMap.h"
#include "Reflector.h"
#include "Types.h"

#include <iostream>
#include <fstream>

inline bool file_exists(const std::string& name) {
    std::ifstream f(name.c_str());
    return f.good();
}

namespace Magnum { namespace Examples {

class CubeMapExample: public Platform::Application {
    public:
        explicit CubeMapExample(const Arguments& arguments);

    private:
        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;
        void keyPressEvent(KeyEvent& event) override;

        CubeMapResourceManager _resourceManager;
        Scene3D _scene;
        SceneGraph::DrawableGroup3D _drawables;
        Object3D* _cameraObject;
        SceneGraph::Camera3D* _camera;

        std::string _assetPath;
};

CubeMapExample::CubeMapExample(const Arguments& arguments): Platform::Application(arguments, Configuration().setTitle("Magnum Cube Map Example")) {
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    /* Set up perspective camera */
    (_cameraObject = new Object3D(&_scene))
        ->translate(Vector3::zAxis(3.0f));
    (_camera = new SceneGraph::Camera3D(*_cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(Deg(55.0f), 1.0f, 0.001f, 100.0f))
        .setViewport(GL::defaultFramebuffer.viewport().size());

    /* Load TGA importer plugin */
    PluginManager::Manager<Trade::AbstractImporter> manager;
    std::unique_ptr<Trade::AbstractImporter> importer = manager.loadAndInstantiate("JpegImporter");
    if(!importer) std::exit(1);

    _resourceManager.set<Trade::AbstractImporter>("jpeg-importer",
        importer.release(), ResourceDataState::Final, ResourcePolicy::Manual);

    // If we don't have any path argument, then we need to look for the assets
    if (arguments.argc < 2) {
        if (file_exists(std::string("/usr/share/magnum/examples/cubemap/+x.jpg"))) {
            _assetPath = std::string("/usr/share/magnum/examples/cubemap/");
        } else if (file_exists("+x.jpg")) {
            _assetPath = std::string("");
        } else {
            std::cerr << "Unable to find cubemap assets. Please provide a path where the assets can be found." << std::endl;
            std::exit(1);
        }
    } else {
        if (file_exists(std::string(arguments.argv[1]) + std::string("/+x.jpg"))) {
            _assetPath = std::string(arguments.argv[1]);
        } else {
            std::cerr << "Provided path does not have the required assets. Please provide a path where the assets can be found." << std::endl;
            std::exit(1);
        }
    }

    /* Add objects to scene */
    (new CubeMap(_assetPath, &_scene, &_drawables))
        ->scale(Vector3(20.0f));

    (new Reflector(&_scene, &_drawables))
        ->scale(Vector3(0.5f))
        .translate(Vector3::xAxis(-0.5f));

    (new Reflector(&_scene, &_drawables))
        ->scale(Vector3(0.3f))
        .rotate(Deg(37.0f), Vector3::xAxis())
        .translate(Vector3::xAxis(0.3f));

    /* We don't need the importer anymore */
    _resourceManager.free<Trade::AbstractImporter>();
}

void CubeMapExample::viewportEvent(const Vector2i& size) {
    GL::defaultFramebuffer.setViewport({{}, size});
    _camera->setViewport(size);
}

void CubeMapExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Depth);
    GL::defaultFramebuffer.invalidate({GL::DefaultFramebuffer::InvalidationAttachment::Color});

    _camera->draw(_drawables);
    swapBuffers();
}

void CubeMapExample::keyPressEvent(KeyEvent& event) {
    if(event.key() == KeyEvent::Key::Up)
        _cameraObject->rotate(Deg(-10.0f), _cameraObject->transformation().right().normalized());

    else if(event.key() == KeyEvent::Key::Down)
        _cameraObject->rotate(Deg(10.0f), _cameraObject->transformation().right().normalized());

    else if(event.key() == KeyEvent::Key::Left || event.key() == KeyEvent::Key::Right) {
        Float translationY = _cameraObject->transformation().translation().y();
        _cameraObject->translate(Vector3::yAxis(-translationY))
            .rotateY(event.key() == KeyEvent::Key::Left ? Deg(10.0f) : Deg(-10.0f))
            .translate(Vector3::yAxis(translationY));

    } else return;

    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::CubeMapExample)
