/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020, 2021
             — Vladimír Vondruš <mosra@centrum.cz>

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

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Optional.h>
#include <Corrade/Containers/Reference.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/DebugStl.h>
#include <Magnum/ImageView.h>
#include <Magnum/Mesh.h>
#include <Magnum/DebugTools/FrameProfiler.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/MeshView.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureArray.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/Concatenate.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/Shaders/Generic.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/Trade/MeshObjectData3D.h>
#include <Magnum/Trade/PhongMaterialData.h>
#include <Magnum/Trade/SceneData.h>
#include <Magnum/Trade/TextureData.h>

namespace Magnum { namespace Examples {

using namespace Containers::Literals;
using namespace Math::Literals;

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

enum class DrawType {
    SceneGraph,
    DumbLoop,
    ImprovedLoop,
    ImprovedLoopViews,
    UboUploadEach,
    UboUploadOnceSetOffset,
    UboUploadOnceSetOffsetMeshViews,
    UboUploadOnceSetOffsetMultiDraw
};

class MultiDrawExample: public Platform::Application {
    public:
        explicit MultiDrawExample(const Arguments& arguments);

        /* Needs to be public to be called from C (which is called from JS) */
        bool setDrawType(DrawType type);

    private:
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void mouseScrollEvent(MouseScrollEvent& event) override;
        void keyPressEvent(KeyEvent& event) override;

        Vector3 positionOnSphere(const Vector2i& position) const;

        void addObject(Trade::AbstractImporter& importer, UnsignedInt i, Containers::ArrayView<const Shaders::TextureTransformationUniform> diffuseTexturesForMaterials);

        Shaders::PhongGL
            _shader{{}, 2},
            _texturedShader{Shaders::PhongGL::Flag::DiffuseTexture, 2},
            _shaderUniformBufferSingle{Shaders::PhongGL::Flag::UniformBuffers, 2},
            _texturedShaderUniformBufferSingle{Shaders::PhongGL::Flag::UniformBuffers|Shaders::PhongGL::Flag::DiffuseTexture, 2},
            _shaderUniformBufferMultiple{NoCreate},
            _shaderUniformBufferMultiDraw{NoCreate},
            _texturedShaderUniformBufferMultiple{NoCreate},
            _texturedShaderUniformBufferMultiDraw{NoCreate};
        Containers::Array<GL::Mesh> _meshes;
        GL::Mesh _combinedMesh;
        Containers::Array<GL::Texture2D> _textures;
        GL::Texture2DArray _combinedTexture{NoCreate};
        Containers::Optional<GL::MeshView> _emptyView;
        Containers::Array<GL::MeshView> _meshViews;

        struct {
            Scene3D scene;
            Object3D manipulator, cameraObject;
            SceneGraph::Camera3D* camera;
            SceneGraph::DrawableGroup3D drawables;
        } _sceneGraph;

        struct {
            Containers::Array<UnsignedInt> parents;
            Containers::Array<Matrix4> transformations;
            Containers::Array<GL::Mesh*> meshes;
            Containers::Array<Containers::Reference<GL::MeshView>> meshViews;

            Containers::Array<Shaders::ProjectionUniform3D> projections;
            Containers::Array<Shaders::TransformationUniform3D> absoluteTransformations;
            Containers::Array<Shaders::PhongDrawUniform> draws;
            Containers::Array<Shaders::TextureTransformationUniform> textureTransformations;
            Containers::Array<Shaders::PhongMaterialUniform> materials;
            Containers::Array<Shaders::PhongLightUniform> lights;
        } _direct;

        struct {
            GL::Buffer projectionUniform{GL::Buffer::TargetHint::Uniform, {
                Shaders::ProjectionUniform3D{}
            }};
            GL::Buffer transformationUniform{GL::Buffer::TargetHint::Uniform, {
                Shaders::TransformationUniform3D{}
            }};
            GL::Buffer drawUniform{GL::Buffer::TargetHint::Uniform, {
                Shaders::PhongDrawUniform{}
            }};
            GL::Buffer materialUniform{GL::Buffer::TargetHint::Uniform, {
                Shaders::PhongMaterialUniform{}
            }};
            GL::Buffer lightUniform{GL::Buffer::TargetHint::Uniform, {
                Shaders::PhongLightUniform{},
                Shaders::PhongLightUniform{}
            }};
        } _uniformSingle[5];
        UnsignedInt _uniformSingleFrameId = 0;

        constexpr static std::size_t UniformMultiCount = 3;
        struct {
            GL::Buffer projectionUniform;
            GL::Buffer materialUniform;

            GL::Buffer transformationUniform[UniformMultiCount];
            GL::Buffer transformationUniformStaging;

            GL::Buffer drawUniform[UniformMultiCount];
            GL::Buffer drawUniformStaging;

            GL::Buffer textureTransformationUniform[UniformMultiCount];
            GL::Buffer textureTransformationUniformStaging;

