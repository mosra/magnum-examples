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
#include "Framebuffer.h"
#include "Contexts/GlutContext.h"
#include "SceneGraph/Scene.h"
#include "SceneGraph/Camera.h"
#include "Trade/AbstractImporter.h"

#include "CubeMap.h"
#include "configure.h"

using namespace std;
using namespace Corrade::PluginManager;

namespace Magnum { namespace Examples {

class CubeMapExample: public Contexts::GlutContext {
    public:
        CubeMapExample(int& argc, char** argv): GlutContext(argc, argv, "Cube map example") {
            /* Every scene needs a camera */
            camera = new SceneGraph::Camera3D(&scene);
            camera->setAspectRatioPolicy(SceneGraph::Camera3D::AspectRatioPolicy::Extend);
            camera->setPerspective(deg(55.0f), 0.001f, 100);
            camera->translate(Vector3::zAxis(3));
            Framebuffer::setFeature(Framebuffer::Feature::DepthTest, true);
            Framebuffer::setFeature(Framebuffer::Feature::FaceCulling, true);

            /* Load TGA importer plugin */
            PluginManager<Trade::AbstractImporter> manager(MAGNUM_PLUGINS_IMPORTER_DIR);
            Trade::AbstractImporter* importer;
            if(manager.load("TgaImporter") != AbstractPluginManager::LoadOk || !(importer = manager.instance("TgaImporter"))) {
                Error() << "Cannot load TGAImporter plugin from" << manager.pluginDirectory();
                exit(1);
            }

            string prefix;
            if(argc == 2)
                prefix = argv[1];

            /* Add cube map to the scene */
            new CubeMap(importer, prefix, &scene);

            delete importer;
        }

    protected:
        inline void viewportEvent(const Math::Vector2<GLsizei>& size) {
            Framebuffer::setViewport({0, 0}, size);
            camera->setViewport(size);
        }

        inline void drawEvent() {
            Framebuffer::clear(Framebuffer::Clear::Depth);
            camera->draw();
            swapBuffers();
        }

        void keyPressEvent(Key key, const Magnum::Math::Vector2<int>&) {
            if(key == Key::Up)
                camera->rotate(deg(-10.0f), camera->transformation()[0].xyz());

            if(key == Key::Down)
                camera->rotate(deg(10.0f), camera->transformation()[0].xyz());

            if(key == Key::Left || key == Key::Right) {
                GLfloat yTransform = camera->transformation()[3][2];
                camera->translate(Vector3::yAxis(-yTransform));
                if(key == Key::Left)
                    camera->rotate(deg(10.0f), Vector3::yAxis());
                else
                    camera->rotate(deg(-10.0f), Vector3::yAxis());
                camera->translate(Vector3::yAxis(yTransform));
            }

            redraw();
        }

    private:
        SceneGraph::Scene3D scene;
        SceneGraph::Camera3D* camera;
};

}}

int main(int argc, char** argv) {
    Magnum::Examples::CubeMapExample e(argc, argv);
    return e.exec();
}
