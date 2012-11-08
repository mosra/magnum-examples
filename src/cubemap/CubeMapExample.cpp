/*
    Copyright © 2010, 2011, 2012 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Magnum.

    Magnum is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Magnum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

#include "CubeMapExample.h"

#include <PluginManager/PluginManager.h>
#include <Math/Constants.h>
#include <AbstractShaderProgram.h>
#include <Framebuffer.h>
#include <IndexedMesh.h>
#include <SceneGraph/Camera.h>
#include <Trade/AbstractImporter.h>

#include "CubeMap.h"
#include "Reflector.h"
#include "configure.h"

using namespace std;
using namespace Corrade::PluginManager;

namespace Magnum { namespace Examples {

CubeMapExample::CubeMapExample(int& argc, char** argv): GlutWindowContext(argc, argv, "Cube map example") {
    Framebuffer::setFeature(Framebuffer::Feature::DepthTest, true);
    Framebuffer::setFeature(Framebuffer::Feature::FaceCulling, true);

    /* Set up perspective camera */
    (cameraObject = new Object3D(&scene))
        ->translate(Vector3::zAxis(3.0f));
    (camera = new SceneGraph::Camera3D<>(cameraObject))
        ->setAspectRatioPolicy(SceneGraph::Camera3D<>::AspectRatioPolicy::Extend)
        ->setPerspective(deg(55.0f), 0.001f, 100.0f);

    /* Load TGA importer plugin */
    PluginManager<Trade::AbstractImporter> manager(MAGNUM_PLUGINS_IMPORTER_DIR);
    Trade::AbstractImporter* importer;
    if(manager.load("TgaImporter") != AbstractPluginManager::LoadOk || !(importer = manager.instance("TgaImporter"))) {
        Error() << "Cannot load TGAImporter plugin from" << manager.pluginDirectory();
        exit(1);
    }
    resourceManager.set<Trade::AbstractImporter>("tga-importer", importer, ResourceDataState::Final, ResourcePolicy::Manual);

    /* Add objects to scene */
    (new CubeMap(argc == 2 ? argv[1] : "", &scene, &drawables))
        ->scale(Vector3(20.0f));

    (new Reflector(&scene, &drawables))
        ->scale(Vector3(0.5f))
        ->translate(Vector3::xAxis(-0.5f));

    (new Reflector(&scene, &drawables))
        ->scale(Vector3(0.3f))
        ->rotate(deg(37.0f), Vector3::xAxis())
        ->translate(Vector3::xAxis(0.3f));

    /* We don't need the importer anymore */
    resourceManager.free<Trade::AbstractImporter>();
}

void CubeMapExample::viewportEvent(const Math::Vector2<GLsizei>& size) {
    Framebuffer::setViewport({0, 0}, size);
    camera->setViewport(size);
}

void CubeMapExample::drawEvent() {
    Framebuffer::clear(Framebuffer::Clear::Depth);
    camera->draw(drawables);
    swapBuffers();
}

void CubeMapExample::keyPressEvent(Key key, const Magnum::Math::Vector2<int>&) {
    if(key == Key::Up)
        cameraObject->rotate(deg(-10.0f), cameraObject->transformation()[0].xyz().normalized());

    else if(key == Key::Down)
        cameraObject->rotate(deg(10.0f), cameraObject->transformation()[0].xyz().normalized());

    else if(key == Key::Left || key == Key::Right) {
        GLfloat translationY = cameraObject->transformation().translation().y();
        cameraObject->translate(Vector3::yAxis(-translationY))
            ->rotate(key == Key::Left ? deg(10.0f) : deg(-10.0f), Vector3::yAxis())
            ->translate(Vector3::yAxis(translationY));

    } else return;

    redraw();
}

}}

int main(int argc, char** argv) {
    Magnum::Examples::CubeMapExample e(argc, argv);
    return e.exec();
}