            GL::Buffer lightUniform[UniformMultiCount];
            GL::Buffer lightUniformStaging;
        } _uniformMulti;
        UnsignedInt _uniformMultiFrameId = 0;

        Vector3 _previousPosition;
        Matrix4
            _projection{Matrix4::perspectiveProjection(35.0_degf, 4.0f/3.0f, 0.01f, 1000.0f)},
            _cameraTransformation{Matrix4::translation(Vector3::zAxis(5.0f))},
            _manipulatorTransformation{Math::IdentityInit};
        DrawType _drawType = DrawType::SceneGraph;

        DebugTools::FrameProfilerGL _profiler{
            DebugTools::FrameProfilerGL::Value::FrameTime|
            DebugTools::FrameProfilerGL::Value::CpuDuration|
            DebugTools::FrameProfilerGL::Value::GpuDuration, 150};
};

class Drawable: public SceneGraph::Drawable3D {
    public:
        explicit Drawable(Object3D& object, Shaders::PhongGL& shader, GL::Mesh& mesh, const Color4& ambient, const Color4& diffuse, const Color3& specular, Float shininess, SceneGraph::DrawableGroup3D& group): SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh), _ambient{ambient}, _diffuse{diffuse}, _specular{specular}, _shininess{shininess} {}

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Shaders::PhongGL& _shader;
        GL::Mesh& _mesh;
        Color4 _ambient, _diffuse;
        Color3 _specular;
        Float _shininess;
};

class TexturedDrawable: public SceneGraph::Drawable3D {
    public:
        explicit TexturedDrawable(Object3D& object, Shaders::PhongGL& shader, GL::Mesh& mesh, GL::Texture2D& diffuseTexture, const Color4& ambient, const Color4& diffuse, const Color3& specular, Float shininess, SceneGraph::DrawableGroup3D& group): SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh), _diffuseTexture(diffuseTexture), _ambient{ambient}, _diffuse{diffuse}, _specular{specular}, _shininess{shininess} {}

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Shaders::PhongGL& _shader;
        GL::Mesh& _mesh;
        GL::Texture2D& _diffuseTexture;
        Color4 _ambient, _diffuse;
        Color3 _specular;
        Float _shininess;
};

