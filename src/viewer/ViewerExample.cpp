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

#include <memory>
#include <unordered_map>

#include "PluginManager/PluginManager.h"
#include "Scene.h"
#include "Camera.h"
#include "Trade/AbstractImporter.h"
#include "Trade/MeshData.h"
#include "Trade/MeshObjectData.h"
#include "Trade/SceneData.h"
#include "MeshTools/Tipsify.h"
#include "MeshTools/Interleave.h"
#include "MeshTools/CompressIndices.h"
#include "Shaders/PhongShader.h"

#include "FpsCounterExample.h"
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

class ViewerExample: public FpsCounterExample {
    public:
        ViewerExample(int& argc, char** argv);

        ~ViewerExample() {
            for(auto i: meshes) delete i.second;
        }

    protected:
        inline void viewportEvent(const Math::Vector2<GLsizei>& size) {
            camera->setViewport(size);
            FpsCounterExample::viewportEvent(size);
        }

        void drawEvent() {
            camera->draw();
            swapBuffers();

            if(fpsCounterEnabled()) redraw();
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
                    camera->translate(Vector3::zAxis(-0.5), false);
                    break;
                case Key::PageDown:
                    camera->translate(Vector3::zAxis(0.5), false);
                    break;
                case Key::Home:
                    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_FILL : GL_LINE);
                    wireframe = !wireframe;
                    break;
                case Key::End:
                    if(fpsCounterEnabled()) printCounterStatistics();
                    else resetCounter();

                    setFpsCounterEnabled(!fpsCounterEnabled());
                    break;
                default: break;
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
                    camera->translate(Vector3::zAxis(distance), false);

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

        void addObject(AbstractImporter* colladaImporter, Object* parent, unordered_map<size_t, PhongMaterialData*>& materials, size_t objectId);

        Scene scene;
        Camera* camera;
        PhongShader shader;
        Object* o;
        unordered_map<size_t, IndexedMesh*> meshes;
        size_t vertexCount, triangleCount, objectCount, meshCount, materialCount;
        bool wireframe;
        Vector3 previousPosition;
};

ViewerExample::ViewerExample(int& argc, char** argv): FpsCounterExample(argc, argv, "Magnum Viewer"), vertexCount(0), triangleCount(0), objectCount(0), meshCount(0), materialCount(0), wireframe(false) {
    if(argc != 2) {
        Debug() << "Usage:" << argv[0] << "file.dae";
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

    Debug() << "Opening file" << argv[1];

    /* Load file */
    if(!colladaImporter->open(argv[1]))
        exit(4);

    if(colladaImporter->sceneCount() == 0)
        exit(5);

    /* Map with materials */
    unordered_map<size_t, PhongMaterialData*> materials;

    /* Default object, parent of all (for manipulation) */
    o = new Object(&scene);

    Debug() << "Adding default scene...";

    /* Load the scene */
    SceneData* scene = colladaImporter->scene(colladaImporter->defaultScene());

    /* Add all children */
    for(size_t objectId: scene->children())
        addObject(colladaImporter.get(), o, materials, objectId);

    Debug() << "Imported" << objectCount << "objects with" << meshCount << "meshes and" << materialCount << "materials,";
    Debug() << "    " << vertexCount << "vertices and" << triangleCount << "triangles total.";

    /* Delete materials, as they are now unused */
    for(auto i: materials) delete i.second;

    colladaImporter->close();
    delete colladaImporter.release();
}

void ViewerExample::addObject(AbstractImporter* colladaImporter, Object* parent, unordered_map<size_t, PhongMaterialData*>& materials, size_t objectId) {
    ObjectData* object = colladaImporter->object(objectId);

    /* Only meshes for now */
    if(object->instanceType() == ObjectData::InstanceType::Mesh) {
        ++objectCount;

        /* Use already processed mesh, if exists */
        IndexedMesh* mesh;
        auto found = meshes.find(object->instanceId());
        if(found != meshes.end()) mesh = found->second;

        /* Or create a new one */
        else {
            ++meshCount;

            mesh = new IndexedMesh;
            meshes.insert(make_pair(object->instanceId(), mesh));

            MeshData* data = colladaImporter->mesh(object->instanceId());
            if(!data || !data->indices() || !data->vertexArrayCount() || !data->normalArrayCount())
                exit(6);

            vertexCount += data->vertices(0)->size();
            triangleCount += data->indices()->size()/3;

            /* Optimize vertices */
            Debug() << "Optimizing vertices of mesh" << object->instanceId() << "using Tipsify algorithm (cache size 24)...";
            MeshTools::tipsify(*data->indices(), data->vertices(0)->size(), 24);

            /* Interleave mesh data */
            Buffer* buffer = mesh->addBuffer(true);
            mesh->bindAttribute<PhongShader::Vertex>(buffer);
            mesh->bindAttribute<PhongShader::Normal>(buffer);
            MeshTools::interleave(mesh, buffer, Buffer::Usage::StaticDraw, *data->vertices(0), *data->normals(0));

            /* Compress indices */
            MeshTools::compressIndices(mesh, Buffer::Usage::StaticDraw, *data->indices());
        }

        /* Use already processed material, if exists */
        PhongMaterialData* material;
        auto materialFound = materials.find(static_cast<MeshObjectData*>(object)->material());
        if(materialFound != materials.end()) material = materialFound->second;

        /* Else get material or create default one */
        else {
            ++materialCount;

            material = static_cast<PhongMaterialData*>(colladaImporter->material(static_cast<MeshObjectData*>(object)->material()));
            if(!material) material = new PhongMaterialData({0.0f, 0.0f, 0.0f}, {0.9f, 0.9f, 0.9f}, {1.0f, 1.0f, 1.0f}, 50.0f);
        }

        /* Add object */
        Object* o = new ViewedObject(mesh, material, &shader, parent);
        o->setTransformation(object->transformation());
    }

    /* Recursively add children */
    for(size_t id: object->children())
        addObject(colladaImporter, o, materials, id);
}

}}

MAGNUM_EXAMPLE_MAIN(Magnum::Examples::ViewerExample)
