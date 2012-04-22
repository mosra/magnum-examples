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
#include "Scene.h"
#include "Trade/AbstractImporter.h"

#include "AbstractExample.h"
#include "Billboard.h"
#include "ColorCorrectionCamera.h"

#include "configure.h"

using namespace Corrade::Utility;
using namespace Corrade::PluginManager;

namespace Magnum { namespace Examples {

class FramebufferExample: public AbstractExample {
    public:
        FramebufferExample(int& argc, char** argv): AbstractExample(argc, argv, "Framebuffer example"), colorCorrectionBuffer(Buffer::Target::Texture) {
            if(argc != 2) {
                Debug() << "Usage:" << argv[0] << "image.tga";
                exit(0);
            }

            camera = new ColorCorrectionCamera(&scene);

            /* Load TGA importer plugin */
            PluginManager<Trade::AbstractImporter> manager(PLUGIN_IMPORTER_DIR);
            Trade::AbstractImporter* importer;
            if(manager.load("TgaImporter") != AbstractPluginManager::LoadOk || !(importer = manager.instance("TgaImporter"))) {
                Error() << "Cannot load TgaImporter plugin from" << PLUGIN_IMPORTER_DIR;
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
                texture[i] = (sin(x*PI)/3.7+x+1)/2;
            }
            colorCorrectionBuffer.setData(sizeof(texture), texture, Buffer::Usage::StaticDraw);

            /* Add billboard to the scene */
            billboard = new Billboard(importer->image2D(0), &colorCorrectionBuffer, &scene);
            delete importer;
        }

    protected:
        inline void viewportEvent(const Math::Vector2<GLsizei>& size) {
            camera->setViewport(size);
        }

        inline void drawEvent() {
            camera->draw();
            swapBuffers();
        }

        void mouseEvent(MouseButton button, MouseState state, const Math::Vector2<int>& position) {
            if(button == MouseButton::WheelUp)
                billboard->scale(Vector3(5.0f/4));
            else if(button == MouseButton::WheelDown)
                billboard->scale(Vector3(4.0f/5));

            previous = position;
            redraw();
        }

        void mouseMoveEvent(const Math::Vector2<int>& position) {
            Math::Vector2<int> delta = position-previous;
            billboard->translate({GLfloat(delta.x())/camera->viewport().x(),
                                 -GLfloat(delta.y())/camera->viewport().y(),
                                  0});
            previous = position;
            redraw();
        }

    private:
        Math::Vector2<int> previous;
        Scene scene;
        Camera* camera;
        Billboard* billboard;
        Buffer colorCorrectionBuffer;
};

}}

MAGNUM_EXAMPLE_MAIN(Magnum::Examples::FramebufferExample)