MultiDrawExample::MultiDrawExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}
        .setTitle("Magnum Multi Draw Example")}
{
    Utility::Arguments args;
    args.addArgument("file").setHelp("file", "file to load")
        .addBooleanOption("no-profile").setHelp("no-profile", "don't enable profiler on startup")
        .addSkippedPrefix("magnum", "engine-specific options")
        .parse(arguments.argc, arguments.argv);

    /* Setup renderer defaults */
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    /* Load a file */
    PluginManager::Manager<Trade::AbstractImporter> manager;
    Containers::Pointer<Trade::AbstractImporter> importer = manager.loadAndInstantiate("TinyGltfImporter");
    if(!importer || !importer->openFile(args.value("file")))
        std::exit(4);

    #ifdef CORRADE_IS_DEBUG_BUILD
    Debug{} << Debug::boldColor(Debug::Color::Yellow) << "Running a debug build.";
    #endif

    Debug{} << "Material count:" << importer->materialCount();
    Debug{} << "Mesh count:" << importer->meshCount();
    Debug{} << "Object count:" << importer->object3DCount();
    Debug{} << "Texture count:" << importer->textureCount();

    /* Load just the basic color info from all materials */
    _direct.materials = Containers::Array<Shaders::PhongMaterialUniform>{ValueInit, importer->materialCount()};
    Containers::Array<Shaders::TextureTransformationUniform> diffuseTexturesForMaterials{DirectInit, importer->materialCount(), Shaders::TextureTransformationUniform{}.setLayer(-1)};
    for(UnsignedInt i = 0; i != importer->materialCount(); ++i) {
        // TODO: fix the macro to correctly propagate &&
        const Containers::Optional<Trade::MaterialData> material = importer->material(i);
        auto&& phong = CORRADE_INTERNAL_ASSERT_EXPRESSION(material)->as<Trade::PhongMaterialData>();
        _direct.materials[i].ambientColor = phong.ambientColor();
        _direct.materials[i].diffuseColor = phong.diffuseColor();
        _direct.materials[i].specularColor = phong.specularColor();

        if(phong.hasAttribute(Trade::MaterialAttribute::DiffuseTexture))
            diffuseTexturesForMaterials[i].setLayer(phong.diffuseTexture())
                .setTextureMatrix(phong.diffuseTextureMatrix());
    }

    /* Load all meshes */
    Containers::Array<Trade::MeshData> meshData;
    arrayReserve(meshData, importer->meshCount());
    Containers::Array<Containers::Reference<const Trade::MeshData>> meshDataReferences;
    _meshes = Containers::Array<GL::Mesh>{importer->meshCount()};
    Containers::Array<UnsignedInt> meshOffsets{importer->meshCount() + 1};
    UnsignedInt offset = 0;
    meshOffsets[0] = 0;
    for(UnsignedInt i = 0; i != importer->meshCount(); ++i) {
        arrayAppend(meshData, *importer->mesh(i));
        arrayAppend(meshDataReferences, InPlaceInit, meshData[i]);
        _meshes[i] = MeshTools::compile(meshData[i]);

        offset += meshData[i].indexCount();
        meshOffsets[i + 1] = offset;
    }
    _combinedMesh = MeshTools::compile(MeshTools::concatenate(meshDataReferences));
    Debug{} << "Combined mesh index count:" << _combinedMesh.count();
    _emptyView.emplace(_combinedMesh).setCount(0);
    for(UnsignedInt i = 0; i != _meshes.size(); ++i) {
        arrayAppend(_meshViews, GL::MeshView{_combinedMesh})
            .setIndexRange(meshOffsets[i])
            .setCount(meshOffsets[i + 1] - meshOffsets[i]);
    }

    /* Load all textures */
    _textures = Containers::Array<GL::Texture2D>{importer->textureCount()};
    Containers::Array<UnsignedInt> textureToImageMapping{importer->textureCount()};
    for(UnsignedInt i = 0; i != importer->textureCount(); ++i) {
        Trade::TextureData texture = *CORRADE_INTERNAL_ASSERT_EXPRESSION(importer->texture(i));

        textureToImageMapping[i] = texture.image();

        Trade::ImageData2D image = *CORRADE_INTERNAL_ASSERT_EXPRESSION(importer->image2D(texture.image()));
        _textures[i]
            .setMagnificationFilter(texture.magnificationFilter())
            .setMinificationFilter(texture.magnificationFilter())
            .setWrapping(texture.wrapping().xy())
            .setStorage(Math::log2(image.size().max()) + 1, GL::textureFormat(image.format()), image.size())
            .setSubImage(0, {}, image)
            .generateMipmap();

        /* Use properties of the first texture/image for the combined texture
           size and sampler options */
        if(!_combinedTexture.id()) {
            Debug{} << image.size() << importer->textureCount();
            _combinedTexture = GL::Texture2DArray{};
            _combinedTexture
                .setMagnificationFilter(texture.magnificationFilter())
                .setMinificationFilter(texture.magnificationFilter())
                .setWrapping(texture.wrapping().xy())
                .setStorage(Math::log2(image.size().max()) + 1, GL::textureFormat(image.format()), {image.size(), Int(importer->textureCount())});
        }

        _combinedTexture.setSubImage(0, {0, 0, Int(i)}, ImageView2D{image});
    }

    _combinedTexture.generateMipmap();

    /* Load the scene. Has to be done recursively in order to have parents
       ordered before children. */
    _direct.transformations = Containers::Array<Matrix4>{NoInit, importer->object3DCount()};
    _direct.absoluteTransformations = Containers::Array<Shaders::TransformationUniform3D>{NoInit, importer->object3DCount() + 1};
    _direct.draws = Containers::Array<Shaders::PhongDrawUniform>{ValueInit, importer->object3DCount()};
    _direct.textureTransformations = Containers::Array<Shaders::TextureTransformationUniform>{ValueInit, importer->object3DCount()};
    arrayReserve(_direct.parents, importer->object3DCount());
    _direct.meshes = Containers::Array<GL::Mesh*>{ValueInit, importer->object3DCount()};
    _direct.meshViews = Containers::Array<Containers::Reference<GL::MeshView>>{DirectInit, importer->object3DCount(), *_emptyView};
    const Trade::SceneData scene = *CORRADE_INTERNAL_ASSERT_EXPRESSION(importer->scene(importer->defaultScene()));
    for(const UnsignedInt objectId: scene.children3D()) {
        arrayAppend(_direct.parents, 0xffffffffu);
        addObject(*importer, objectId, diffuseTexturesForMaterials);
    }

    /* Projection, light setup. Just two lights right now. */
    _direct.projections = Containers::Array<Shaders::ProjectionUniform3D>{ValueInit, 1};
    _direct.lights = Containers::Array<Shaders::PhongLightUniform>{ValueInit, 2};

    /* Set up a multi-draw shader and uniform storage based on the data count
       we have */
    _shaderUniformBufferMultiple = Shaders::PhongGL{
        Shaders::PhongGL::Flag::UniformBuffers|Shaders::PhongGL::Flag::DiffuseTexture|Shaders::PhongGL::Flag::TextureArrays|Shaders::PhongGL::Flag::TextureTransformation,
        UnsignedInt(_direct.lights.size()),
        UnsignedInt(_direct.materials.size()),
        UnsignedInt(_direct.draws.size())
    };
    _shaderUniformBufferMultiDraw = Shaders::PhongGL{
        Shaders::PhongGL::Flag::UniformBuffers|Shaders::PhongGL::Flag::MultiDraw|Shaders::PhongGL::Flag::DiffuseTexture|Shaders::PhongGL::Flag::TextureArrays|Shaders::PhongGL::Flag::TextureTransformation,
        UnsignedInt(_direct.lights.size()),
        UnsignedInt(_direct.materials.size()),
        UnsignedInt(_direct.draws.size())
    };
    _direct.projections[0].projectionMatrix = _projection;
    #ifndef MAGNUM_TARGET_GLES
    if(GL::Context::current().isExtensionSupported<GL::Extensions::ARB::buffer_storage>()) {
        _uniformMulti.projectionUniform.setStorage(_direct.projections, {});
        _uniformMulti.materialUniform.setStorage(_direct.materials, {});
        for(std::size_t i = 0; i != UniformMultiCount; ++i) {
            _uniformMulti.transformationUniform[i].setStorage(_direct.draws.size()*sizeof(Shaders::TransformationUniform3D), {});
            _uniformMulti.drawUniform[i].setStorage(_direct.draws.size()*sizeof(Shaders::PhongDrawUniform), {});
            _uniformMulti.textureTransformationUniform[i].setStorage(_direct.textureTransformations.size()*sizeof(Shaders::TextureTransformationUniform), {});
            _uniformMulti.lightUniform[i].setStorage(_direct.lights.size()*sizeof(Shaders::PhongLightUniform), {});
        }
        _uniformMulti.transformationUniformStaging.setStorage(_direct.transformations.size()*sizeof(Shaders::TransformationUniform3D), GL::Buffer::StorageFlag::DynamicStorage);
        _uniformMulti.drawUniformStaging.setStorage(_direct.draws.size()*sizeof(Shaders::PhongDrawUniform), GL::Buffer::StorageFlag::DynamicStorage);
        _uniformMulti.textureTransformationUniformStaging.setStorage(_direct.textureTransformations.size()*sizeof(Shaders::TextureTransformationUniform), GL::Buffer::StorageFlag::DynamicStorage);
        _uniformMulti.lightUniformStaging.setStorage(_direct.lights.size()*sizeof(Shaders::PhongLightUniform), GL::Buffer::StorageFlag::DynamicStorage);
    } else
    #endif
    {
        _uniformMulti.projectionUniform.setData(_direct.projections);
        _uniformMulti.materialUniform.setData(_direct.materials);
        for(std::size_t i = 0; i != UniformMultiCount; ++i) {
            _uniformMulti.transformationUniform[i].setData({nullptr, _direct.draws.size()*sizeof(Shaders::TransformationUniform3D)});
            _uniformMulti.drawUniform[i].setData({nullptr, _direct.draws.size()*sizeof(Shaders::PhongDrawUniform)});
            _uniformMulti.textureTransformationUniform[i].setData({nullptr, _direct.textureTransformations.size()*sizeof(Shaders::TextureTransformationUniform)});
            _uniformMulti.lightUniform[i].setData({nullptr, _direct.lights.size()*sizeof(Shaders::PhongLightUniform)});
        }
        _uniformMulti.transformationUniformStaging.setData({nullptr, _direct.transformations.size()*sizeof(Shaders::TransformationUniform3D)}, GL::BufferUsage::DynamicDraw);
        _uniformMulti.drawUniformStaging.setData({nullptr, _direct.draws.size()*sizeof(Shaders::PhongDrawUniform)}, GL::BufferUsage::DynamicDraw);
        _uniformMulti.textureTransformationUniformStaging.setData({nullptr, _direct.textureTransformations.size()*sizeof(Shaders::TextureTransformationUniform)}, GL::BufferUsage::DynamicDraw);
        _uniformMulti.lightUniformStaging.setData({nullptr, _direct.lights.size()*sizeof(Shaders::PhongLightUniform)}, GL::BufferUsage::DynamicDraw);
    }

    /* SceneGraph setup. Uhh, so much typing. */
    {
        _sceneGraph.cameraObject
            .setParent(&_sceneGraph.scene);
        (*(_sceneGraph.camera = new SceneGraph::Camera3D{_sceneGraph.cameraObject}))
            .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
            .setProjectionMatrix(_projection)
            .setViewport(GL::defaultFramebuffer.viewport().size());

        /* Base object, parent of all (for easy manipulation) */
        _sceneGraph.manipulator.setParent(&_sceneGraph.scene);

        /* Create objects based on the imported info */
        Containers::Array<Object3D*> parentPointers{NoInit, _direct.parents.size()};
        for(std::size_t i = 0; i != _direct.parents.size(); ++i) {
            Object3D* o = new Object3D;;
            o->setTransformation(_direct.transformations[i]);
            if(_direct.parents[i] == 0xffffffffu) {
                o->setParent(&_sceneGraph.manipulator);
            } else {
                CORRADE_INTERNAL_ASSERT(_direct.parents[i] < i);
                o->setParent(parentPointers[_direct.parents[i]]);
            }

            parentPointers[i] = o;

            if(_direct.meshes[i]) {
                if(Int(diffuseTexturesForMaterials[_direct.draws[i].materialId].layer) != -1)
                    new TexturedDrawable{*o, _texturedShader, *_direct.meshes[i],
                        _textures[textureToImageMapping[diffuseTexturesForMaterials[_direct.draws[i].materialId].layer]],
                        _direct.materials[_direct.draws[i].materialId].ambientColor,
                        _direct.materials[_direct.draws[i].materialId].diffuseColor,
                        _direct.materials[_direct.draws[i].materialId].specularColor.rgb(),
                        _direct.materials[_direct.draws[i].materialId].shininess,
                        _sceneGraph.drawables};
                else new Drawable{*o, _shader, *_direct.meshes[i],
                    _direct.materials[_direct.draws[i].materialId].ambientColor,
                    _direct.materials[_direct.draws[i].materialId].diffuseColor,
                    _direct.materials[_direct.draws[i].materialId].specularColor.rgb(),
                    _direct.materials[_direct.draws[i].materialId].shininess,
                    _sceneGraph.drawables};
            }
        }
    }

    if(args.isSet("no-profile")) _profiler.disable();

    /* Initially we're drawing with the SceneGraph */
    Debug{} << "Using a SceneGraph";
}

