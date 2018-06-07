/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 —
            Vladimír Vondruš <mosra@centrum.cz>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Arguments.h>
#include <Magnum/Mesh.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/ResourceManager.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/SceneGraph/Camera.h>
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

namespace Magnum { namespace Examples {

typedef ResourceManager<GL::Buffer, GL::Mesh, GL::Texture2D, Shaders::Phong, Trade::PhongMaterialData> ViewerResourceManager;
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
        void mouseScrollEvent(MouseScrollEvent& event) override;

        Vector3 positionOnSphere(const Vector2i& _position) const;

        void addObject(Trade::AbstractImporter& importer, Object3D* parent, UnsignedInt i);

        ViewerResourceManager _resourceManager;

        Scene3D _scene;
        Object3D *_o, *_cameraObject;
        SceneGraph::Camera3D* _camera;
        SceneGraph::DrawableGroup3D _drawables;
        Vector3 _previousPosition;
};

class ColoredObject: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit ColoredObject(ResourceKey meshId, ResourceKey materialId, Object3D* parent, SceneGraph::DrawableGroup3D* group);

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Resource<GL::Mesh> _mesh;
        Resource<Shaders::Phong> _shader;
        Color4 _ambientColor,
            _diffuseColor,
            _specularColor;
        Float _shininess;
};

class TexturedObject: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit TexturedObject(ResourceKey meshId, ResourceKey materialId, ResourceKey diffuseTextureId, Object3D* parent, SceneGraph::DrawableGroup3D* group);

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Resource<GL::Mesh> _mesh;
        Resource<GL::Texture2D> _diffuseTexture;
        Resource<Shaders::Phong> _shader;
        Color4 _ambientColor,
            _specularColor;
        Float _shininess;
};

