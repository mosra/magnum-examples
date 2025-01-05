/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>

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

#include <Corrade/Containers/Pair.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/Path.h>
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

#include "configure.h"

namespace Magnum { namespace Examples {

class CubeMapExample: public Platform::Application {
    public:
        explicit CubeMapExample(const Arguments& arguments);

    private:
        void viewportEvent(ViewportEvent& event) override;
        void drawEvent() override;
        void keyPressEvent(KeyEvent& event) override;

        CubeMapResourceManager _resourceManager;
        Scene3D _scene;
        SceneGraph::DrawableGroup3D _drawables;
        Object3D* _cameraObject;
        SceneGraph::Camera3D* _camera;
};

using namespace Math::Literals;

CubeMapExample::CubeMapExample(const Arguments& arguments): Platform::Application{arguments, NoCreate} {
    /* Try to locate the bundled images, first in the source directory, then in
       the installation directory and as a fallback next to the executable */
    Containers::String defaultPath;
    if(Utility::Path::exists(CUBEMAP_EXAMPLE_DIR))
        defaultPath = CUBEMAP_EXAMPLE_DIR;
    else if(Utility::Path::exists(CUBEMAP_EXAMPLE_INSTALL_DIR))
        defaultPath = CUBEMAP_EXAMPLE_INSTALL_DIR;
    else
        defaultPath = Utility::Path::split(*Utility::Path::executableLocation()).first();

    /* Finally, provide a way for the user to override the path */
    Utility::Arguments args;
    args.addFinalOptionalArgument("path", defaultPath)
            .setHelp("path", "a combined cube map file (such as an EXR) or a directory where the +x.jpg, +y.jpg, ... files are")
        .addSkippedPrefix("magnum", "engine-specific options")
        .setGlobalHelp("Cube map rendering example")
        .parse(arguments.argc, arguments.argv);

    /* Create the context after parsing arguments to avoid a flickering window
       in case parse() exits */
    create(Configuration{}.setTitle("Magnum Cube Map Example"));

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    /* Set up perspective camera */
    (_cameraObject = new Object3D(&_scene))
        ->translate(Vector3::zAxis(3.0f));
    (_camera = new SceneGraph::Camera3D(*_cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(55.0_degf, 1.0f, 0.001f, 100.0f))
        .setViewport(GL::defaultFramebuffer.viewport().size());

    /* Load image importer plugin */
    PluginManager::Manager<Trade::AbstractImporter> manager;
    Containers::Pointer<Trade::AbstractImporter> importer = manager.loadAndInstantiate("AnyImageImporter");
    if(!importer) std::exit(1);

    _resourceManager.set<Trade::AbstractImporter>("jpeg-importer",
        importer.release(), ResourceDataState::Final, ResourcePolicy::Manual);

    /* Add objects to scene */
    (new CubeMap(_resourceManager, args.value("path"), &_scene, &_drawables))
        ->scale(Vector3(20.0f));

    (new Reflector(_resourceManager, &_scene, &_drawables))
        ->scale(Vector3(0.5f))
        .translate(Vector3::xAxis(-0.5f));

    (new Reflector(_resourceManager, &_scene, &_drawables))
        ->scale(Vector3(0.3f))
        .rotate(37.0_degf, Vector3::xAxis())
        .translate(Vector3::xAxis(0.3f));

    /* We don't need the importer anymore */
    _resourceManager.free<Trade::AbstractImporter>();
}

void CubeMapExample::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});
    _camera->setViewport(event.windowSize());
}

void CubeMapExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Depth);
    GL::defaultFramebuffer.invalidate({GL::DefaultFramebuffer::InvalidationAttachment::Color});

    _camera->draw(_drawables);
    swapBuffers();
}

void CubeMapExample::keyPressEvent(KeyEvent& event) {
    if(event.key() == Key::Up)
        _cameraObject->rotate(-10.0_degf, _cameraObject->transformation().right().normalized());

    else if(event.key() == Key::Down)
        _cameraObject->rotate(10.0_degf, _cameraObject->transformation().right().normalized());

    else if(event.key() == Key::Left || event.key() == Key::Right) {
        Float translationY = _cameraObject->transformation().translation().y();
        _cameraObject->translate(Vector3::yAxis(-translationY))
            .rotateY(event.key() == Key::Left ? 10.0_degf : -10.0_degf)
            .translate(Vector3::yAxis(translationY));

    } else return;

    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::CubeMapExample)