void MultiDrawExample::addObject(Trade::AbstractImporter& importer, UnsignedInt i, Containers::ArrayView<const Shaders::TextureTransformationUniform> diffuseTexturesForMaterials) {
    Containers::Pointer<Trade::ObjectData3D> objectData = importer.object3D(i);
    CORRADE_INTERNAL_ASSERT(objectData);

    const UnsignedInt orderedId = _direct.parents.size() - 1;

    _direct.transformations[orderedId] = objectData->transformation();

    /* Add a drawable if the object has a mesh and the mesh is loaded */
    if(objectData->instanceType() == Trade::ObjectInstanceType3D::Mesh) {
        CORRADE_INTERNAL_ASSERT(objectData->instance() != -1);
        const Int materialId = static_cast<Trade::MeshObjectData3D*>(objectData.get())->material();
        CORRADE_INTERNAL_ASSERT(materialId != -1);

        _direct.draws[orderedId].materialId = materialId;
        _direct.meshes[orderedId] = &_meshes[objectData->instance()];
        _direct.meshViews[orderedId] = _meshViews[objectData->instance()];
        _direct.textureTransformations[orderedId] = diffuseTexturesForMaterials[materialId];
    }

    /* Recursively add children */
    for(std::size_t child: objectData->children()) {
        arrayAppend(_direct.parents, orderedId);
        addObject(importer, child, diffuseTexturesForMaterials);
    }
}

void Drawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setAmbientColor(_ambient)
        .setDiffuseColor(_diffuse)
        .setSpecularColor(_specular)
        .setShininess(_shininess)
        .setLightPositions({
            {-300.0f, 100.0f, 100.0f, 0.0f},
            {300.0f, 100.0f, 100.0f, 0.0f}
        })
        .setLightColors({0xffffff_rgbf, 0xffffff_rgbf})
        .setLightSpecularColors({0xffffff_rgbf, 0xffffff_rgbf})
        .setLightRanges({Constants::inf(), Constants::inf()})
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .draw(_mesh);
}

void TexturedDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setAmbientColor(_ambient)
        .setDiffuseColor(_diffuse)
        .setSpecularColor(_specular)
        .setShininess(_shininess)
        .setLightPositions({
            {-300.0f, 100.0f, 100.0f, 0.0f},
            {300.0f, 100.0f, 100.0f, 0.0f}
        })
        .setLightColors({0xffffff_rgbf, 0xffffff_rgbf})
        .setLightSpecularColors({0xffffff_rgbf, 0xffffff_rgbf})
        .setLightRanges({Constants::inf(), Constants::inf()})
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.normalMatrix())
        .setProjectionMatrix(camera.projectionMatrix())
        .bindDiffuseTexture(_diffuseTexture)
        .draw(_mesh);
}

void MultiDrawExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    _profiler.beginFrame();

    /* Draw using the scene graph */
    if(_drawType == DrawType::SceneGraph) {
        _sceneGraph.manipulator.setTransformation(_manipulatorTransformation);
        _sceneGraph.cameraObject.setTransformation(_cameraTransformation);
        _sceneGraph.camera->draw(_sceneGraph.drawables);

    /* Otherwise calculate absolute transformations first */
    } else {
        CORRADE_INTERNAL_ASSERT(_direct.parents.size() == _direct.transformations.size());
        CORRADE_INTERNAL_ASSERT(_direct.parents.size() == _direct.draws.size());
        CORRADE_INTERNAL_ASSERT(_direct.parents.size() == _direct.meshes.size());
        const std::size_t objectCount = _direct.parents.size();

        const Matrix4 cameraMatrix = _cameraTransformation.invertedRigid();
        _direct.absoluteTransformations[0].transformationMatrix = cameraMatrix*_manipulatorTransformation;
        for(std::size_t i = 0; i != objectCount; ++i) {
            _direct.absoluteTransformations[i + 1].transformationMatrix = _direct.absoluteTransformations[_direct.parents[i] + 1].transformationMatrix*_direct.transformations[i];
        }
        for(std::size_t i = 0; i != objectCount; ++i) {
            _direct.draws[i].setNormalMatrix(_direct.absoluteTransformations[i + 1].transformationMatrix.normalMatrix());
        }

        _direct.lights[0].position = {-300.0f, 100.0f, 100.0f, 0.0f};
        _direct.lights[1].position = {300.0f, 100.0f, 100.0f, 0.0f};

        /* Render all objects that have a mesh in a simple loop */
        if(_drawType == DrawType::DumbLoop) {
            for(std::size_t i = 0; i != objectCount; ++i) {
                if(!_direct.meshes[i]) continue;
                const std::size_t materialId = _direct.draws[i].materialId;
                _shader
                    .setAmbientColor(_direct.materials[materialId].ambientColor)
                    .setDiffuseColor(_direct.materials[materialId].diffuseColor)
                    .setSpecularColor(_direct.materials[materialId].specularColor)
                    .setShininess(_direct.materials[materialId].shininess)
                    .setLightPositions({_direct.lights[0].position,
                                        _direct.lights[1].position})
                    .setLightColors({_direct.lights[0].color,
                                     _direct.lights[1].color})
                    .setLightSpecularColors({_direct.lights[0].specularColor,
                                             _direct.lights[1].specularColor})
                    .setLightRanges({_direct.lights[0].range,
                                     _direct.lights[1].range})
                    .setTransformationMatrix(_direct.absoluteTransformations[i + 1].transformationMatrix)
                    .setNormalMatrix({Vector4{_direct.draws[i].normalMatrix[0]}.xyz(),
                                      Vector4{_direct.draws[i].normalMatrix[1]}.xyz(),
                                      Vector4{_direct.draws[i].normalMatrix[2]}.xyz()})
                    .setProjectionMatrix(_projection)
                    .draw(*_direct.meshes[i]);
            }

        } else if(_drawType == DrawType::ImprovedLoop) {
            _shader
                .setProjectionMatrix(_projection)
                .setLightPositions({_direct.lights[0].position,
                                    _direct.lights[1].position})
                .setLightColors({_direct.lights[0].color,
                                 _direct.lights[1].color})
                .setLightSpecularColors({_direct.lights[0].specularColor,
                                         _direct.lights[1].specularColor})
                .setLightRanges({_direct.lights[0].range,
                                 _direct.lights[1].range});

            for(std::size_t i = 0; i != objectCount; ++i) {
                if(!_direct.meshes[i]) continue;
                const std::size_t materialId = _direct.draws[i].materialId;
                _shader
                    .setAmbientColor(_direct.materials[materialId].ambientColor)
                    .setDiffuseColor(_direct.materials[materialId].diffuseColor)
                    .setSpecularColor(_direct.materials[materialId].specularColor)
                    .setShininess(_direct.materials[materialId].shininess)
                    .setTransformationMatrix(_direct.absoluteTransformations[i + 1].transformationMatrix)
                    .setNormalMatrix({Vector4{_direct.draws[i].normalMatrix[0]}.xyz(),
                                      Vector4{_direct.draws[i].normalMatrix[1]}.xyz(),
                                      Vector4{_direct.draws[i].normalMatrix[2]}.xyz()})
                    .draw(*_direct.meshes[i]);
            }

        } else if(_drawType == DrawType::ImprovedLoopViews) {
            _shader
                .setProjectionMatrix(_projection)
                .setLightPositions({_direct.lights[0].position,
                                    _direct.lights[1].position})
                .setLightColors({_direct.lights[0].color,
                                 _direct.lights[1].color})
                .setLightSpecularColors({_direct.lights[0].specularColor,
                                         _direct.lights[1].specularColor})
                .setLightRanges({_direct.lights[0].range,
                                 _direct.lights[1].range});

            for(std::size_t i = 0; i != objectCount; ++i) {
                if(!_direct.meshViews[i]->count()) continue;
                const std::size_t materialId = _direct.draws[i].materialId;
                _shader
                    .setAmbientColor(_direct.materials[materialId].ambientColor)
                    .setDiffuseColor(_direct.materials[materialId].diffuseColor)
                    .setSpecularColor(_direct.materials[materialId].specularColor)
                    .setShininess(_direct.materials[materialId].shininess)
                    .setTransformationMatrix(_direct.absoluteTransformations[i + 1].transformationMatrix)
                    .setNormalMatrix({Vector4{_direct.draws[i].normalMatrix[0]}.xyz(),
                                      Vector4{_direct.draws[i].normalMatrix[1]}.xyz(),
                                      Vector4{_direct.draws[i].normalMatrix[2]}.xyz()})
                    .draw(*_direct.meshViews[i]);
            }

        } else if(_drawType == DrawType::UboUploadEach) {
            _uniformSingle[_uniformSingleFrameId].projectionUniform.setSubData(0, _direct.projections);
            _uniformSingle[_uniformSingleFrameId].lightUniform.setSubData(0, _direct.lights);
            _shaderUniformBufferSingle
                .bindProjectionBuffer(_uniformSingle[_uniformSingleFrameId].projectionUniform)
                .bindLightBuffer(_uniformSingle[_uniformSingleFrameId].lightUniform);

            for(std::size_t i = 0; i != objectCount; ++i) {
                if(!_direct.meshes[i]) continue;
                _uniformSingle[_uniformSingleFrameId].transformationUniform.setSubData(0, _direct.absoluteTransformations.slice<1>(i + 1));
                _uniformSingle[_uniformSingleFrameId].materialUniform.setSubData(0, _direct.materials.slice<1>(_direct.draws[i].materialId));
                /* We're uploading a single material, so patch the material ID
                   to be 0 */
                Shaders::PhongDrawUniform draw = _direct.draws[i];
                draw.materialId = 0;
                _uniformSingle[_uniformSingleFrameId].drawUniform.setSubData(0, {draw});
                _shaderUniformBufferSingle
                    .bindTransformationBuffer(_uniformSingle[_uniformSingleFrameId].transformationUniform)
                    .bindMaterialBuffer(_uniformSingle[_uniformSingleFrameId].materialUniform)
                    .bindDrawBuffer(_uniformSingle[_uniformSingleFrameId].drawUniform)
                    .draw(*_direct.meshes[i]);
            }

            _uniformSingleFrameId = (_uniformSingleFrameId + 1) % Containers::arraySize(_uniformSingle);

        } else if(_drawType == DrawType::UboUploadOnceSetOffset || _drawType == DrawType::UboUploadOnceSetOffsetMeshViews || _drawType == DrawType::UboUploadOnceSetOffsetMultiDraw) {
            #ifndef MAGNUM_TARGET_GLES
            if(GL::Context::current().isExtensionSupported<GL::Extensions::ARB::buffer_storage>()) {
                _uniformMulti.lightUniformStaging.setSubData(0, _direct.lights);
                _uniformMulti.transformationUniformStaging.setSubData(0, _direct.absoluteTransformations.suffix(1));
                _uniformMulti.drawUniformStaging.setSubData(0, _direct.draws);
                _uniformMulti.textureTransformationUniformStaging.setSubData(0, _direct.textureTransformations);
                GL::Buffer::copy(
                    _uniformMulti.lightUniformStaging,
                    _uniformMulti.lightUniform[_uniformMultiFrameId],
                    0, 0, _uniformMulti.lightUniformStaging.size());
                GL::Buffer::copy(
                    _uniformMulti.transformationUniformStaging,
                    _uniformMulti.transformationUniform[_uniformMultiFrameId],
                    0, 0, _uniformMulti.transformationUniformStaging.size());
                GL::Buffer::copy(
                    _uniformMulti.drawUniformStaging,
                    _uniformMulti.drawUniform[_uniformMultiFrameId],
                    0, 0, _uniformMulti.drawUniformStaging.size());
                GL::Buffer::copy(
                    _uniformMulti.textureTransformationUniformStaging,
                    _uniformMulti.textureTransformationUniform[_uniformMultiFrameId],
                    0, 0, _uniformMulti.textureTransformationUniformStaging.size());
            } else
            #endif
            {
                _uniformMulti.lightUniform[_uniformMultiFrameId].setSubData(0, _direct.lights);
                _uniformMulti.transformationUniform[_uniformMultiFrameId].setSubData(0, _direct.absoluteTransformations.suffix(1));
                _uniformMulti.drawUniform[_uniformMultiFrameId].setSubData(0, _direct.draws);
                _uniformMulti.textureTransformationUniform[_uniformMultiFrameId].setSubData(0, _direct.textureTransformations);
            }

            ((_drawType == DrawType::UboUploadOnceSetOffset || _drawType == DrawType::UboUploadOnceSetOffsetMeshViews) ? _shaderUniformBufferMultiple : _shaderUniformBufferMultiDraw)
                .bindProjectionBuffer(_uniformMulti.projectionUniform)
                .bindMaterialBuffer(_uniformMulti.materialUniform)
                .bindLightBuffer(_uniformMulti.lightUniform[_uniformMultiFrameId])
                .bindTransformationBuffer(_uniformMulti.transformationUniform[_uniformMultiFrameId])
                .bindDrawBuffer(_uniformMulti.drawUniform[_uniformMultiFrameId])
                .bindTextureTransformationBuffer(_uniformMulti.textureTransformationUniform[_uniformMultiFrameId])
                .bindDiffuseTexture(_combinedTexture);

            if(_drawType == DrawType::UboUploadOnceSetOffset) for(std::size_t i = 0; i != objectCount; ++i) {
                if(!_direct.meshes[i]) continue;
                _shaderUniformBufferMultiple
                    .setDrawOffset(i)
                    .draw(*_direct.meshes[i]);
            } else if(_drawType == DrawType::UboUploadOnceSetOffsetMeshViews) for(std::size_t i = 0; i != objectCount; ++i) {
                if(!_direct.meshViews[i]->count()) continue;
                _shaderUniformBufferMultiple
                    .setDrawOffset(i)
                    .draw(_direct.meshViews[i]);
            } else if(_drawType == DrawType::UboUploadOnceSetOffsetMultiDraw) {
                _shaderUniformBufferMultiDraw.draw(_direct.meshViews);
            }

            _uniformMultiFrameId = (_uniformMultiFrameId + 1) % UniformMultiCount;

        } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }

    _profiler.endFrame();
    _profiler.printStatistics(50);

    swapBuffers();
    if(_profiler.isEnabled()) redraw();
}

