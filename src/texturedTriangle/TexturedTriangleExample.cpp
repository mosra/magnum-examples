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
#include "Camera.h"
#include "Trade/AbstractImporter.h"

#include "AbstractExample.h"
#include "TexturedTriangle.h"

#include "configure.h"

using namespace Corrade::Utility;
using namespace Corrade::PluginManager;

namespace Magnum { namespace Examples {

class TexturedTriangleExample: public AbstractExample {
    public:
        TexturedTriangleExample(int& argc, char** argv): AbstractExample(argc, argv, "Textured triangle example") {
            camera = new Camera(&scene);

            /* Load TGA importer plugin */
            PluginManager<Trade::AbstractImporter> manager(MAGNUM_PLUGINS_IMPORTER_DIR);
            Trade::AbstractImporter* importer;
            if(manager.load("TgaImporter") != AbstractPluginManager::LoadOk || !(importer = manager.instance("TgaImporter"))) {
                Error() << "Cannot load TgaImporter plugin from" << manager.pluginDirectory();
                exit(1);
            }

            /* Load the texture */
            Resource rs("data");
            std::istringstream in(rs.get("stone.tga"));
            if(!importer->open(in) || !importer->image2DCount()) {
                Error() << "Cannot load texture";
                exit(2);
            }

            /* Add textured triangle to the scene */
            new TexturedTriangle(importer->image2D(0), &scene);

            /* Delete importer plugin after use */
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

    private:
        Scene scene;
        Camera* camera;
};

}}

MAGNUM_EXAMPLE_MAIN(Magnum::Examples::TexturedTriangleExample)
