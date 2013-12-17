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
#include <Utility/Arguments.h>
#include <ColorFormat.h>
#include <DefaultFramebuffer.h>
#include <TextureFormat.h>
#include <Mesh.h>
#include <Renderer.h>
#include <ResourceManager.h>
#include <Texture.h>
#include <MeshTools/Interleave.h>
#include <MeshTools/CompressIndices.h>
#include <Platform/GlutApplication.h>
#include <SceneGraph/Camera3D.h>
#include <SceneGraph/Drawable.h>
#include <SceneGraph/MatrixTransformation3D.h>
#include <SceneGraph/Scene.h>
#include <Shaders/Phong.h>
#include <Trade/AbstractImporter.h>
#include <Trade/ImageData.h>
#include <Trade/MeshData3D.h>
#include <Trade/MeshObjectData3D.h>
#include <Trade/PhongMaterialData.h>
#include <Trade/SceneData.h>
#include <Trade/TextureData.h>

#include "configure.h"

namespace Magnum { namespace Examples {

typedef ResourceManager<Buffer, Mesh, Texture2D, Shaders::Phong, Trade::AbstractImporter, Trade::PhongMaterialData> ViewerResourceManager;
typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

class ViewerExample: public Platform::Application {
    public:
        explicit ViewerExample(const Arguments& arguments);

    private:
        void viewportEvent(const Vector2i& size) override;
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;

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

class TextureLoader: public AbstractResourceLoader<Texture2D> {
    public:
        explicit TextureLoader();

    private:
        void doLoad(ResourceKey key) override;

        Resource<Trade::AbstractImporter> importer;
        std::unordered_map<ResourceKey, std::size_t> keyMap;
};

class ColoredObject: public Object3D, SceneGraph::Drawable3D {
    public:
        ColoredObject(ResourceKey meshKey, ResourceKey materialKey, Object3D* parent, SceneGraph::DrawableGroup3D* group);

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D& camera) override;

        Resource<Mesh> mesh;
        Resource<Shaders::Phong> shader;
        Vector3 ambientColor,
            diffuseColor,
            specularColor;
        Float shininess;
};

class TexturedObject: public Object3D, SceneGraph::Drawable3D {
    public:
        TexturedObject(ResourceKey meshKey, ResourceKey materialKey, ResourceKey diffuseTextureKey, Object3D* parent, SceneGraph::DrawableGroup3D* group);

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D& camera) override;

