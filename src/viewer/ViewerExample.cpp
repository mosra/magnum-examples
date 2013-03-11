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

#include <memory>
#include <unordered_map>

#include <PluginManager/PluginManager.h>
#include <DefaultFramebuffer.h>
#include <Mesh.h>
#include <Renderer.h>
#include <MeshTools/Tipsify.h>
#include <MeshTools/Interleave.h>
#include <MeshTools/CompressIndices.h>
#include <SceneGraph/Scene.h>
#include <SceneGraph/Camera3D.h>
#include <Shaders/PhongShader.h>
#include <Trade/AbstractImporter.h>
#include <Trade/MeshData3D.h>
#include <Trade/MeshObjectData3D.h>
#include <Trade/SceneData.h>

#include "FpsCounterExample.h"
#include "ViewedObject.h"
#include "configure.h"

using namespace Corrade::PluginManager;
using namespace Magnum::Shaders;
using namespace Magnum::Trade;
using namespace Magnum::Examples;

namespace Magnum { namespace Examples {

class ViewerExample: public FpsCounterExample {
    public:
        ViewerExample(int& argc, char** argv);
        ~ViewerExample();

    protected:
        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;
        void keyPressEvent(KeyEvent& event) override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;

    private:
        Vector3 positionOnSphere(const Vector2i& _position) const;

        void addObject(AbstractImporter* colladaImporter, Object3D* parent, std::unordered_map<std::size_t, PhongMaterialData*>& materials, std::size_t objectId);

        Scene3D scene;
        SceneGraph::DrawableGroup3D<> drawables;
        Object3D* cameraObject;
        SceneGraph::Camera3D<>* camera;
        PhongShader shader;
        Object3D* o;
        std::unordered_map<std::size_t, std::tuple<Buffer*, Buffer*, Mesh*>> meshes;
        std::size_t vertexCount, triangleCount, objectCount, meshCount, materialCount;
        bool wireframe;
        Vector3 previousPosition;
};

ViewerExample::ViewerExample(int& argc, char** argv): FpsCounterExample(argc, argv, "Magnum Viewer"), vertexCount(0), triangleCount(0), objectCount(0), meshCount(0), materialCount(0), wireframe(false) {
    if(argc != 2) {
        Debug() << "Usage:" << argv[0] << "file.dae";
        std::exit(0);
    }

    /* Instance ColladaImporter plugin */
    PluginManager<AbstractImporter> manager(MAGNUM_PLUGINS_IMPORTER_DIR);
    if(manager.load("ColladaImporter") != LoadState::Loaded) {
        Error() << "Could not load ColladaImporter plugin";
        std::exit(1);
    }
    std::unique_ptr<AbstractImporter> colladaImporter(manager.instance("ColladaImporter"));
    if(!colladaImporter) {
        Error() << "Could not instance ColladaImporter plugin";
        std::exit(2);
    }
    if(!(colladaImporter->features() & AbstractImporter::Feature::OpenFile)) {
        Error() << "ColladaImporter cannot open files";
        std::exit(3);
    }

    /* Every scene needs a camera */
    (cameraObject = new Object3D(&scene))
        ->translate(Vector3::zAxis(5));
    (camera = new SceneGraph::Camera3D<>(cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        ->setPerspective(35.0_degf, 1.0f, 0.001f, 100);
    Renderer::setFeature(Renderer::Feature::DepthTest, true);
    Renderer::setFeature(Renderer::Feature::FaceCulling, true);

    Debug() << "Opening file" << argv[1];

    /* Load file */
    if(!colladaImporter->open(argv[1]))
        std::exit(4);

    if(colladaImporter->sceneCount() == 0)
        std::exit(5);

    /* Map with materials */
    std::unordered_map<std::size_t, PhongMaterialData*> materials;

    /* Default object, parent of all (for manipulation) */
    o = new Object3D(&scene);

    Debug() << "Adding default scene...";

    /* Load the scene */
    SceneData* scene = colladaImporter->scene(colladaImporter->defaultScene());

    /* Add all children */
    for(std::size_t objectId: scene->children3D())
        addObject(colladaImporter.get(), o, materials, objectId);

    Debug() << "Imported" << objectCount << "objects with" << meshCount << "meshes and" << materialCount << "materials,";
    Debug() << "    " << vertexCount << "vertices and" << triangleCount << "triangles total.";

    /* Delete materials, as they are now unused */
    for(auto i: materials) delete i.second;

    colladaImporter->close();
    delete colladaImporter.release();
}

ViewerExample::~ViewerExample() {
    for(auto i: meshes) {
        delete std::get<0>(i.second);
        delete std::get<1>(i.second);
        delete std::get<2>(i.second);
    }
}

void ViewerExample::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});
    camera->setViewport(size);
    FpsCounterExample::viewportEvent(size);
}

void ViewerExample::drawEvent() {
    defaultFramebuffer.clear(DefaultFramebuffer::Clear::Color|DefaultFramebuffer::Clear::Depth);
    camera->draw(drawables);
    swapBuffers();

    if(fpsCounterEnabled()) redraw();
}

void ViewerExample::keyPressEvent(KeyEvent& event) {
    switch(event.key()) {
        case KeyEvent::Key::Up:
            o->rotateX(-10.0_degf);
            break;
        case KeyEvent::Key::Down:
            o->rotateX(10.0_degf);
            break;
        case KeyEvent::Key::Left:
            o->rotateY(-10.0_degf, SceneGraph::TransformationType::Local);
            break;
        case KeyEvent::Key::Right:
            o->rotateY(10.0_degf, SceneGraph::TransformationType::Local);
            break;
        case KeyEvent::Key::PageUp:
            cameraObject->translate(Vector3::zAxis(-0.5), SceneGraph::TransformationType::Local);
            break;
        case KeyEvent::Key::PageDown:
            cameraObject->translate(Vector3::zAxis(0.5), SceneGraph::TransformationType::Local);
            break;
        #ifndef MAGNUM_TARGET_GLES
        case KeyEvent::Key::Home:
            Mesh::setPolygonMode(wireframe ? Mesh::PolygonMode::Fill : Mesh::PolygonMode::Line);
            wireframe = !wireframe;
            break;
        #endif
        case KeyEvent::Key::End:
            if(fpsCounterEnabled()) printCounterStatistics();
            else resetCounter();

            setFpsCounterEnabled(!fpsCounterEnabled());
            break;
        default: break;
    }

    redraw();
}

void ViewerExample::mousePressEvent(MouseEvent& event) {
    switch(event.button()) {
        case MouseEvent::Button::Left:
            previousPosition = positionOnSphere(event.position());
            break;
        case MouseEvent::Button::WheelUp:
        case MouseEvent::Button::WheelDown: {
            /* Distance between origin and near camera clipping plane */
            Float distance = cameraObject->transformation().translation().z()-0-camera->near();

            /* Move 15% of the distance back or forward */
            if(event.button() == MouseEvent::Button::WheelUp)
                distance *= 1 - 1/0.85f;
            else
                distance *= 1 - 0.85f;
            cameraObject->translate(Vector3::zAxis(distance), SceneGraph::TransformationType::Global);

            redraw();
            break;
        }
        default: ;
    }
}

void ViewerExample::mouseReleaseEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left)
        previousPosition = Vector3();
}

