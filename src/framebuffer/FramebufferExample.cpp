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

#include <PluginManager/PluginManager.h>
#include <DefaultFramebuffer.h>
#include <Platform/GlutApplication.h>
#include <SceneGraph/Scene.h>
#include <Trade/AbstractImporter.h>

#include "Billboard.h"
#include "ColorCorrectionCamera.h"

#include "configure.h"

using namespace Corrade::PluginManager;

namespace Magnum { namespace Examples {

class FramebufferExample: public Platform::GlutApplication {
    public:
        FramebufferExample(int& argc, char** argv);

    protected:
        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;

    private:
        Vector2i previous;
        Scene2D scene;
        SceneGraph::DrawableGroup2D<> drawables;
        SceneGraph::Camera2D<>* camera;
        Billboard* billboard;
        Buffer colorCorrectionBuffer;
};

FramebufferExample::FramebufferExample(int& argc, char** argv): GlutApplication(argc, argv, "Framebuffer example") {
    if(argc != 2) {
        Debug() << "Usage:" << argv[0] << "image.tga";
        std::exit(0);
    }

    camera = new ColorCorrectionCamera(&scene);

    /* Load TGA importer plugin */
    PluginManager<Trade::AbstractImporter> manager(MAGNUM_PLUGINS_IMPORTER_DIR);
    Trade::AbstractImporter* importer;
    if(manager.load("TgaImporter") != LoadState::Loaded || !(importer = manager.instance("TgaImporter"))) {
        Error() << "Cannot load TgaImporter plugin from" << manager.pluginDirectory();
        std::exit(1);
    }

    /* Load the image */
    if(!importer->open(argv[1]) || !importer->image2DCount()) {
        Error() << "Cannot open image" << argv[1];
        std::exit(2);
    }

    /* Create color correction texture */
    Float texture[1024];
    for(std::size_t i = 0; i != 1024; ++i) {
        Float x = i*2/1023.0-1;
        texture[i] = (std::sin(x*Constants::pi())/3.7f+x+1)/2;
    }
    colorCorrectionBuffer.setData(sizeof(texture), texture, Buffer::Usage::StaticDraw);

    /* Add billboard to the scene */
    billboard = new Billboard(importer->image2D(0), &colorCorrectionBuffer, &scene, &drawables);
    delete importer;
}

void FramebufferExample::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});
    camera->setViewport(size);
}

void FramebufferExample::drawEvent() {
    defaultFramebuffer.clear(DefaultFramebuffer::Clear::Color);
    camera->draw(drawables);
    swapBuffers();
}

void FramebufferExample::mousePressEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::WheelUp)
        billboard->scale(Vector2(5.0f/4));
    else if(event.button() == MouseEvent::Button::WheelDown)
        billboard->scale(Vector2(4.0f/5));

    previous = event.position();
    redraw();
}

void FramebufferExample::mouseMoveEvent(MouseMoveEvent& event) {
    billboard->translate(camera->projectionSize()*Vector2(event.position()-previous)/camera->viewport()*Vector2(2.0f, -2.0f));
    previous = event.position();
    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::FramebufferExample)
