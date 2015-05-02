/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015
              Vladimír Vondruš <mosra@centrum.cz>

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

#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Arguments.h>
#include <Magnum/Buffer.h>
#include <Magnum/ColorFormat.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/Mesh.h>
#include <Magnum/Renderer.h>
#include <Magnum/ResourceManager.h>
#include <Magnum/Texture.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/SceneGraph/Camera3D.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData3D.h>
#include <Magnum/Trade/MeshObjectData3D.h>
#include <Magnum/Trade/PhongMaterialData.h>
#include <Magnum/Trade/SceneData.h>
#include <Magnum/Trade/TextureData.h>

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

        void addObject(Trade::AbstractImporter& importer, Object3D* parent, std::size_t objectId);

        ViewerResourceManager _resourceManager;

        Scene3D _scene;
        Object3D *_o, *_cameraObject;
        SceneGraph::Camera3D* _camera;
        SceneGraph::DrawableGroup3D _drawables;
        Vector3 _previousPosition;
};

class MeshLoader: public AbstractResourceLoader<Mesh> {
    public:
        explicit MeshLoader();

    private:
        void doLoad(ResourceKey key) override;

        Resource<Trade::AbstractImporter> _importer;
        std::unordered_map<ResourceKey, std::size_t> _keyMap;
};

class MaterialLoader: public AbstractResourceLoader<Trade::PhongMaterialData> {
    public:
        explicit MaterialLoader();

    private:
        void doLoad(ResourceKey key) override;

        Resource<Trade::AbstractImporter> _importer;
        std::unordered_map<ResourceKey, std::size_t> _keyMap;
};

class TextureLoader: public AbstractResourceLoader<Texture2D> {
    public:
        explicit TextureLoader();

    private:
        void doLoad(ResourceKey key) override;

        Resource<Trade::AbstractImporter> _importer;
        std::unordered_map<ResourceKey, std::size_t> _keyMap;
};

class ColoredObject: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit ColoredObject(ResourceKey meshKey, ResourceKey materialKey, Object3D* parent, SceneGraph::DrawableGroup3D* group);

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D& camera) override;

        Resource<Mesh> _mesh;
        Resource<Shaders::Phong> _shader;
        Vector3 _ambientColor,
            _diffuseColor,
            _specularColor;
        Float _shininess;
};

class TexturedObject: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit TexturedObject(ResourceKey meshKey, ResourceKey materialKey, ResourceKey diffuseTextureKey, Object3D* parent, SceneGraph::DrawableGroup3D* group);

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D& camera) override;

        Resource<Mesh> _mesh;
        Resource<Texture2D> _diffuseTexture;
        Resource<Shaders::Phong> _shader;
        Vector3 _ambientColor,
            _specularColor;
        Float _shininess;
};