ViewerExample::ViewerExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}.setTitle("Magnum Viewer Example")}
{
    Utility::Arguments args;
    args.addArgument("file").setHelp("file", "file to load")
        .addOption("importer", "AnySceneImporter").setHelp("importer", "importer plugin to use")
        .setHelp("Loads and displays 3D scene file (such as OpenGEX or "
                 "COLLADA one) provided on command-line.")
        .parse(arguments.argc, arguments.argv);

    /* Phong shader instances */
    _resourceManager
        .set("color", new Shaders::Phong)
        .set("texture", new Shaders::Phong{Shaders::Phong::Flag::DiffuseTexture});

    using namespace Math::Literals;

    /* Fallback material, texture and mesh in case the data are not present or
       cannot be loaded */
    auto material = new Trade::PhongMaterialData{{}, 50.0f};
    material->ambientColor() = 0x000000_rgbf;
    material->diffuseColor() = 0xe5e5e5_rgbf;
    material->specularColor() = 0xffffff_rgbf;
    _resourceManager
        .setFallback(material)
        .setFallback(new GL::Texture2D)
        .setFallback(new GL::Mesh);

    /* Every scene needs a camera */
    (*(_cameraObject = new Object3D{&_scene}))
        .translate(Vector3::zAxis(5.0f));
    (*(_camera = new SceneGraph::Camera3D{*_cameraObject}))
        .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 1.0f, 0.01f, 10.0f))
        .setViewport(GL::defaultFramebuffer.viewport().size());
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    /* Load scene importer plugin */
    PluginManager::Manager<Trade::AbstractImporter> manager;
    std::unique_ptr<Trade::AbstractImporter> importer = manager.loadAndInstantiate(args.value("importer"));
    if(!importer) std::exit(1);

    Debug{} << "Opening file" << args.value("file");

    /* Load file */
    if(!importer->openFile(args.value("file")))
        std::exit(4);

    /* Load all materials */
    for(UnsignedInt i = 0; i != importer->materialCount(); ++i) {
        Debug{} << "Importing material" << i << importer->materialName(i);

        std::unique_ptr<Trade::AbstractMaterialData> materialData = importer->material(i);
        if(!materialData || materialData->type() != Trade::MaterialType::Phong) {
            Warning{} << "Cannot load material, skipping";
            continue;
        }

        /* Save the material */
        _resourceManager.set(ResourceKey{i}, static_cast<Trade::PhongMaterialData*>(materialData.release()));
    }

    /* Load all textures */
    for(UnsignedInt i = 0; i != importer->textureCount(); ++i) {
        Debug{} << "Importing texture" << i << importer->textureName(i);

        Containers::Optional<Trade::TextureData> textureData = importer->texture(i);
        if(!textureData || textureData->type() != Trade::TextureData::Type::Texture2D) {
            Warning{} << "Cannot load texture, skipping";
            continue;
        }

        Debug{} << "Importing image" << textureData->image() << importer->image2DName(textureData->image());

        Containers::Optional<Trade::ImageData2D> imageData = importer->image2D(textureData->image());
        if(!imageData || imageData->format() != PixelFormat::RGB8Unorm) {
            Warning{} << "Cannot load texture image, skipping";
            continue;
        }

        /* Configure texture */
        auto texture = new GL::Texture2D;
        texture->setMagnificationFilter(textureData->magnificationFilter())
            .setMinificationFilter(textureData->minificationFilter(), textureData->mipmapFilter())
            .setWrapping(textureData->wrapping().xy())
            .setStorage(1, GL::TextureFormat::RGB8, imageData->size())
            .setSubImage(0, {}, *imageData)
            .generateMipmap();

        /* Save it */
        _resourceManager.set(ResourceKey{i}, texture, ResourceDataState::Final, ResourcePolicy::Manual);
    }

    /* Load all meshes */
    for(UnsignedInt i = 0; i != importer->mesh3DCount(); ++i) {
        Debug{} << "Importing mesh" << i << importer->mesh3DName(i);

        Containers::Optional<Trade::MeshData3D> meshData = importer->mesh3D(i);
        if(!meshData || !meshData->hasNormals() || meshData->primitive() != MeshPrimitive::Triangles) {
            Warning{} << "Cannot load mesh, skipping";
            continue;
        }

        /* Compile the mesh */
        GL::Mesh mesh{NoCreate};
        std::unique_ptr<GL::Buffer> buffer, indexBuffer;
        std::tie(mesh, buffer, indexBuffer) = MeshTools::compile(*meshData, GL::BufferUsage::StaticDraw);

        /* Save things */
        _resourceManager.set(ResourceKey{i}, new GL::Mesh{std::move(mesh)}, ResourceDataState::Final, ResourcePolicy::Manual);
        _resourceManager.set(std::to_string(i) + "-vertices", buffer.release(), ResourceDataState::Final, ResourcePolicy::Manual);
        if(indexBuffer)
            _resourceManager.set(std::to_string(i) + "-indices", indexBuffer.release(), ResourceDataState::Final, ResourcePolicy::Manual);
    }

    /* Default object, parent of all (for manipulation) */
    _o = new Object3D{&_scene};

    /* Load the scene */
    if(importer->defaultScene() != -1) {
        Debug{} << "Adding default scene" << importer->sceneName(importer->defaultScene());

        Containers::Optional<Trade::SceneData> sceneData = importer->scene(importer->defaultScene());
        if(!sceneData) {
            Error{} << "Cannot load scene, exiting";
            return;
        }

        /* Recursively add all children */
        for(UnsignedInt objectId: sceneData->children3D())
            addObject(*importer, _o, objectId);

    /* The format has no scene support, display just the first loaded mesh with
       default material and be done with it */
    } else if(_resourceManager.state<GL::Mesh>(ResourceKey{0}) == ResourceState::Final)
        new ColoredObject{ResourceKey{0}, ResourceKey(-1), _o, &_drawables};

    /* Materials were consumed by objects and they are not needed anymore. Also
       free all texture/mesh data that weren't referenced by any object. */
    _resourceManager.setFallback<Trade::PhongMaterialData>(nullptr)
        .clear<Trade::PhongMaterialData>()
        .free<GL::Texture2D>()
        .free<GL::Mesh>();
}

