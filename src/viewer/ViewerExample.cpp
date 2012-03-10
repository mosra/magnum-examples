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

#include <iostream>
#include <chrono>
#include <memory>

#include "PluginManager/PluginManager.h"
#include "Scene.h"
#include "Camera.h"
#include "Trade/AbstractImporter.h"
#include "Trade/MeshData.h"
#include "MeshTools/Tipsify.h"
#include "MeshTools/Interleave.h"
#include "MeshTools/CompressIndices.h"
#include "Shaders/PhongShader.h"

#include "AbstractExample.h"
#include "ViewedObject.h"
#include "configure.h"

using namespace std;
using namespace Corrade::PluginManager;
using namespace Corrade::Utility;
using namespace Magnum;
using namespace Magnum::Shaders;
using namespace Magnum::Trade;
using namespace Magnum::Examples;

namespace Magnum { namespace Examples {

class ViewerExample: public AbstractExample {
    public:
        ViewerExample(int& argc, char** argv);

    protected:
        inline void viewportEvent(const Math::Vector2<GLsizei>& size) {
            camera->setViewport(size);
        }

        void drawEvent() {
            if(fps) {
                chrono::high_resolution_clock::time_point now = chrono::high_resolution_clock::now();
                double duration = chrono::duration<double>(now-before).count();
                if(duration > 3.5) {
                    cout << frames << " frames in " << duration << " sec: "
                        << frames/duration << " FPS         \r";
                    cout.flush();
                    totalfps += frames/duration;
                    before = now;
                    frames = 0;
                    ++totalmeasurecount;
                }
            }
            camera->draw();
            swapBuffers();

            if(fps) {
                ++frames;
                redraw();
            }
        }

        void keyEvent(Key key, const Math::Vector2<int>& position) {
            switch(key) {
                case Key::Up:
                    o->rotate(deg(10.0f), Vector3::xAxis(-1));
                    break;
                case Key::Down:
                    o->rotate(deg(10.0f), Vector3::xAxis(1));
                    break;
                case Key::Left:
                    o->rotate(deg(10.0f), Vector3::yAxis(-1), false);
                    break;
                case Key::Right:
                    o->rotate(deg(10.0f), Vector3::yAxis(1), false);
                    break;
                case Key::PageUp:
                    camera->translate(Vector3::zAxis(-0.5));
                    break;
                case Key::PageDown:
                    camera->translate(Vector3::zAxis(0.5));
                    break;
                case Key::Home:
                    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_FILL : GL_LINE);
                    wireframe = !wireframe;
                    break;
                case Key::End:
                    if(fps) cout << "Average FPS on " << camera->viewport().x()
                        << 'x' << camera->viewport().y() << " from "
                        << totalmeasurecount << " measures: "
                        << totalfps/totalmeasurecount << "          " << endl;
                    else before = chrono::high_resolution_clock::now();

                    fps = !fps;
                    frames = totalmeasurecount = 0;
                    totalfps = 0;
                    break;
            }

            redraw();
        }

        void mouseEvent(MouseButton button, MouseState state, const Math::Vector2<int>& position) {
            switch(button) {
                case MouseButton::Left:
                    if(state == MouseState::Down) previousPosition = positionOnSphere(position);
                    else previousPosition = Vector3();
                    break;
                case MouseButton::WheelUp:
                case MouseButton::WheelDown: {
                    if(state == MouseState::Up) return;

                    /* Distance between origin and near camera clipping plane */
                    GLfloat distance = camera->transformation().at(3).z()-0-camera->near();

                    /* Move 15% of the distance back or forward */
                    if(button == MouseButton::WheelUp)
                        distance *= 1 - 1/0.85f;
                    else
                        distance *= 1 - 0.85f;
                    camera->translate(Vector3::zAxis(distance));

                    redraw();
                    break;
                }
                default: ;
            }
        }