ViewerExample::ViewerExample(const Arguments& arguments): Platform::Application(arguments, Configuration().setTitle("Magnum Viewer Example")) {
    Utility::Arguments args;
    args.addArgument("file").setHelpKey("file", "file.dae").setHelp("file", "file to load")
        .parse(arguments.argc, arguments.argv);

    /* Instance ColladaImporter plugin */
    PluginManager::Manager<Trade::AbstractImporter> manager(MAGNUM_PLUGINS_IMPORTER_DIR);
    if(!(manager.load("ColladaImporter") & PluginManager::LoadState::Loaded))
        std::exit(1);
    Trade::AbstractImporter* importer = manager.instance("ColladaImporter").release();

    Debug() << "Opening file" << args.value("file");

    /* Load file */
    if(!importer->openFile(args.value("file")))
        std::exit(4);
    if(!importer->sceneCount() || importer->defaultScene() == -1)
        std::exit(5);

    /* Save importer for later use */
    _resourceManager.set("importer", importer);

    /* Resource loaders */
    auto meshLoader = new MeshLoader;
    auto materialLoader = new MaterialLoader;
    auto textureLoader = new TextureLoader;
    _resourceManager.setLoader(meshLoader)
        .setLoader(materialLoader)
        .setLoader(textureLoader);

    /* Phong shader instances */
    _resourceManager.set("color", new Shaders::Phong);
    _resourceManager.set("texture", new Shaders::Phong(Shaders::Phong::Flag::DiffuseTexture));

    /* Fallback mesh for objects with no mesh */
    _resourceManager.setFallback(new Mesh);

    /* Fallback material for objects with no material */
    auto material = new Trade::PhongMaterialData({}, 50.0f);
    material->ambientColor() = {0.0f, 0.0f, 0.0f};
    material->diffuseColor() = {0.9f, 0.9f, 0.9f};
    material->specularColor() = {1.0f, 1.0f, 1.0f};
    _resourceManager.setFallback(material);

    /* Fallback texture for object without texture */
    _resourceManager.setFallback(new Texture2D);

    /* Every scene needs a camera */
    (_cameraObject = new Object3D(&_scene))
        ->translate(Vector3::zAxis(5.0f));
    (_camera = new SceneGraph::Camera3D(*_cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setPerspective(Deg(35.0f), 1.0f, 0.001f, 100)
        .setViewport(defaultFramebuffer.viewport().size());
    Renderer::enable(Renderer::Feature::DepthTest);
    Renderer::enable(Renderer::Feature::FaceCulling);

    /* Default object, parent of all (for manipulation) */
    _o = new Object3D(&_scene);

    Debug() << "Adding default scene" << importer->sceneName(importer->defaultScene());

    /* Load the scene */
    std::optional<Trade::SceneData> sceneData = importer->scene(importer->defaultScene());
    CORRADE_INTERNAL_ASSERT(sceneData);

    /* Add all children */
    for(UnsignedInt objectId: sceneData->children3D())
        addObject(*importer, _o, objectId);

    /* Importer, materials and loaders are not needed anymore */
    _resourceManager.setFallback<Trade::PhongMaterialData>(nullptr)
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

void ViewerExample::addObject(Trade::AbstractImporter& importer, Object3D* parent, std::size_t objectId) {
    Debug() << "Importing object" << importer.object3DName(objectId);

    Object3D* object = nullptr;
    std::unique_ptr<Trade::ObjectData3D> objectData = importer.object3D(objectId);
    CORRADE_INTERNAL_ASSERT(objectData);

    /* Only meshes for now */
    if(objectData->instanceType() == Trade::ObjectInstanceType3D::Mesh) {
        const auto materialName = importer.materialName(static_cast<Trade::MeshObjectData3D*>(objectData.get())->material());
        const ResourceKey materialKey(materialName);

        /* Decide what object to add based on material type */
        Resource<Trade::PhongMaterialData> material = _resourceManager.get<Trade::PhongMaterialData>(materialKey);

        /* Color-only material */
        if(!material->flags())
            (object = new ColoredObject(importer.mesh3DName(objectData->instance()), materialKey, parent, &_drawables))
                ->setTransformation(objectData->transformation());

        /* Diffuse texture material */
        else if(material->flags() == Trade::PhongMaterialData::Flag::DiffuseTexture)
            (object = new TexturedObject(importer.mesh3DName(objectData->instance()), materialKey, importer.textureName(material->diffuseTexture()), parent, &_drawables))
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
    _camera->setViewport(size);
}

void ViewerExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color|FramebufferClear::Depth);
    _camera->draw(_drawables);
    swapBuffers();
}

void ViewerExample::mousePressEvent(MouseEvent& event) {
    switch(event.button()) {
        case MouseEvent::Button::Left:
            _previousPosition = positionOnSphere(event.position());
            break;

        case MouseEvent::Button::WheelUp:
        case MouseEvent::Button::WheelDown: {
            /* Distance between origin and near camera clipping plane */
            Float distance = _cameraObject->transformation().translation().z()-0- _camera->near();

            /* Move 15% of the distance back or forward */
            distance *= 1 - (event.button() == MouseEvent::Button::WheelUp ? 1/0.85f : 0.85f);
            _cameraObject->translate(Vector3::zAxis(distance));

            redraw();
            break;
        }

        default: ;
    }
}

void ViewerExample::mouseReleaseEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left)
        _previousPosition = Vector3();
}

void ViewerExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    Vector3 currentPosition = positionOnSphere(event.position());

    Vector3 axis = Math::cross(_previousPosition, currentPosition);

    if(_previousPosition.length() < 0.001f || axis.length() < 0.001f) return;

    _o->rotate(Math::angle(_previousPosition, currentPosition), axis.normalized());

    _previousPosition = currentPosition;

    redraw();
}

Vector3 ViewerExample::positionOnSphere(const Vector2i& _position) const {
    Vector2 position = Vector2(_position*2)/Vector2(_camera->viewport()) - Vector2(1.0f);

    Float length = position.length();
    Vector3 result(length > 1.0f ? Vector3(position, 0.0f) : Vector3(position, 1.0f - length));
    result.y() *= -1.0f;
    return result.normalized();
}

MeshLoader::MeshLoader(): _importer(ViewerResourceManager::instance().get<Trade::AbstractImporter>("importer")) {
    /* Fill key->name map */
    for(UnsignedInt i = 0; i != _importer->mesh3DCount(); ++i)
        _keyMap.insert({_importer->mesh3DName(i), i});
}

MaterialLoader::MaterialLoader(): _importer(ViewerResourceManager::instance().get<Trade::AbstractImporter>("importer")) {
    /* Fill key->name map */
    for(UnsignedInt i = 0; i != _importer->materialCount(); ++i)
        _keyMap.insert({_importer->materialName(i), i});
}

TextureLoader::TextureLoader(): _importer(ViewerResourceManager::instance().get<Trade::AbstractImporter>("importer")) {
    /* Fill key->name map */
    for(UnsignedInt i = 0; i != _importer->textureCount(); ++i)
        _keyMap.insert({_importer->textureName(i), i});
}