void MultiDrawExample::mousePressEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left)
        _previousPosition = positionOnSphere(event.position());
}

void MultiDrawExample::mouseReleaseEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left)
        _previousPosition = Vector3{};
}

void MultiDrawExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    const Vector3 currentPosition = positionOnSphere(event.position());
    const Vector3 axis = Math::cross(_previousPosition, currentPosition);

    if(_previousPosition.length() < 0.001f || axis.length() < 0.001f) return;

    _manipulatorTransformation =
        Matrix4::rotation(Math::angle(_previousPosition, currentPosition), axis.normalized())*
        _manipulatorTransformation;
    _previousPosition = currentPosition;

    redraw();
}

void MultiDrawExample::mouseScrollEvent(MouseScrollEvent& event) {
    if(!event.offset().y()) return;

    /* Distance to origin */
    const Float distance = _cameraTransformation.translation().z();

    /* Move 15% of the distance back or forward */
    _cameraTransformation =
        Matrix4::translation(Vector3::zAxis(
        distance*(1.0f - (event.offset().y() > 0 ? 1/0.85f : 0.85f))))*
        _cameraTransformation;

    redraw();
}

Vector3 MultiDrawExample::positionOnSphere(const Vector2i& position) const {
    const Vector2 positionNormalized = Vector2{position}/Vector2{windowSize()} - Vector2{0.5f};
    const Float length = positionNormalized.length();
    const Vector3 result(length > 1.0f ? Vector3(positionNormalized, 0.0f) : Vector3(positionNormalized, 1.0f - length));
    return (result*Vector3::yScale(-1.0f)).normalized();
}

