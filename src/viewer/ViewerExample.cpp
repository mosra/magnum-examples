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
#include <Mesh.h>
#include <Renderer.h>
#include <ResourceManager.h>
#include <MeshTools/Interleave.h>
#include <MeshTools/CompressIndices.h>
#include <SceneGraph/Scene.h>
#include <SceneGraph/Camera3D.h>
#include <SceneGraph/Drawable.h>
#include <SceneGraph/MatrixTransformation3D.h>
#include <Shaders/Phong.h>
#include <Trade/AbstractImporter.h>
#include <Trade/MeshData3D.h>
#include <Trade/MeshObjectData3D.h>
#include <Trade/PhongMaterialData.h>
#include <Trade/SceneData.h>

#ifndef MAGNUM_TARGET_GLES
#include <Platform/GlutApplication.h>
#else
#include <Platform/XEglApplication.h>
#endif

#include "configure.h"

namespace Magnum { namespace Examples {

typedef ResourceManager<Trade::AbstractImporter, Trade::PhongMaterialData, Shaders::Phong, Mesh, Buffer> ViewerResourceManager;
typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

class ViewerExample: public Platform::Application {
    public:
        explicit ViewerExample(const Arguments& arguments);

    protected:
        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;

    private:
        Vector3 positionOnSphere(const Vector2i& _position) const;

        void addObject(Trade::AbstractImporter* importer, Object3D* parent, std::size_t objectId);

        ViewerResourceManager resourceManager;

        Scene3D scene;
        Object3D *o, *cameraObject;
        SceneGraph::Camera3D* camera;
        SceneGraph::DrawableGroup3D drawables;
        Vector3 previousPosition;
};

class MeshLoader: public AbstractResourceLoader<Mesh> {
    public:
        explicit MeshLoader();

    private:
        void doLoad(ResourceKey key) override;

        Resource<Trade::AbstractImporter> importer;
        std::unordered_map<ResourceKey, std::size_t> keyMap;
};

class MaterialLoader: public AbstractResourceLoader<Trade::PhongMaterialData> {
    public:
        explicit MaterialLoader();

    private:
        void doLoad(ResourceKey key) override;

        Resource<Trade::AbstractImporter> importer;
        std::unordered_map<ResourceKey, std::size_t> keyMap;
};

class ViewedObject: public Object3D, SceneGraph::Drawable3D {
    public:
        ViewedObject(ResourceKey meshKey, ResourceKey materialKey, Object3D* parent, SceneGraph::DrawableGroup3D* group);

        void draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D* camera) override;