void MeshLoader::doLoad(const ResourceKey key) {
    const UnsignedInt id = _keyMap.at(key);

    Debug() << "Importing mesh" << _importer->mesh3DName(id) << "...";

    std::optional<Trade::MeshData3D> data = _importer->mesh3D(id);
    if(!data || !data->hasNormals() || data->primitive() != MeshPrimitive::Triangles) {
        setNotFound(key);
        return;
    }

    /* Compile the mesh */
    std::optional<Mesh> mesh;
    std::unique_ptr<Buffer> buffer, indexBuffer;
    std::tie(mesh, buffer, indexBuffer) = MeshTools::compile(*data, BufferUsage::StaticDraw);

    /* Save things */
    ViewerResourceManager::instance().set(_importer->mesh3DName(id) + "-vertices", buffer.release());
    if(indexBuffer)
        ViewerResourceManager::instance().set(_importer->mesh3DName(id) + "-indices", indexBuffer.release());
    set(key, new Mesh(std::move(*mesh)));
}

void MaterialLoader::doLoad(const ResourceKey key) {
    const UnsignedInt id = _keyMap.at(key);

    Debug() << "Importing material" << _importer->materialName(id);

    std::unique_ptr<Trade::AbstractMaterialData> material = _importer->material(id);
    if(material && material->type() == Trade::MaterialType::Phong)
        set(key, static_cast<Trade::PhongMaterialData*>(material.release()), ResourceDataState::Final, ResourcePolicy::Manual);
    else setNotFound(key);
}

void TextureLoader::doLoad(const ResourceKey key) {
    const UnsignedInt id = _keyMap.at(key);

    Debug() << "Importing texture" << _importer->textureName(id);

    std::optional<Trade::TextureData> data = _importer->texture(id);
    if(!data || data->type() != Trade::TextureData::Type::Texture2D) {
        setNotFound(key);
        return;
    }

    Debug() << "Importing image" << _importer->image2DName(data->image()) << "...";

    std::optional<Trade::ImageData2D> image = _importer->image2D(data->image());
    if(!image || (image->format() != ColorFormat::RGB
        #ifndef MAGNUM_TARGET_GLES
        && image->format() != ColorFormat::BGR
        #endif
        ))
    {
        setNotFound(key);
        return;
    }

    /* Configure texture */
    auto texture = new Texture2D;
    texture->setMagnificationFilter(data->magnificationFilter())
        .setMinificationFilter(data->minificationFilter(), data->mipmapFilter())
        .setWrapping(data->wrapping().xy())
        .setStorage(1, TextureFormat::RGB8, image->size())
        .setSubImage(0, {}, *image)
        .generateMipmap();

    /* Save it */
    set(key, texture);
}

ColoredObject::ColoredObject(const ResourceKey meshKey, const ResourceKey materialKey, Object3D* parent, SceneGraph::DrawableGroup3D* group): Object3D(parent), SceneGraph::Drawable3D(*this, group), _mesh(ViewerResourceManager::instance().get<Mesh>(meshKey)), _shader(ViewerResourceManager::instance().get<Shaders::Phong>("color")) {
    Resource<Trade::PhongMaterialData> material = ViewerResourceManager::instance().get<Trade::PhongMaterialData>(materialKey);
    _ambientColor = material->ambientColor();
    _diffuseColor = material->diffuseColor();
    _specularColor = material->specularColor();
    _shininess = material->shininess();
}

TexturedObject::TexturedObject(ResourceKey meshKey, ResourceKey materialKey, ResourceKey diffuseTextureKey, Object3D* parent, SceneGraph::DrawableGroup3D* group): Object3D(parent), SceneGraph::Drawable3D(*this, group), _mesh(ViewerResourceManager::instance().get<Mesh>(meshKey)), _diffuseTexture(ViewerResourceManager::instance().get<Texture2D>(diffuseTextureKey)), _shader(ViewerResourceManager::instance().get<Shaders::Phong>("texture")) {
    Resource<Trade::PhongMaterialData> material = ViewerResourceManager::instance().get<Trade::PhongMaterialData>(materialKey);
    _ambientColor = material->ambientColor();
    _specularColor = material->specularColor();
    _shininess = material->shininess();
}

void ColoredObject::draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D& camera) {
    _shader->setAmbientColor(_ambientColor)
        .setDiffuseColor(_diffuseColor)
        .setSpecularColor(_specularColor)
        .setShininess(_shininess)
        .setLightPosition(camera.cameraMatrix().transformPoint({-3.0f, 10.0f, 10.0f}))
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.rotation())
        .setProjectionMatrix(camera.projectionMatrix());

    _mesh->draw(*_shader);
}

void TexturedObject::draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D& camera) {
    _shader->setAmbientColor(_ambientColor)
        .setDiffuseTexture(*_diffuseTexture)
        .setSpecularColor(_specularColor)
        .setShininess(_shininess)
        .setLightPosition(camera.cameraMatrix().transformPoint({-3.0f, 10.0f, 10.0f}))
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.rotation())
        .setProjectionMatrix(camera.projectionMatrix());

    _mesh->draw(*_shader);
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::ViewerExample)