void MultiDrawExample::keyPressEvent(KeyEvent& event) {
    if(event.key() == KeyEvent::Key::S && !setDrawType(DrawType::SceneGraph))
        return;
    else if(event.key() == KeyEvent::Key::D && !setDrawType(DrawType::DumbLoop))
        return;
    else if(event.key() == KeyEvent::Key::I && !setDrawType(DrawType::ImprovedLoop))
        return;
    else if(event.key() == KeyEvent::Key::W && !setDrawType(DrawType::ImprovedLoopViews))
        return;
    else if(event.key() == KeyEvent::Key::U && !setDrawType(DrawType::UboUploadEach))
        return;
    else if(event.key() == KeyEvent::Key::O && !setDrawType(DrawType::UboUploadOnceSetOffset))
        return;
    else if(event.key() == KeyEvent::Key::V && !setDrawType(DrawType::UboUploadOnceSetOffsetMeshViews))
        return;
    else if(event.key() == KeyEvent::Key::M && !setDrawType(DrawType::UboUploadOnceSetOffsetMultiDraw))
        return;
    else if(event.key() == KeyEvent::Key::P) {
        _profiler.isEnabled() ? _profiler.disable() : _profiler.enable();
        Debug{} << "Toggling the profiler to" << _profiler.isEnabled();
    } else return;

    redraw();
}

namespace {

Containers::StringView drawTypeToString(const DrawType type) {
    switch(type) {
        case DrawType::SceneGraph:
            return "Using a SceneGraph"_s;
        case DrawType::DumbLoop:
            return "Using a dumb direct loop"_s;
        case DrawType::ImprovedLoop:
            return "Using a direct loop, setting certain uniforms just once";
        case DrawType::ImprovedLoopViews:
            return "Using a direct loop, setting certain uniforms just once, drawing views";
        case DrawType::UboUploadEach:
            return "Using a direct loop + UBOs uploaded for each";
        case DrawType::UboUploadOnceSetOffset:
            return "Using a direct loop + UBOs uploaded once and setting draw offset";
        case DrawType::UboUploadOnceSetOffsetMeshViews:
            return "Using a direct loop + UBOs uploaded once, drawing views with offset";
        case DrawType::UboUploadOnceSetOffsetMultiDraw:
            return "Using UBOs + multidraw";
    }

    CORRADE_INTERNAL_ASSERT_UNREACHABLE();
}

}

bool MultiDrawExample::setDrawType(DrawType type) {
    if(_drawType == type) return false;

    _drawType = type;
    #ifndef CORRADE_TARGET_EMSCRIPTEN
    Debug{} << drawTypeToString(type);
    #endif
    if(_profiler.isEnabled()) _profiler.enable();
    #ifdef CORRADE_TARGET_EMSCRIPTEN
    updateOverlay();
    #endif
    return true;
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::MultiDrawExample)