void ViewerExample::addObject(Trade::AbstractImporter& importer, Object3D* parent, UnsignedInt i) {
    Debug{} << "Importing object" << i << importer.object3DName(i);

    Object3D* object = nullptr;
    std::unique_ptr<Trade::ObjectData3D> objectData = importer.object3D(i);
    if(!objectData) {
        Error{} << "Cannot import object, skipping";
        return;
    }

    /* Only meshes for now */
    if(objectData->instanceType() == Trade::ObjectInstanceType3D::Mesh) {
        Int materialId = static_cast<Trade::MeshObjectData3D*>(objectData.get())->material();

        /* Decide what object to add based on material type */
        auto materialData = _resourceManager.get<Trade::PhongMaterialData>(ResourceKey(materialId));

        /* Color-only material */
        if(!materialData->flags()) {
            object = new ColoredObject(ResourceKey(objectData->instance()),
                                       ResourceKey(materialId),
                                       parent, &_drawables);
            object->setTransformation(objectData->transformation());

        /* Diffuse texture material */
        } else if(materialData->flags() == Trade::PhongMaterialData::Flag::DiffuseTexture) {
            object = new TexturedObject(ResourceKey(objectData->instance()),
                                        ResourceKey(materialId),
                                        ResourceKey(materialData->diffuseTexture()),
                                        parent, &_drawables);
            object->setTransformation(objectData->transformation());

        /* No other material types are supported yet */
        } else {
            Warning() << "Texture combination of material"
                      << materialId << importer.materialName(materialId)
                      << "is not supported, using default material instead";

            object = new ColoredObject(ResourceKey(objectData->instance()),
                                       ResourceKey(-1),
                                       parent, &_drawables);
            object->setTransformation(objectData->transformation());
        }
    }

    /* Create parent object for children, if it doesn't already exist */
    if(!object && !objectData->children().empty()) {
        object = new Object3D(parent);
        object->setTransformation(objectData->transformation());
    }

    /* Recursively add children */
    for(std::size_t id: objectData->children())
        addObject(importer, object, id);
}

void ViewerExample::viewportEvent(const Vector2i& size) {
    GL::defaultFramebuffer.setViewport({{}, size});
    _camera->setViewport(size);
}

void ViewerExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);
    _camera->draw(_drawables);
    swapBuffers();
}

void ViewerExample::mousePressEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left)
        _previousPosition = positionOnSphere(event.position());
}

void ViewerExample::mouseReleaseEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left)
        _previousPosition = Vector3();
}

void ViewerExample::mouseScrollEvent(MouseScrollEvent& event) {
    if(!event.offset().y()) return;

    /* Distance to origin */
    Float distance = _cameraObject->transformation().translation().z();

    /* Move 15% of the distance back or forward */
    distance *= 1 - (event.offset().y() > 0 ? 1/0.85f : 0.85f);
    _cameraObject->translate(Vector3::zAxis(distance));

    redraw();
}

Vector3 ViewerExample::positionOnSphere(const Vector2i& position) const {
    Vector2 positionNormalized = Vector2(position*2)/Vector2(_camera->viewport()) - Vector2(1.0f);

    Float length = positionNormalized.length();
    Vector3 result(length > 1.0f ? Vector3(positionNormalized, 0.0f) : Vector3(positionNormalized, 1.0f - length));
    result.y() *= -1.0f;
    return result.normalized();
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

ColoredObject::ColoredObject(ResourceKey meshId, ResourceKey materialId, Object3D* parent, SceneGraph::DrawableGroup3D* group):
    Object3D{parent}, SceneGraph::Drawable3D{*this, group},
    _mesh{ViewerResourceManager::instance().get<GL::Mesh>(meshId)}, _shader{ViewerResourceManager::instance().get<Shaders::Phong>("color")}
{
    auto material = ViewerResourceManager::instance().get<Trade::PhongMaterialData>(materialId);
    _ambientColor = material->ambientColor();
    _diffuseColor = material->diffuseColor();
    _specularColor = material->specularColor();
    _shininess = material->shininess();
}

TexturedObject::TexturedObject(ResourceKey meshId, ResourceKey materialId, ResourceKey diffuseTextureId, Object3D* parent, SceneGraph::DrawableGroup3D* group):
    Object3D{parent}, SceneGraph::Drawable3D{*this, group},
    _mesh{ViewerResourceManager::instance().get<GL::Mesh>(meshId)}, _diffuseTexture{ViewerResourceManager::instance().get<GL::Texture2D>(diffuseTextureId)}, _shader{ViewerResourceManager::instance().get<Shaders::Phong>("texture")}
{
    auto material = ViewerResourceManager::instance().get<Trade::PhongMaterialData>(materialId);
    _ambientColor = material->ambientColor();
    _specularColor = material->specularColor();
    _shininess = material->shininess();
}

void ColoredObject::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
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

void TexturedObject::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader->setAmbientColor(_ambientColor)
        .setSpecularColor(_specularColor)
        .setShininess(_shininess)
        .setLightPosition(camera.cameraMatrix().transformPoint({-3.0f, 10.0f, 10.0f}))
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.rotation())
        .setProjectionMatrix(camera.projectionMatrix())
        .bindDiffuseTexture(*_diffuseTexture);

    _mesh->draw(*_shader);
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::ViewerExample)