        Resource<Mesh> mesh;
        Resource<Texture2D> diffuseTexture;
        Resource<Shaders::Phong> shader;
        Vector3 ambientColor,
            specularColor;
        Float shininess;
};

ViewerExample::ViewerExample(const Arguments& arguments): Platform::GlutApplication(arguments, Configuration().setTitle("Magnum Viewer Example")) {
    Utility::Arguments args;
    args.addArgument("file").setHelpKey("file", "file.dae").setHelp("file", "file to load")
        .parse(arguments.argc, arguments.argv);

    /* Instance ColladaImporter plugin */
    PluginManager::Manager<Trade::AbstractImporter> manager(MAGNUM_PLUGINS_IMPORTER_DIR);
    if(!(manager.load("ColladaImporter") & PluginManager::LoadState::Loaded)) {
        Error() << "Could not load ColladaImporter plugin";
        std::exit(1);
    }
    Trade::AbstractImporter* importer = manager.instance("ColladaImporter").release();
    if(!importer) {
        Error() << "Could not instance ColladaImporter plugin";
        std::exit(2);
    }

    Debug() << "Opening file" << args.value("file");

    /* Load file */
    if(!importer->openFile(args.value("file")))
        std::exit(4);
    if(!importer->sceneCount() || importer->defaultScene() == -1)
        std::exit(5);

    /* Save importer for later use */
    resourceManager.set("importer", importer);

    /* Resource loaders */
    auto meshLoader = new MeshLoader;
    auto materialLoader = new MaterialLoader;
    auto textureLoader = new TextureLoader;
    resourceManager.setLoader(meshLoader)
        .setLoader(materialLoader)
        .setLoader(textureLoader);

    /* Phong shader instances */
    resourceManager.set("color", new Shaders::Phong);
    resourceManager.set("texture", new Shaders::Phong(Shaders::Phong::Flag::DiffuseTexture));

    /* Fallback mesh for objects with no mesh */
    resourceManager.setFallback(new Mesh);

    /* Fallback material for objects with no material */
    auto material = new Trade::PhongMaterialData({}, 50.0f);
    material->ambientColor() = {0.0f, 0.0f, 0.0f};
    material->diffuseColor() = {0.9f, 0.9f, 0.9f};
    material->specularColor() = {1.0f, 1.0f, 1.0f};
    resourceManager.setFallback(material);

    /* Fallback texture for object without texture */
    resourceManager.setFallback(new Texture2D);

    /* Every scene needs a camera */
    (cameraObject = new Object3D(&scene))
        ->translate(Vector3::zAxis(5.0f));
    (camera = new SceneGraph::Camera3D(*cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setPerspective(Deg(35.0f), 1.0f, 0.001f, 100)
        .setViewport(defaultFramebuffer.viewport().size());
    Renderer::setFeature(Renderer::Feature::DepthTest, true);
    Renderer::setFeature(Renderer::Feature::FaceCulling, true);

    /* Default object, parent of all (for manipulation) */
    o = new Object3D(&scene);

    Debug() << "Adding default scene" << importer->sceneName(importer->defaultScene());

    /* Load the scene */
    std::optional<Trade::SceneData> sceneData = importer->scene(importer->defaultScene());
    CORRADE_INTERNAL_ASSERT(sceneData);

    /* Add all children */
    for(UnsignedInt objectId: sceneData->children3D())
        addObject(importer, o, objectId);

    /* Importer, materials and loaders are not needed anymore */
    resourceManager.setFallback<Trade::PhongMaterialData>(nullptr)
        .setLoader<Mesh>(nullptr)
        .setLoader<Texture2D>(nullptr)
        .setLoader<Trade::PhongMaterialData>(nullptr)
        .clear<Trade::PhongMaterialData>()
        .clear<Trade::AbstractImporter>();

    Debug() << "Imported" << meshLoader->loadedCount() << "meshes,"
            << materialLoader->loadedCount() << "materials and"
            << textureLoader->loadedCount() << "textures,"
            << meshLoader->notFoundCount() << "meshes,"
            << materialLoader->notFoundCount() << "materials and"
            << textureLoader->notFoundCount() << "textures weren't found.";
}

void ViewerExample::addObject(Trade::AbstractImporter* importer, Object3D* parent, std::size_t objectId) {
    Debug() << "Importing object" << importer->object3DName(objectId);

    Object3D* object = nullptr;
    std::unique_ptr<Trade::ObjectData3D> objectData = importer->object3D(objectId);
    CORRADE_INTERNAL_ASSERT(objectData);

    /* Only meshes for now */
    if(objectData->instanceType() == Trade::ObjectInstanceType3D::Mesh) {
        const auto materialName = importer->materialName(static_cast<Trade::MeshObjectData3D*>(objectData.get())->material());
        const ResourceKey materialKey(materialName);

        /* Decide what object to add based on material type */
        Resource<Trade::PhongMaterialData> material = resourceManager.get<Trade::PhongMaterialData>(materialKey);

        /* Color-only material */
        if(!material->flags())
            (object = new ColoredObject(importer->mesh3DName(objectData->instance()), materialKey, parent, &drawables))
                ->setTransformation(objectData->transformation());

        /* Diffuse texture material */
        else if(material->flags() == Trade::PhongMaterialData::Flag::DiffuseTexture)
            (object = new TexturedObject(importer->mesh3DName(objectData->instance()), materialKey, importer->textureName(material->diffuseTexture()), parent, &drawables))
                ->setTransformation(objectData->transformation());

        /* No other material types are supported yet */
        else Error() << "Texture combination of material" << materialName << "is not supported";
    }

    /* Create parent object for children, if it doesn't already exist */
    if(!object) object = new Object3D(parent);

    /* Recursively add children */
    for(std::size_t id: objectData->children())
        addObject(importer, object, id);
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
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    Vector3 currentPosition = positionOnSphere(event.position());

    Vector3 axis = Vector3::cross(previousPosition, currentPosition);

    if(previousPosition.length() < 0.001f || axis.length() < 0.001f) return;

    o->rotate(Vector3::angle(previousPosition, currentPosition), axis.normalized());

    previousPosition = currentPosition;

    redraw();
}

Vector3 ViewerExample::positionOnSphere(const Vector2i& _position) const {
    Vector2 position = Vector2(_position*2)/Vector2(camera->viewport()) - Vector2(1.0f);

    Float length = position.length();
    Vector3 result(length > 1.0f ? Vector3(position, 0.0f) : Vector3(position, 1.0f - length));
    result.y() *= -1.0f;
    return result.normalized();
}

MeshLoader::MeshLoader(): importer(ViewerResourceManager::instance().get<Trade::AbstractImporter>("importer")) {
    /* Fill key->name map */
    for(UnsignedInt i = 0; i != importer->mesh3DCount(); ++i)
        keyMap.emplace(importer->mesh3DName(i), i);
}

MaterialLoader::MaterialLoader(): importer(ViewerResourceManager::instance().get<Trade::AbstractImporter>("importer")) {
    /* Fill key->name map */
    for(UnsignedInt i = 0; i != importer->materialCount(); ++i)
        keyMap.emplace(importer->materialName(i), i);
}

TextureLoader::TextureLoader(): importer(ViewerResourceManager::instance().get<Trade::AbstractImporter>("importer")) {
    /* Fill key->name map */
    for(UnsignedInt i = 0; i != importer->textureCount(); ++i)
        keyMap.emplace(importer->textureName(i), i);
}

void MeshLoader::doLoad(const ResourceKey key) {
    const UnsignedInt id = keyMap.at(key);

    Debug() << "Importing mesh" << importer->mesh3DName(id) << "...";

    std::optional<Trade::MeshData3D> data = importer->mesh3D(id);
    if(!data || !data->isIndexed() || !data->positionArrayCount() || !data->normalArrayCount() || data->primitive() != MeshPrimitive::Triangles) {
        setNotFound(key);
        return;
    }

    /* Fill mesh data */
    auto mesh = new Mesh;
    auto buffer = new Buffer;
    auto indexBuffer = new Buffer;
    mesh->setPrimitive(data->primitive());
    MeshTools::compressIndices(*mesh, *indexBuffer, BufferUsage::StaticDraw, data->indices());

    /* Textured mesh */
    if(data->textureCoords2DArrayCount()) {
        MeshTools::interleave(*mesh, *buffer, BufferUsage::StaticDraw,
            data->positions(0), data->normals(0), data->textureCoords2D(0));
        mesh->addVertexBuffer(*buffer, 0,
            Shaders::Phong::Position(), Shaders::Phong::Normal(), Shaders::Phong::TextureCoordinates());

    /* Non-textured mesh */
    } else {
        MeshTools::interleave(*mesh, *buffer, BufferUsage::StaticDraw,
            data->positions(0), data->normals(0));
        mesh->addVertexBuffer(*buffer, 0,
            Shaders::Phong::Position(), Shaders::Phong::Normal());
    }

    /* Save things */
    ViewerResourceManager::instance().set(importer->mesh3DName(id) + "-vertices", buffer)
        .set(importer->mesh3DName(id) + "-indices", indexBuffer);
    set(key, mesh);
}

void MaterialLoader::doLoad(const ResourceKey key) {
    const UnsignedInt id = keyMap.at(key);

    Debug() << "Importing material" << importer->materialName(id);

    std::unique_ptr<Trade::AbstractMaterialData> material = importer->material(id);
    if(material && material->type() == Trade::MaterialType::Phong)
        set(key, static_cast<Trade::PhongMaterialData*>(material.release()), ResourceDataState::Final, ResourcePolicy::Manual);
    else setNotFound(key);
}

void TextureLoader::doLoad(const ResourceKey key) {
    const UnsignedInt id = keyMap.at(key);

    Debug() << "Importing texture" << importer->textureName(id);

    std::optional<Trade::TextureData> data = importer->texture(id);
    if(!data || data->type() != Trade::TextureData::Type::Texture2D) {
        setNotFound(key);
        return;
    }

    Debug() << "Importing image" << importer->image2DName(data->image()) << "...";

    std::optional<Trade::ImageData2D> image = importer->image2D(data->image());
    if(!image || (image->format() != ColorFormat::RGB && image->format() != ColorFormat::BGR)) {
        setNotFound(key);
        return;
    }

    /* Configure texture */
    auto texture = new Texture2D;
    texture->setMagnificationFilter(data->magnificationFilter())
        .setMinificationFilter(data->minificationFilter(), data->mipmapFilter())
        .setWrapping(data->wrapping().xy())
        .setImage(0, TextureFormat::RGB8, *image)
        .generateMipmap();

    /* Save it */
    set(key, texture);
}

ColoredObject::ColoredObject(const ResourceKey meshKey, const ResourceKey materialKey, Object3D* parent, SceneGraph::DrawableGroup3D* group): Object3D(parent), SceneGraph::Drawable3D(*this, group), mesh(ViewerResourceManager::instance().get<Mesh>(meshKey)), shader(ViewerResourceManager::instance().get<Shaders::Phong>("color")) {
    Resource<Trade::PhongMaterialData> material = ViewerResourceManager::instance().get<Trade::PhongMaterialData>(materialKey);
    ambientColor = material->ambientColor();
    diffuseColor = material->diffuseColor();
    specularColor = material->specularColor();
    shininess = material->shininess();
}

TexturedObject::TexturedObject(ResourceKey meshKey, ResourceKey materialKey, ResourceKey diffuseTextureKey, Object3D* parent, SceneGraph::DrawableGroup3D* group): Object3D(parent), SceneGraph::Drawable3D(*this, group), mesh(ViewerResourceManager::instance().get<Mesh>(meshKey)), diffuseTexture(ViewerResourceManager::instance().get<Texture2D>(diffuseTextureKey)), shader(ViewerResourceManager::instance().get<Shaders::Phong>("texture")) {
    Resource<Trade::PhongMaterialData> material = ViewerResourceManager::instance().get<Trade::PhongMaterialData>(materialKey);
    ambientColor = material->ambientColor();
    specularColor = material->specularColor();
    shininess = material->shininess();
}

void ColoredObject::draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D& camera) {
    shader->setAmbientColor(ambientColor)
        .setDiffuseColor(diffuseColor)
        .setSpecularColor(specularColor)
        .setShininess(shininess)
        .setLightPosition({-3.0f, 10.0f, 10.0f})
        .setTransformationMatrix(transformationMatrix)
        /** @todo How to avoid the assertions here? */
        .setNormalMatrix(transformationMatrix.rotationNormalized())
        .setProjectionMatrix(camera.projectionMatrix())
        .use();

    mesh->draw();
}

void TexturedObject::draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D& camera) {
    shader->setAmbientColor(ambientColor)
        .setSpecularColor(specularColor)
        .setShininess(shininess)
        .setLightPosition({-3.0f, 10.0f, 10.0f})
        .setTransformationMatrix(transformationMatrix)
        /** @todo How to avoid the assertions here? */
        .setNormalMatrix(transformationMatrix.rotationNormalized())
        .setProjectionMatrix(camera.projectionMatrix())
        .use();

    diffuseTexture->bind(Shaders::Phong::DiffuseTextureLayer);

    mesh->draw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::ViewerExample)