        void mouseMoveEvent(const Math::Vector2<int>& position) {
            Vector3 currentPosition = positionOnSphere(position);

            Vector3 axis = Vector3::cross(previousPosition, currentPosition);

            if(previousPosition.length() < 0.001f || axis.length() < 0.001f) return;

            GLfloat angle = acos(previousPosition*currentPosition);
            o->rotate(angle, axis);

            previousPosition = currentPosition;

            redraw();
        }

    private:
        Vector3 positionOnSphere(const Math::Vector2<int>& _position) const {
            Math::Vector2<GLsizei> viewport = camera->viewport();
            Vector2 position(_position.x()*2.0f/viewport.x() - 1.0f,
                             _position.y()*2.0f/viewport.y() - 1.0f);

            GLfloat length = position.length();
            Vector3 result(length > 1.0f ? position : Vector3(position, 1.0f - length));
            result.setY(-result.y());
            return result.normalized();
        }

        Scene scene;
        Camera* camera;
        PhongShader shader;
        IndexedMesh mesh;
        Object* o;
        chrono::high_resolution_clock::time_point before;
        bool wireframe, fps;
        size_t frames;
        double totalfps;
        size_t totalmeasurecount;
        Vector3 previousPosition;
};

ViewerExample::ViewerExample(int& argc, char** argv): AbstractExample(argc, argv, "Magnum Viewer"), wireframe(false), fps(false) {
    if(argc != 2) {
        cout << "Usage: " << argv[0] << " file.dae" << endl;
        exit(0);
    }

    /* Instance ColladaImporter plugin */
    PluginManager<AbstractImporter> manager(PLUGIN_IMPORTER_DIR);
    if(manager.load("ColladaImporter") != AbstractPluginManager::LoadOk) {
        Error() << "Could not load ColladaImporter plugin";
        exit(1);
    }
    unique_ptr<AbstractImporter> colladaImporter(manager.instance("ColladaImporter"));
    if(!colladaImporter) {
        Error() << "Could not instance ColladaImporter plugin";
        exit(2);
    }
    if(!(colladaImporter->features() & AbstractImporter::OpenFile)) {
        Error() << "ColladaImporter cannot open files";
        exit(3);
    }

    scene.setFeature(Scene::DepthTest, true);

    /* Every scene needs a camera */
    camera = new Camera(&scene);
    camera->setPerspective(deg(35.0f), 0.001f, 100);
    camera->translate(Vector3::zAxis(5));

    /* Load file */
    if(!colladaImporter->open(argv[1]))
        exit(4);

    if(colladaImporter->meshCount() == 0)
        exit(5);

    MeshData* data = colladaImporter->mesh(0);
    if(!data || !data->indices() || !data->vertexArrayCount() || !data->normalArrayCount())
        exit(6);

    /* Optimize vertices */
    Debug() << "Optimizing mesh vertices using Tipsify algorithm (cache size 24)...";
    MeshTools::tipsify(*data->indices(), data->vertices(0)->size(), 24);

    /* Interleave mesh data */
    Buffer* buffer = mesh.addBuffer(true);
    mesh.bindAttribute<PhongShader::Vertex>(buffer);
    mesh.bindAttribute<PhongShader::Normal>(buffer);
    MeshTools::interleave(&mesh, buffer, Buffer::Usage::StaticDraw, *data->vertices(0), *data->normals(0));

    /* Compress indices */
    MeshTools::compressIndices(&mesh, Buffer::Usage::StaticDraw, *data->indices());

    /* Get material or create default one */
    PhongMaterialData* material = static_cast<PhongMaterialData*>(colladaImporter->material(0));
    if(!material) material = new PhongMaterialData({0.0f, 0.0f, 0.0f}, {0.9f, 0.9f, 0.9f}, {1.0f, 1.0f, 1.0f}, 50.0f);

    o = new ViewedObject(&mesh, static_cast<PhongMaterialData*>(material), &shader, &scene);

    colladaImporter->close();
    delete colladaImporter.release();
}

}}

MAGNUM_EXAMPLE_MAIN(Magnum::Examples::ViewerExample)
