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

#include "PluginManager/PluginManager.h"
#include <Math/Constants.h>
#include <Contexts/GlutWindowContext.h>
#include "SceneGraph/Scene.h"
#include "Trade/AbstractImporter.h"

#include "Billboard.h"
#include "ColorCorrectionCamera.h"

#include "configure.h"

using namespace Corrade::PluginManager;

namespace Magnum { namespace Examples {

class FramebufferExample: public Contexts::GlutWindowContext {
    public:
        FramebufferExample(int& argc, char** argv): GlutWindowContext(argc, argv, "Framebuffer example") {
            if(argc != 2) {
                Debug() << "Usage:" << argv[0] << "image.tga";
                exit(0);
            }

            camera = new ColorCorrectionCamera(&scene);

            /* Load TGA importer plugin */
            PluginManager<Trade::AbstractImporter> manager(MAGNUM_PLUGINS_IMPORTER_DIR);
            Trade::AbstractImporter* importer;
            if(manager.load("TgaImporter") != AbstractPluginManager::LoadOk || !(importer = manager.instance("TgaImporter"))) {
                Error() << "Cannot load TgaImporter plugin from" << manager.pluginDirectory();
                exit(1);
            }

            /* Load the image */
            if(!importer->open(argv[1]) || !importer->image2DCount()) {
                Error() << "Cannot open image" << argv[1];
                exit(2);
            }

            /* Create color correction texture */
            GLfloat texture[1024];
            for(size_t i = 0; i != 1024; ++i) {
                GLfloat x = i*2/1023.0-1;
                texture[i] = (float(sin(x*Math::Constants<float>::pi()))/3.7f+x+1)/2;
            }
            colorCorrectionBuffer.setData(sizeof(texture), texture, Buffer::Usage::StaticDraw);

            /* Add billboard to the scene */
            billboard = new Billboard(importer->image2D(0), &colorCorrectionBuffer, &scene);
            delete importer;
        }

    protected:
        inline void viewportEvent(const Math::Vector2<GLsizei>& size) {
            Framebuffer::setViewport({0, 0}, size);
            camera->setViewport(size);
        }

        inline void drawEvent() {
            Framebuffer::clear(Framebuffer::Clear::Color);
            camera->draw();
            swapBuffers();
        }

        void mousePressEvent(MouseButton button, const Math::Vector2<int>& position) {
            if(button == MouseButton::WheelUp)
                billboard->scale(Vector2(5.0f/4));
            else if(button == MouseButton::WheelDown)
                billboard->scale(Vector2(4.0f/5));

            previous = position;
            redraw();
        }

        void mouseMotionEvent(const Math::Vector2<int>& position) {
            billboard->translate(camera->projectionSize()*Vector2::from(position-previous)/Vector2::from(camera->viewport())*Vector2(2.0f, -2.0f));
            previous = position;
            redraw();
        }

    private:
        Math::Vector2<int> previous;
        SceneGraph::Scene2D scene;
        SceneGraph::Camera2D* camera;
        Billboard* billboard;
        Buffer colorCorrectionBuffer;
};

}}

int main(int argc, char** argv) {
    Magnum::Examples::FramebufferExample e(argc, argv);
    return e.exec();
}
