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

#include <Scene.h>
#include <Camera.h>
#include <Contexts/GlutContext.h>
#include <Trade/AbstractImporter.h>

#include "TexturedTriangleExample.h"
#include "TexturedTriangle.h"

#include "configure.h"

namespace Magnum { namespace Examples {

TexturedTriangleExample::TexturedTriangleExample(int& argc, char** argv): GlutContext(argc, argv, "Textured triangle example"), camera(&scene) {
    /* Load TGA importer plugin */
    Corrade::PluginManager::PluginManager<Trade::AbstractImporter> manager(MAGNUM_PLUGINS_IMPORTER_DIR);
    Magnum::Trade::AbstractImporter* importer;
    if(manager.load("TgaImporter") != Corrade::PluginManager::AbstractPluginManager::LoadOk || !(importer = manager.instance("TgaImporter"))) {
        Corrade::Utility::Error() << "Cannot load TgaImporter plugin from" << manager.pluginDirectory();
        exit(1);
    }

    /* Load the texture */
    Corrade::Utility::Resource rs("data");
    std::istringstream in(rs.get("stone.tga"));
    if(!importer->open(in) || !importer->image2DCount()) {
        Corrade::Utility::Error() << "Cannot load texture";
        exit(2);
    }

    /* Add textured triangle to the scene and delete the importer */
    new TexturedTriangle(importer->image2D(0), &scene);
    delete importer;
}

}}

int main(int argc, char** argv) {
    Magnum::Examples::TexturedTriangleExample e(argc, argv);
    return e.exec();
}

