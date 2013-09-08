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
#include <DefaultFramebuffer.h>
#include <Platform/GlutApplication.h>
#include <SceneGraph/Scene.h>
#include <Trade/AbstractImporter.h>

#include "Billboard.h"
#include "ColorCorrectionCamera.h"

#include "configure.h"

namespace Magnum { namespace Examples {

class FramebufferExample: public Platform::GlutApplication {
    public:
        FramebufferExample(const Arguments& arguments);

    protected:
        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;

    private:
        Vector2i previous;
        Scene2D scene;
        SceneGraph::DrawableGroup2D drawables;
        SceneGraph::Camera2D* camera;
        Billboard* billboard;
        Buffer colorCorrectionBuffer;
};

FramebufferExample::FramebufferExample(const Arguments& arguments): GlutApplication(arguments, Configuration().setTitle("Framebuffer example")) {
    if(arguments.argc != 2) {
        Debug() << "Usage:" << arguments.argv[0] << "image.tga";
        std::exit(0);
    }

    camera = new ColorCorrectionCamera(scene);

    /* Load TGA importer plugin */
    PluginManager::Manager<Trade::AbstractImporter> manager(MAGNUM_PLUGINS_IMPORTER_DIR);
    std::unique_ptr<Trade::AbstractImporter> importer;
    if(manager.load("TgaImporter") != PluginManager::LoadState::Loaded || !(importer = manager.instance("TgaImporter"))) {
        Error() << "Cannot load TgaImporter plugin from" << manager.pluginDirectory();
        std::exit(1);
    }

    /* Load the image */
    if(!importer->openFile(arguments.argv[1]) || !importer->image2DCount()) {
        Error() << "Cannot open image" << arguments.argv[1];
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
    auto image = importer->image2D(0);
    CORRADE_INTERNAL_ASSERT(image);
    billboard = new Billboard(*image, &colorCorrectionBuffer, &scene, &drawables);
}

void FramebufferExample::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});
    camera->setViewport(size);
}

void FramebufferExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color);
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
    billboard->translate(camera->projectionSize()*Vector2(event.position()-previous)/Vector2(camera->viewport())*Vector2(2.0f, -2.0f));
    previous = event.position();
    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::FramebufferExample)