    private:
        Resource<Mesh> mesh;
        Resource<Shaders::Phong> shader;
        Vector3 ambientColor,
            diffuseColor,
            specularColor;
        Float shininess;
};

ViewerExample::ViewerExample(const Arguments& arguments): Platform::Application(arguments, Configuration().setTitle("Magnum Viewer")) {
    if(arguments.argc != 2) {
        Debug() << "Usage:" << arguments.argv[0] << "file.dae";
        std::exit(0);
    }

    /* Instance ColladaImporter plugin */
    PluginManager::Manager<Trade::AbstractImporter> manager(MAGNUM_PLUGINS_IMPORTER_DIR);
    if(manager.load("ColladaImporter") != PluginManager::LoadState::Loaded) {
        Error() << "Could not load ColladaImporter plugin";
        std::exit(1);
    }
    Trade::AbstractImporter* importer = manager.instance("ColladaImporter");
    if(importer) resourceManager.set("importer", importer, ResourceDataState::Final, ResourcePolicy::Manual);
    else {
        Error() << "Could not instance ColladaImporter plugin";
        std::exit(2);
    }

    /* Every scene needs a camera */
    (cameraObject = new Object3D(&scene))
        ->translate(Vector3::zAxis(5));
    (camera = new SceneGraph::Camera3D(cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        ->setPerspective(35.0_degf, 1.0f, 0.001f, 100);
    Renderer::setFeature(Renderer::Feature::DepthTest, true);
    Renderer::setFeature(Renderer::Feature::FaceCulling, true);

    Debug() << "Opening file" << arguments.argv[1];

    /* Load file */
    if(!importer->openFile(arguments.argv[1])) {
        std::exit(4);
    }

    if(!importer->sceneCount()) {
        std::exit(5);
    }

    /* Resource loaders */
    auto meshLoader = new MeshLoader;
    auto materialLoader = new MaterialLoader;
    resourceManager.setLoader(meshLoader)->setLoader(materialLoader);

    /* Phong shader instance */
    resourceManager.set("color", new Shaders::Phong);

    /* Fallback mesh for objects with no mesh */
    resourceManager.setFallback(new Mesh);

    /* Fallback material for objects with no material */
    auto material = new Trade::PhongMaterialData({}, 50.0f);
    material->ambientColor() = {0.0f, 0.0f, 0.0f};
    material->diffuseColor() = {0.9f, 0.9f, 0.9f};
    material->specularColor() = {1.0f, 1.0f, 1.0f};
    resourceManager.setFallback(material);

    Debug() << "Adding default scene" << importer->sceneName(importer->defaultScene());

    /* Default object, parent of all (for manipulation) */
    o = new Object3D(&scene);

    /* Load the scene */
    Trade::SceneData* scene = importer->scene(importer->defaultScene());

    /* Add all children */
    for(UnsignedInt objectId: scene->children3D())
        addObject(importer, o, objectId);

    /* Importer, materials and loaders are not needed anymore */
    resourceManager.setFallback<Trade::PhongMaterialData>(nullptr)
        ->setLoader<Mesh>(nullptr)
        ->setLoader<Trade::PhongMaterialData>(nullptr)
        ->clear<Trade::PhongMaterialData>()
        ->clear<Trade::AbstractImporter>();

    Debug() << "Imported" << meshLoader->loadedCount() << "meshes and"
            << materialLoader->loadedCount() << "materials," << meshLoader->notFoundCount()
            << "meshes and" << materialLoader->notFoundCount() << "materials weren't found.";
}

void ViewerExample::viewportEvent(const Vector2i& size) {
    defaultFramebuffer.setViewport({{}, size});
    camera->setViewport(size);
}

void ViewerExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color|FramebufferClear::Depth);
    camera->draw(drawables);
    swapBuffers();
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
            distance *= 1 - (event.button() == MouseEvent::Button::WheelUp ? 1/0.85f : 0.85f);
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
    Vector2 position = Vector2(_position*2)/camera->viewport() - Vector2(1.0f);

    Float length = position.length();
    Vector3 result(length > 1.0f ? Vector3(position, 0.0f) : Vector3(position, 1.0f - length));
    result.y() *= -1.0f;
    return result.normalized();
}

void ViewerExample::addObject(Trade::AbstractImporter* importer, Object3D* parent, std::size_t objectId) {
    Trade::ObjectData3D* object = importer->object3D(objectId);

    Debug() << "Importing object" << importer->object3DName(objectId);

    /* Only meshes for now */
    if(object->instanceType() == Trade::ObjectData3D::InstanceType::Mesh) {
        const auto materialName = importer->materialName(static_cast<Trade::MeshObjectData3D*>(object)->material());
        const ResourceKey materialKey(materialName);

        /* Decide what object to add based on material type */
        Resource<Trade::PhongMaterialData> material = resourceManager.get<Trade::PhongMaterialData>(materialKey);

        /* Color-only material */
        if(!material->flags())
            (new ViewedObject(importer->mesh3DName(object->instance()), materialKey, parent, &drawables))
                ->setTransformation(object->transformation());

        /* No other material types are supported yet */
        else Error() << "Texture combination of material" << materialName << "is not supported";
    }

    /* Recursively add children */
    for(std::size_t id: object->children())
        addObject(importer, o, id);
}

MeshLoader::MeshLoader(): importer(ViewerResourceManager::instance()->get<Trade::AbstractImporter>("importer")) {
    /* Fill key->name map */
    for(UnsignedInt i = 0; i != importer->mesh3DCount(); ++i)
        keyMap.emplace(importer->mesh3DName(i), i);
}

void MeshLoader::doLoad(const ResourceKey key) {
    const UnsignedInt id = keyMap.at(key);

    Debug() << "Importing mesh" << importer->mesh3DName(id) << "...";

    Trade::MeshData3D* data = importer->mesh3D(id);
    if(!data || !data->isIndexed() || !data->positionArrayCount() || !data->normalArrayCount() || data->primitive() != Mesh::Primitive::Triangles) {
        delete data;
        setNotFound(key);
        return;
    }

    /* Fill mesh data */
    auto mesh = new Mesh;
    auto buffer = new Buffer;
    auto indexBuffer = new Buffer;
    MeshTools::interleave(mesh, buffer, Buffer::Usage::StaticDraw, data->positions(0), data->normals(0));
    MeshTools::compressIndices(mesh, indexBuffer, Buffer::Usage::StaticDraw, data->indices());
    mesh->addInterleavedVertexBuffer(buffer, 0, Shaders::Phong::Position(), Shaders::Phong::Normal())
        ->setPrimitive(data->primitive());

    /* Save things */
    ViewerResourceManager::instance()->set(importer->mesh3DName(id) + "-vertices", buffer);
    ViewerResourceManager::instance()->set(importer->mesh3DName(id) + "-indices", indexBuffer);
    set(key, mesh);
    delete data;
}

MaterialLoader::MaterialLoader(): importer(ViewerResourceManager::instance()->get<Trade::AbstractImporter>("importer")) {
    /* Fill key->name map */
    for(UnsignedInt i = 0; i != importer->materialCount(); ++i)
        keyMap.emplace(importer->materialName(i), i);
}

void MaterialLoader::doLoad(const ResourceKey key) {
    const UnsignedInt id = keyMap.at(key);

    Debug() << "Importing material" << importer->materialName(id);

    auto material = importer->material(id);
    if(material && material->type() == Trade::AbstractMaterialData::Type::Phong)
        set(key, static_cast<Trade::PhongMaterialData*>(material), ResourceDataState::Final, ResourcePolicy::Manual);
    else setNotFound(key);
}

ViewedObject::ViewedObject(const ResourceKey meshKey, const ResourceKey materialKey, Object3D* parent, SceneGraph::DrawableGroup3D* group): Object3D(parent), SceneGraph::Drawable3D(this, group), mesh(ViewerResourceManager::instance()->get<Mesh>(meshKey)), shader(ViewerResourceManager::instance()->get<Shaders::Phong>("color")) {
    Resource<Trade::PhongMaterialData> material = ViewerResourceManager::instance()->get<Trade::PhongMaterialData>(materialKey);
    ambientColor = material->ambientColor();
    diffuseColor = material->diffuseColor();
    specularColor = material->specularColor();
    shininess = material->shininess();
}

void ViewedObject::draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D* camera) {
    shader->setAmbientColor(ambientColor)
        ->setDiffuseColor(diffuseColor)
        ->setSpecularColor(specularColor)
        ->setShininess(shininess)
        ->setLightPosition({-3.0f, 10.0f, 10.0f})
        ->setTransformationMatrix(transformationMatrix)
        ->setNormalMatrix(transformationMatrix.rotation())
        ->setProjectionMatrix(camera->projectionMatrix())
        ->use();

    mesh->draw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::ViewerExample)