void ViewerExample::mouseMoveEvent(MouseMoveEvent& event) {
    Vector3 currentPosition = positionOnSphere(event.position());

    Vector3 axis = Vector3::cross(previousPosition, currentPosition);

    if(previousPosition.length() < 0.001f || axis.length() < 0.001f) return;

    o->rotate(Vector3::angle(previousPosition, currentPosition), axis.normalized());

    previousPosition = currentPosition;

    redraw();
}

Vector3 ViewerExample::positionOnSphere(const Vector2i& _position) const {
    Vector2i viewport = camera->viewport();
    Vector2 position(_position.x()*2.0f/viewport.x() - 1.0f,
                     _position.y()*2.0f/viewport.y() - 1.0f);

    Float length = position.length();
    Vector3 result(length > 1.0f ? Vector3(position, 0.0f) : Vector3(position, 1.0f - length));
    result.y() *= -1.0f;
    return result.normalized();
}

void ViewerExample::addObject(AbstractImporter* colladaImporter, Object3D* parent, std::unordered_map<std::size_t, PhongMaterialData*>& materials, std::size_t objectId) {
    ObjectData3D* object = colladaImporter->object3D(objectId);

    /* Only meshes for now */
    if(object->instanceType() == ObjectData3D::InstanceType::Mesh) {
        ++objectCount;

        /* Use already processed mesh, if exists */
        Mesh* mesh;
        auto found = meshes.find(object->instanceId());
        if(found != meshes.end()) mesh = std::get<2>(found->second);

        /* Or create a new one */
        else {
            ++meshCount;

            mesh = new Mesh;
            Buffer* buffer = new Buffer;
            Buffer* indexBuffer = new Buffer;
            meshes.insert(std::make_pair(object->instanceId(), std::make_tuple(buffer, indexBuffer, mesh)));

            MeshData3D* data = colladaImporter->mesh3D(object->instanceId());
            if(!data || !data->indices() || !data->positionArrayCount() || !data->normalArrayCount())
                std::exit(6);

            vertexCount += data->positions(0)->size();
            triangleCount += data->indices()->size()/3;

            /* Optimize vertices */
            Debug() << "Optimizing vertices of mesh" << object->instanceId() << "using Tipsify algorithm (cache size 24)...";
            MeshTools::tipsify(*data->indices(), data->positions(0)->size(), 24);

            /* Interleave mesh data */
            MeshTools::interleave(mesh, buffer, Buffer::Usage::StaticDraw, *data->positions(0), *data->normals(0));
            mesh->addInterleavedVertexBuffer(buffer, 0, PhongShader::Position(), PhongShader::Normal());

            /* Compress indices */
            MeshTools::compressIndices(mesh, indexBuffer, Buffer::Usage::StaticDraw, *data->indices());
        }

        /* Use already processed material, if exists */
        PhongMaterialData* material;
        auto materialFound = materials.find(static_cast<MeshObjectData3D*>(object)->material());
        if(materialFound != materials.end()) material = materialFound->second;

        /* Else get material or create default one */
        else {
            ++materialCount;

            material = static_cast<PhongMaterialData*>(colladaImporter->material(static_cast<MeshObjectData3D*>(object)->material()));
            if(!material) material = new PhongMaterialData({0.0f, 0.0f, 0.0f}, {0.9f, 0.9f, 0.9f}, {1.0f, 1.0f, 1.0f}, 50.0f);
        }

        /* Add object */
        Object3D* o = new ViewedObject(mesh, material, &shader, parent, &drawables);
        o->setTransformation(object->transformation());
    }

    /* Recursively add children */
    for(std::size_t id: object->children())
        addObject(colladaImporter, o, materials, id);
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::ViewerExample)
