/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025, 2026
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
#include <Corrade/Containers/StaticArray.h>
#include <Corrade/PluginManager/Manager.h>
#include <Corrade/Utility/Algorithms.h>
#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/Format.h>
#ifdef CORRADE_TARGET_EMSCRIPTEN
#include <Corrade/Utility/Resource.h>
#endif
#include <Magnum/Mesh.h>
#include <Magnum/DebugTools/FrameProfiler.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Extensions.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/MeshView.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/TimeStl.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/Concatenate.h>
#include <Magnum/Platform/Gesture.h>
#ifdef CORRADE_TARGET_EMSCRIPTEN
#include <Magnum/Platform/EmscriptenApplication.h>
#else
#include <Magnum/Platform/Sdl2Application.h>
#endif
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneTools/Hierarchy.h>
#include <Magnum/Shaders/Generic.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Text/AbstractFont.h> /** @todo remove once extra glyph cache fill is done better */
#include <Magnum/Text/AbstractGlyphCache.h> /** @todo remove once extra glyph cache fill is done better */
#include <Magnum/Text/Alignment.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/Trade/MeshObjectData3D.h>
#include <Magnum/Trade/PhongMaterialData.h>
#include <Magnum/Trade/SceneData.h>
#include <Magnum/Ui/Anchor.h>
#include <Magnum/Ui/Application.h>
#include <Magnum/Ui/Checkbox.h>
#include <Magnum/Ui/EnumStorage.h>
#include <Magnum/Ui/Label.h>
#include <Magnum/Ui/NodeFlags.h>
#include <Magnum/Ui/SnapLayout.h>
#include <Magnum/Ui/SnapLayouter.h>
#include <Magnum/Ui/TextLayer.h> /** @todo remove once extra glyph cache fill is done better */
#include <Magnum/Ui/TextProperties.h>
#include <Magnum/Ui/Theme.h>
#include <Magnum/Ui/UserInterfaceGL.h>

namespace Magnum { namespace Examples { namespace {

using namespace Containers::Literals;
using namespace Math::Literals;

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

enum class DrawType {
    /* Classic SceneGraph. Every draw treated individually, no data sharing
       among them. */
    SceneGraph,
    /* Like above, but replacing the SceneGraph machinery with a direct
       calculation of hierarchic transformations and then a trivial loop doing
       exactly the same as the Drawable implementation in the SceneGraph. */
    TrivialLoop,
    /* The above loop but with common uniforms for projection and lighting set
       just once instead of for every draw */
    DeduplicatedLoop,
    /* The above loop, but using views on a single mesh instead of individual
       meshes. Mesh views are then used in all following cases as well. */
    DeduplicatedLoopMeshViews,
    /* Using uniform buffers instead of immediate uniforms and uploading their
       per-draw contents immediately before each draw. Depending on the GPU and
       driver UBOs used in such trivial way may or may not be faster than
       immediate uniforms. */
    UboPerDraw,
    /* Like above but uploading large UBOs containing all draws just once and
       then rebinding them with an offset before each draw. Available only if
       UBO bind alignment is 32 bytes or less, with larger alignment the
       uniform buffer items would need to have extra padding. */
    UboBindOffset,
    /* Like above but binding the UBOs just once before, using a shader that
       can handle multiple draws (and multiple materials), and setting a draw
       offset. This significantly reduces the CPU / driver overhead but means
       the GPU has to fetch per-draw data from an uniformly dynamic location
       and material data from a fully dynamic location, which depending on the
       GPU may be slower than UBO access from a static location in case of
       UboPerDraw and UboBindOffset above. */
    UboDrawOffset,
    // TODO single material? / material from offset?
    /* Like UboDrawOffset but calling a multi-draw command instead of looping
       over all draws, resulting in further reduction of CPU / driver overhead.
       Compared to the above the GPU now has to fetch *all* uniform data from
       dynamic locations, further pushing the tradeoff between CPU and GPU time
       spent. Available only if the driver supports gl_DrawID. */
    MultiDraw
};

class MultiDrawExample: public Platform::Application {
    public:
        explicit MultiDrawExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void pointerPressEvent(PointerEvent& event) override;
        void pointerReleaseEvent(PointerEvent& event) override;
        void pointerMoveEvent(PointerMoveEvent& event) override;
        void scrollEvent(ScrollEvent& event) override;

        Vector3 positionOnSphere(const Vector2& position) const;

        Shaders::PhongGL
            _shader{NoCreate},
            _shaderUniformBufferSingle{NoCreate},
            _shaderUniformBufferMultiple{NoCreate},
            #ifndef MAGNUM_TARGET_WEBGL
            _shaderUniformBufferMultipleShaderStorage{NoCreate},
            #endif
            _shaderUniformBufferMultiDraw{NoCreate}
            #ifndef MAGNUM_TARGET_WEBGL
            , _shaderUniformBufferMultiDrawShaderStorage{NoCreate}
            #endif
            ;
        Containers::Array<GL::Mesh> _meshes;
        GL::Mesh _combinedMesh;
        Containers::Array<GL::MeshView> _meshViews;

        struct {
            Scene3D scene;
            Object3D manipulator, cameraObject;
            SceneGraph::Camera3D* camera;
            SceneGraph::DrawableGroup3D drawables;
        } _sceneGraph;

        struct {
            /* (Object ID, parent ID or -1), ordered so parents are before
               their children */
            Containers::Array<Containers::Pair<UnsignedInt, Int>> parentOrder;
            /* Relative transformations indexed by object ID */
            Containers::Array<Matrix4> objectTransformations;
            /* Root transformation and then absolute transformations indexed by
               object ID + 1 (i.e., one item more than the
               objectTransformations array above) */
            Containers::Array<Matrix4> rootObjectAbsoluteTransformations;

            /* Projection uniform and light uniforms common for all draws,
               materials shared by all draws */
            Containers::Array<Shaders::ProjectionUniform3D> projection;
            Containers::Array<Shaders::PhongLightUniform> lights;
            Containers::Array<Shaders::PhongMaterialUniform> materials;

            /* Actual meshes / mesh views to draw and associated draw data */
            Containers::Array<Containers::Reference<GL::Mesh>> meshes;
            Containers::Array<Containers::Reference<GL::MeshView>> meshViews;
                // TODO drop, make a list of offsets/sizes instead...
            Containers::Array<Shaders::PhongDrawUniform> draws;

            /* Mapping from draw data to the rootObjectAbsoluteTransformations
               array above, and transformations copied from there in that
               order */
            Containers::Array<UnsignedInt> absoluteTransformationMapping;
            Containers::Array<Shaders::TransformationUniform3D> absoluteTransformations;
        } _direct;

        /* Used for UBO upload right before every draw, every time containing
           just the data for the next draw. Not doing any multi-buffering here
           as there would need to be thousands of buffers to prevent stalls. */
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
        } _uniformSingle;

        /* Used for UBO / SSBO upload once per frame and then either rebinding
           them with an offset before each draw or (multi-)drawing with a
           dynamic draw offset. Rotating multiple buffers so the (relatively
           large) upload isn't blocking if the GPU is busy rendering the
           previous frame.

           The *Storage + *Staging buffers are used to make UBOs / SSBOs in
           GPU-local memory and then copy to them from CPU-accessible memory,
           which is only possible with buffer storage extensions. */
        constexpr static std::size_t UniformMultiCount = 3;
        struct {
            /* These two are immutable, i.e. filled with either setStorage() or
               setData() upfront and then never changed after, so there doesn't
               need to be multiple variants */
            GL::Buffer projectionUniform;
            GL::Buffer materialUniform;

            GL::Buffer transformationUniform[UniformMultiCount];
            #ifndef MAGNUM_TARGET_WEBGL
            Containers::StaticArray<UniformMultiCount, GL::Buffer>
                       transformationUniformStorage{DirectInit, NoCreate};
            GL::Buffer transformationUniformStaging{NoCreate};
            #endif

            GL::Buffer drawUniform[UniformMultiCount];
            #ifndef MAGNUM_TARGET_WEBGL
            Containers::StaticArray<UniformMultiCount, GL::Buffer>
                       drawUniformStorage{DirectInit, NoCreate};
            GL::Buffer drawUniformStaging{NoCreate};
            #endif

            GL::Buffer lightUniform[UniformMultiCount];
            #ifndef MAGNUM_TARGET_WEBGL
            Containers::StaticArray<UniformMultiCount, GL::Buffer>
                       lightUniformStorage{DirectInit, NoCreate};
            GL::Buffer lightUniformStaging{NoCreate};
            #endif
        } _uniformMulti;
        UnsignedInt _uniformMultiFrameId = 0;

        Vector3 _previousPosition;
        Matrix4
            _projection{Matrix4::perspectiveProjection(35.0_degf, 4.0f/3.0f, 0.01f, 1000.0f)},
            // TODO hardcoded for the Buggy, put into an ifdef or sth??
                // TODO lol or make an ad-hoc 10000 Suzannes glTF in-place?
            _cameraTransformation{Matrix4::translation(Vector3::zAxis(450.0f))},
            _manipulatorTransformation{
                Matrix4::rotationX(20.0_degf)*
                Matrix4::rotationY(-40.0_degf)};

        DebugTools::FrameProfilerGL _profiler{
            DebugTools::FrameProfilerGL::Value::FrameTime|
            DebugTools::FrameProfilerGL::Value::CpuDuration|
            DebugTools::FrameProfilerGL::Value::GpuDuration, 150};

        /* Toggles driven by the UI */
        DrawType _drawType = DrawType::SceneGraph;
        #ifndef MAGNUM_TARGET_WEBGL
        // TODO make a bool
        Int _stagingBuffers = false;
        Int _shaderStorageBuffers = false;
        #endif

        /* UI */
        Ui::UserInterfaceGL _ui{NoCreate};
        Ui::Label _profilerOutput{NoCreate};
        #ifndef MAGNUM_TARGET_WEBGL
        Ui::NodeHandle _multiDrawToggles;
        #endif

        /* Touch gesture handling */
        Platform::TwoFingerGesture _pinchToZoom;
};

Nanoseconds now() {
    return Nanoseconds{std::chrono::steady_clock::now()};
}

MultiDrawExample::MultiDrawExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}
        .setTitle("Magnum Multi Draw Example")}
{
    Utility::Arguments args;
    args
        #ifndef CORRADE_TARGET_EMSCRIPTEN
        .addArgument("file").setHelp("file", "file to load")
        #endif
        .addBooleanOption("no-profile").setHelp("no-profile", "don't enable profiler on startup")
        .addSkippedPrefix("magnum", "engine-specific options")
        .parse(arguments.argc, arguments.argv);

    /* Setup renderer defaults */
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    /* Load a file */
    PluginManager::Manager<Trade::AbstractImporter> manager;
    #ifndef CORRADE_TARGET_EMSCRIPTEN
    Containers::Pointer<Trade::AbstractImporter> importer = manager.loadAndInstantiate("AnySceneImporter");
    if(!importer || !importer->openFile(args.value("file")))
        std::exit(1);
    #else
    Containers::Pointer<Trade::AbstractImporter> importer = manager.loadAndInstantiate("GltfImporter");
    CORRADE_INTERNAL_ASSERT_OUTPUT(importer && importer->openMemory(Utility::Resource{"data"}.getRaw("Buggy.glb")));
    #endif

    /* Load just the basic color info from all materials */
    _direct.materials = Containers::Array<Shaders::PhongMaterialUniform>{ValueInit, importer->materialCount()};
    for(UnsignedInt i = 0; i != importer->materialCount(); ++i) {
        const Containers::Optional<Trade::MaterialData> material = importer->material(i);
        CORRADE_INTERNAL_ASSERT(material);
        auto&& phong = material->as<Trade::PhongMaterialData>();
        _direct.materials[i].ambientColor = phong.ambientColor();
        _direct.materials[i].diffuseColor = phong.diffuseColor();
        _direct.materials[i].specularColor = phong.specularColor();
    }

    /* Load all meshes */
    Containers::Array<Trade::MeshData> meshData;
    arrayReserve(meshData, importer->meshCount());
    _meshes = Containers::Array<GL::Mesh>{ValueInit, importer->meshCount()};
    Containers::Array<UnsignedInt> meshOffsets{ValueInit, importer->meshCount() + 1};
    UnsignedInt offset = 0;
    meshOffsets[0] = 0;
    for(UnsignedInt i = 0; i != importer->meshCount(); ++i) {
        arrayAppend(meshData, *importer->mesh(i));
        _meshes[i] = MeshTools::compile(meshData[i]);

        offset += meshData[i].indexCount();
        meshOffsets[i + 1] = offset;
    }
    _combinedMesh = MeshTools::compile(MeshTools::concatenate(meshData));
    for(UnsignedInt i = 0; i != _meshes.size(); ++i) {
        arrayAppend(_meshViews, GL::MeshView{_combinedMesh})
            .setIndexOffset(meshOffsets[i])
            .setCount(meshOffsets[i + 1] - meshOffsets[i]);
    }

    /* Load the scene */
    const Trade::SceneData scene = *CORRADE_INTERNAL_ASSERT_EXPRESSION(importer->scene(importer->defaultScene()));

    /* Projection, light setup. Just two lights right now. */
    _direct.projection = Containers::Array<Shaders::ProjectionUniform3D>{ValueInit, 1};
    _direct.lights = Containers::Array<Shaders::PhongLightUniform>{ValueInit, 2};

    /* (Object ID, transformation) pairs and (Object ID, (mesh, material))
       tuples, both in no particular order */
    const Containers::Array<Containers::Pair<UnsignedInt, Matrix4>> transformations = scene.transformations3DAsArray();
    const Containers::Array<Containers::Pair<UnsignedInt, Containers::Pair<UnsignedInt, Int>>> meshesMaterials = scene.meshesMaterialsAsArray();

    /* SceneGraph setup. Uhh, so much typing. Yes, it's a local class to have
       all the abstraction overhead nicely visible from a single place. */
    {
        class Drawable: public SceneGraph::Drawable3D {
            public:
                explicit Drawable(Object3D& object, Shaders::PhongGL& shader, GL::Mesh& mesh, const Color4& ambient, const Color4& diffuse, const Color3& specular, Float shininess, SceneGraph::DrawableGroup3D& group): SceneGraph::Drawable3D{object, &group}, _shader(shader), _mesh(mesh), _ambient{ambient}, _diffuse{diffuse}, _specular{specular}, _shininess{shininess} {}

            private:
                void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override {
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

                Shaders::PhongGL& _shader;
                GL::Mesh& _mesh;
                Color4 _ambient, _diffuse;
                Color3 _specular;
                Float _shininess;
        };

        _sceneGraph.cameraObject
            .setParent(&_sceneGraph.scene);
        (*(_sceneGraph.camera = new SceneGraph::Camera3D{_sceneGraph.cameraObject}))
            .setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
            .setProjectionMatrix(_projection)
            .setViewport(GL::defaultFramebuffer.viewport().size());

        /* Base object, parent of all (for easy manipulation) */
        _sceneGraph.manipulator.setParent(&_sceneGraph.scene);

        /* Create objects, parent them based on the hierarchy (in a separate
           step as it's not guaranteed that a parent is listed before its
           children), and apply transformations to objects that have it. The
           objects array isn't needed for anything after, it acts just as a
           temporary storage to be able to access them by object ID. */
        const Containers::Array<Containers::Pair<UnsignedInt, Int>> parents = scene.parentsAsArray();
        Containers::Array<Object3D*> objects{ValueInit, std::size_t(scene.mappingBound())};
        for(const Containers::Pair<UnsignedInt, Int>& parent: parents)
            objects[parent.first()] = new Object3D;
        for(const Containers::Pair<UnsignedInt, Int>& parent: parents)
            objects[parent.first()]->setParent(
                parent.second() == -1 ? &_sceneGraph.manipulator : objects[parent.second()]);
        for(const Containers::Pair<UnsignedInt, Matrix4>& transformation: transformations)
            objects[transformation.first()]->setTransformation(transformation.second());

        /* Assign drawables to the created objects. This is not a 1:1 mapping,
           i.e. there can be more than one mesh assigned to the same object, or
           none at all. */
        for(Containers::Pair<UnsignedInt, Containers::Pair<UnsignedInt, Int>> meshMaterial: meshesMaterials) {
            CORRADE_INTERNAL_ASSERT(meshMaterial.second().second() != -1);

            new Drawable{*objects[meshMaterial.first()], _shader,
                _meshes[meshMaterial.second().first()],
                _direct.materials[meshMaterial.second().second()].ambientColor,
                _direct.materials[meshMaterial.second().second()].diffuseColor,
                _direct.materials[meshMaterial.second().second()].specularColor.rgb(),
                _direct.materials[meshMaterial.second().second()].shininess,
                _sceneGraph.drawables};
        }
    }

    /* For everything other than SceneGraph we need an (object ID, parent ID)
       mapping ordered in a way that puts parents before their children, which
       is going to get used for hierarchical transformation calculation */
    _direct.parentOrder = SceneTools::parentsBreadthFirst(scene);

    /* Object transformations, and a temporary array for calculating absolute
       transformations. This array will get fully overwritten every draw but
       keeping it as a member to avoid reallocating it every frame. */
    _direct.objectTransformations = Containers::Array<Matrix4>{ValueInit, std::size_t(scene.mappingBound())};
    for(const Containers::Pair<UnsignedInt, Matrix4>& transformation: transformations)
        _direct.objectTransformations[transformation.first()] = transformation.second();
    _direct.rootObjectAbsoluteTransformations = Containers::Array<Matrix4>{NoInit, std::size_t(scene.mappingBound()) + 1};

    /* Direct drawing is simply done in the order of the meshesMaterials list,
       so we also have to populate a mapping from those to transformations
       indexed by object ID */
    _direct.absoluteTransformationMapping = Containers::Array<UnsignedInt>{NoInit, meshesMaterials.size()};
    _direct.absoluteTransformations = Containers::Array<Shaders::TransformationUniform3D>{ValueInit, meshesMaterials.size()};
    arrayReserve(_direct.meshes, meshesMaterials.size());
    arrayReserve(_direct.meshViews, meshesMaterials.size());
    _direct.draws = Containers::Array<Shaders::PhongDrawUniform>{ValueInit, meshesMaterials.size()};
    for(std::size_t i = 0; i != meshesMaterials.size(); ++i) {
        /* The rootObjectAbsoluteTransformations array contains a root
           transformation at index 0, so add 1 to the object ID */
        _direct.absoluteTransformationMapping[i] = meshesMaterials[i].first() + 1;
        CORRADE_INTERNAL_ASSERT(meshesMaterials[i].second().second() != -1);
        arrayAppend(_direct.meshes, _meshes[meshesMaterials[i].second().first()]);
        arrayAppend(_direct.meshViews, _meshViews[meshesMaterials[i].second().first()]);
        _direct.draws[i].setMaterialId(meshesMaterials[i].second().second());
    }

    /* Set up shaders. The multi-draw shaders and uniform storage are set up
       based on the data count we have. */
    _shader = Shaders::PhongGL{Shaders::PhongGL::Configuration{}
        .setLightCount(2)};
    _shaderUniformBufferSingle = Shaders::PhongGL{Shaders::PhongGL::Configuration{}
        .setFlags(Shaders::PhongGL::Flag::UniformBuffers)
        .setLightCount(2)};
    _shaderUniformBufferMultiple = Shaders::PhongGL{Shaders::PhongGL::Configuration{}
        .setFlags(Shaders::PhongGL::Flag::UniformBuffers)
        .setLightCount(_direct.lights.size())
        .setMaterialCount(_direct.materials.size())
        /* At most 1024 draws can fit into the usual 64k UBO limit */
        .setDrawCount(Math::min<UnsignedInt>(1024, _direct.draws.size()))};
    #ifndef MAGNUM_TARGET_WEBGL
    #ifndef MAGNUM_TARGET_GLES
    if(GL::Context::current().isExtensionSupported<GL::Extensions::ARB::shader_storage_buffer_object>())
    #else
    if(GL::Context::current().isVersionSupported<GL::Version::GLES310>())
    #endif
        _shaderUniformBufferMultipleShaderStorage = Shaders::PhongGL{Shaders::PhongGL::Configuration{}
            .setFlags(Shaders::PhongGL::Flag::ShaderStorageBuffers|
                      Shaders::PhongGL::Flag::MultiDraw)
            .setLightCount(_direct.lights.size())};
    #endif
    /* Compile the multidraw shader only if gl_DrawID is available. It isn't
       used otherwise. */
    if(GL::Context::current().isExtensionSupported<
        #ifdef MAGNUM_TARGET_WEBGL
        GL::Extensions::WEBGL::multi_draw
        #elif defined(MAGNUM_TARGET_GLES)
        GL::Extensions::ANGLE::multi_draw
        #else
        GL::Extensions::ARB::shader_draw_parameters
        #endif
    >()) {
        _shaderUniformBufferMultiDraw = Shaders::PhongGL{Shaders::PhongGL::Configuration{}
            .setFlags(Shaders::PhongGL::Flag::UniformBuffers|
                      Shaders::PhongGL::Flag::MultiDraw)
            .setLightCount(_direct.lights.size())
            .setMaterialCount(_direct.materials.size())
            /* At most 1024 draws can fit into the usual 64k UBO limit */
            .setDrawCount(Math::min<UnsignedInt>(1024, _direct.draws.size()))};
        #ifndef MAGNUM_TARGET_WEBGL
        #ifndef MAGNUM_TARGET_GLES
        if(GL::Context::current().isExtensionSupported<GL::Extensions::ARB::shader_storage_buffer_object>())
        #else
        if(GL::Context::current().isVersionSupported<GL::Version::GLES310>())
        #endif
            _shaderUniformBufferMultiDrawShaderStorage = Shaders::PhongGL{Shaders::PhongGL::Configuration{}
                .setFlags(Shaders::PhongGL::Flag::ShaderStorageBuffers|
                          Shaders::PhongGL::Flag::MultiDraw)
                .setLightCount(_direct.lights.size())};
        #endif
    }
    _direct.projection[0].projectionMatrix = _projection;
    #ifndef MAGNUM_TARGET_WEBGL
    if(GL::Context::current().isExtensionSupported<
        #ifndef MAGNUM_TARGET_GLES
        GL::Extensions::ARB::buffer_storage
        #else
        GL::Extensions::EXT::buffer_storage
        #endif
    >()) {
        _uniformMulti.projectionUniform.setStorage(_direct.projection, {});
        _uniformMulti.materialUniform.setStorage(_direct.materials, {});
        for(std::size_t i = 0; i != UniformMultiCount; ++i) {
            _uniformMulti.transformationUniformStorage[i] = GL::Buffer{};
            _uniformMulti.transformationUniformStorage[i].setStorage(_direct.draws.size()*sizeof(Shaders::TransformationUniform3D), {});
            _uniformMulti.drawUniformStorage[i] = GL::Buffer{};
            _uniformMulti.drawUniformStorage[i].setStorage(_direct.draws.size()*sizeof(Shaders::PhongDrawUniform), {});
            _uniformMulti.lightUniformStorage[i] = GL::Buffer{};
            _uniformMulti.lightUniformStorage[i].setStorage(_direct.lights.size()*sizeof(Shaders::PhongLightUniform), {});
        }
        _uniformMulti.transformationUniformStaging = GL::Buffer{};
        _uniformMulti.transformationUniformStaging.setStorage(_direct.absoluteTransformations.size()*sizeof(Shaders::TransformationUniform3D), GL::Buffer::StorageFlag::DynamicStorage);
        _uniformMulti.drawUniformStaging = GL::Buffer{};
        _uniformMulti.drawUniformStaging.setStorage(_direct.draws.size()*sizeof(Shaders::PhongDrawUniform), GL::Buffer::StorageFlag::DynamicStorage);
        _uniformMulti.lightUniformStaging = GL::Buffer{};
        _uniformMulti.lightUniformStaging.setStorage(_direct.lights.size()*sizeof(Shaders::PhongLightUniform), GL::Buffer::StorageFlag::DynamicStorage);
    } else {
        _uniformMulti.projectionUniform.setData(_direct.projection);
        _uniformMulti.materialUniform.setData(_direct.materials);
    }
    #endif

    for(std::size_t i = 0; i != UniformMultiCount; ++i) {
        _uniformMulti.transformationUniform[i].setData({nullptr, _direct.draws.size()*sizeof(Shaders::TransformationUniform3D)}, GL::BufferUsage::DynamicDraw);
        _uniformMulti.drawUniform[i].setData({nullptr, _direct.draws.size()*sizeof(Shaders::PhongDrawUniform)}, GL::BufferUsage::DynamicDraw);
        _uniformMulti.lightUniform[i].setData({nullptr, _direct.lights.size()*sizeof(Shaders::PhongLightUniform)}, GL::BufferUsage::DynamicDraw);
    }

    /* Create the UI */
    {
        _ui.create(*this, Ui::DarkTheme{Ui::DarkTheme::Feature::Animations});
        /** @todo make a builtin API for this, or, better, make it automatic */
        CORRADE_INTERNAL_ASSERT(_ui.textLayer().shared().font(Ui::fontHandle(0, 1)).fillGlyphCache(_ui.textLayer().shared().glyphCache(), "μ"));

        /* Profiler toggle, reflect the --no-profile command line flag in the
           initial state */
        // TODO have a BitStorage
        Ui::EnumStorage<Int> profile{_ui, DirectInit, args.isSet("no-profile") ? false : true};
        profile.setEnumSet(true);
        profile->onUpdate([&](Int enabled) {
            enabled ? _profiler.enable() : _profiler.disable();
            if(!enabled)
                _profilerOutput.setText({});
        });

        /* All these are updated only from the UI but read in every draw event
           so it makes sense for them to just reference a member variable. */
        Ui::EnumStorage<DrawType> drawType{_ui, Ui::NonOwned, _drawType};
        drawType->onUpdate([&](const DrawType type) {
            /* Every time the draw type changes the profiler needs to be reset
               to not display stale numbers */
            _profiler.enable();

            #ifndef MAGNUM_TARGET_WEBGL
            /* Enable the multi-draw toggles only when they affect the
               currently picked draw type */
            type == DrawType::UboDrawOffset ||
            type == DrawType::MultiDraw ?
                _ui.clearNodeFlags(_multiDrawToggles, Ui::NodeFlag::Disabled) :
                _ui.addNodeFlags(_multiDrawToggles, Ui::NodeFlag::Disabled);
            #else
            static_cast<void>(type);
            #endif
        });

        #ifndef MAGNUM_TARGET_WEBGL
        // TODO have a BitStorage
        Ui::EnumStorage<Int> stagingBuffers{_ui, Ui::NonOwned, _stagingBuffers};
        Ui::EnumStorage<Int> shaderStorageBuffers{_ui, Ui::NonOwned, _shaderStorageBuffers};
        stagingBuffers.setEnumSet(true);
        shaderStorageBuffers.setEnumSet(true);
        /* Every time these change the profiler also needs to be reset to not
           display stale numbers */
        stagingBuffers->onUpdate([&](Int) { _profiler.enable(); });
        shaderStorageBuffers->onUpdate([&](Int) { _profiler.enable(); });
        #endif

        /* Scene stats, profiler output */
        Ui::SnapLayoutRowTop info = Ui::SnapLayout::snapRoot(_ui, Ui::Snap::Top|Ui::Snap::FillX);
        {
            Ui::SnapLayoutColumnLeft column = info.child();
            Ui::checkbox(column.child(), profile.value<1>(), "Enable profiling");
            _profilerOutput = Ui::Label(column.child(), {});
        } {
            /* Spacer between the left and right label */
            info.child(Ui::Snap::FillX);
        } {
            Ui::SnapLayoutColumnLeft column = info.child();
            Ui::label(column.child(), Utility::format(
                "Material count: {}\n"
                "Mesh count: {}\n"
                "Draw count: {}\n"
                "Total triangles: {}",
                importer->materialCount(),
                importer->meshCount(),
                meshesMaterials.size(),
                _combinedMesh.count()/3), Text::Alignment::MiddleLeft);
            #ifdef CORRADE_IS_DEBUG_BUILD
            Ui::label(column.child(), "Running a debug build", Text::Alignment::MiddleLeft, Ui::LabelStyle::Warning);
            #endif
            if(meshesMaterials.size() > 1024)
                Ui::label(column.child(), "UBOs limited to 1024 draws", Text::Alignment::MiddleLeft, Ui::LabelStyle::Danger);
        }

        /* Two columns of radio buttons to toggle what draw type is used */
        Ui::SnapLayoutColumn toggles = Ui::SnapLayout::snapRoot(_ui, Ui::Snap::Bottom);
        Ui::SnapLayoutRow toggles1 = toggles.child();
        {
            Ui::SnapLayoutColumnLeft column = toggles1.child(Ui::Snap::FillX);
            Ui::radioButton(column.child(),
                drawType.value<DrawType::SceneGraph>(), "SceneGraph");
            Ui::radioButton(column.child(),
                drawType.value<DrawType::DeduplicatedLoop>(), "Loop, deduplicated uniform setters");
            Ui::radioButton(column.child(),
                drawType.value<DrawType::UboPerDraw>(), "Loop, mesh views, separate per-draw UBOs");
            Ui::radioButton(column.child(),
                drawType.value<DrawType::UboDrawOffset>(), "Loop, mesh views, UBOs with draw offset");
        } {
            Ui::SnapLayoutColumnLeft column = toggles1.child(Ui::Snap::FillX);
            Ui::radioButton(column.child(),
                drawType.value<DrawType::TrivialLoop>(), "Loop, trivial");
            Ui::radioButton(column.child(),
                drawType.value<DrawType::DeduplicatedLoopMeshViews>(), "Loop, mesh views, deduplicated setters");
            /* If UBO bind alignment is larger than a size of Vector4, we
               cannot just rebind for individual uniforms. Disable this option
               in that case. */
            Ui::radioButton(
                column.child({}, GL::Buffer::uniformOffsetAlignment() <= 32 ?
                    Ui::NodeFlags{} : Ui::NodeFlag::Disabled),
                drawType.value<DrawType::UboBindOffset>(), "Loop, mesh views, UBOs with bind offset");
            /* If gl_DrawID isn't supported, we cannot do multidraw. Disable
               this option in that case. */
            Ui::radioButton(
                column.child({}, GL::Context::current().isExtensionSupported<
                    #ifdef MAGNUM_TARGET_WEBGL
                    GL::Extensions::WEBGL::multi_draw
                    #elif defined(MAGNUM_TARGET_GLES)
                    GL::Extensions::ANGLE::multi_draw
                    #else
                    GL::Extensions::ARB::shader_draw_parameters
                    #endif
                >() ?
                    Ui::NodeFlags{} : Ui::NodeFlag::Disabled),
                drawType.value<DrawType::MultiDraw>(), "Mesh views, UBOs, multidraw");
        } {
            #ifndef MAGNUM_TARGET_WEBGL
            /* These toggles get enabled only for related draw types. Neither
               of these is a thing on WebGL. */
            Ui::SnapLayoutRow row = toggles.child({}, Ui::NodeFlag::Disabled);
            _multiDrawToggles = row;
            Ui::checkbox(
                row.child({}, GL::Context::current().isExtensionSupported<
                    #ifndef MAGNUM_TARGET_GLES
                    GL::Extensions::ARB::buffer_storage
                    #else
                    GL::Extensions::EXT::buffer_storage
                    #endif
                >() ?
                    Ui::NodeFlags{} : Ui::NodeFlag::Disabled),
                stagingBuffers.value<1>(), "Staging uniform buffers");
            Ui::checkbox(
                row.child({},
                    #ifndef MAGNUM_TARGET_GLES
                    GL::Context::current().isExtensionSupported<GL::Extensions::ARB::shader_storage_buffer_object>() ?
                    #else
                    GL::Context::current().isVersionSupported<GL::Version::GLES310>() ?
                    #endif
                    Ui::NodeFlags{} : Ui::NodeFlag::Disabled),
                shaderStorageBuffers.value<1>(), "SSBOs instead of UBOs");
            #endif
        }
    }

    /* The profiler is enabled by default, causing constant redraw right from
       the start. Disable it if requested on command line. */
    if(args.isSet("no-profile"))
        _profiler.disable();
}

void MultiDrawExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

    _profiler.beginFrame();

    /* Draw using the scene graph */
    if(_drawType == DrawType::SceneGraph) {
        _sceneGraph.manipulator.setTransformation(_manipulatorTransformation);
        _sceneGraph.cameraObject.setTransformation(_cameraTransformation);
        _sceneGraph.camera->draw(_sceneGraph.drawables);

    /* Direct drawing  */
    } else {
        /* Calculate absolute transformations based on the parent order first.
           The first index in rootObjectAbsoluteTransformations is the root
           transform that's applied to all others, so all other indices are
           shifted by 1. */
        _direct.rootObjectAbsoluteTransformations[0] = _cameraTransformation.invertedRigid()*_manipulatorTransformation;
        for(Containers::Pair<UnsignedInt, Int> objectParent: _direct.parentOrder)
            _direct.rootObjectAbsoluteTransformations[objectParent.first() + 1] = _direct.rootObjectAbsoluteTransformations[objectParent.second() + 1]*_direct.objectTransformations[objectParent.first()];

        /* Then copy those to corresponding draws. The mapping is not 1:1 so
           a single transformation may be used for multiple meshes but also
           none at all. */
        for(std::size_t i = 0; i != _direct.absoluteTransformations.size(); ++i) {
            const Matrix4 transformation = _direct.rootObjectAbsoluteTransformations[_direct.absoluteTransformationMapping[i]];
            _direct.absoluteTransformations[i].setTransformationMatrix(transformation);
            _direct.draws[i].setNormalMatrix(transformation.normalMatrix());
        }

        _direct.lights[0].position = {-300.0f, 100.0f, 100.0f, 0.0f};
        _direct.lights[1].position = {300.0f, 100.0f, 100.0f, 0.0f};

        /* Render everything in a simple loop */
        if(_drawType == DrawType::TrivialLoop) {
            for(std::size_t i = 0; i != _direct.draws.size(); ++i) {
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
                    .setTransformationMatrix(_direct.absoluteTransformations[i].transformationMatrix)
                    .setNormalMatrix(Matrix3x3{_direct.draws[i].normalMatrix})
                    .setProjectionMatrix(_projection)
                    .draw(*_direct.meshes[i]);
            }

        } else if(_drawType == DrawType::DeduplicatedLoop) {
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

            for(std::size_t i = 0; i != _direct.draws.size(); ++i) {
                const std::size_t materialId = _direct.draws[i].materialId;
                _shader
                    .setAmbientColor(_direct.materials[materialId].ambientColor)
                    .setDiffuseColor(_direct.materials[materialId].diffuseColor)
                    .setSpecularColor(_direct.materials[materialId].specularColor)
                    .setShininess(_direct.materials[materialId].shininess)
                    .setTransformationMatrix(_direct.absoluteTransformations[i].transformationMatrix)
                    .setNormalMatrix(Matrix3x3{_direct.draws[i].normalMatrix})
                    .draw(*_direct.meshes[i]);
            }

        } else if(_drawType == DrawType::DeduplicatedLoopMeshViews) {
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

            for(std::size_t i = 0; i != _direct.draws.size(); ++i) {
                const std::size_t materialId = _direct.draws[i].materialId;
                _shader
                    .setAmbientColor(_direct.materials[materialId].ambientColor)
                    .setDiffuseColor(_direct.materials[materialId].diffuseColor)
                    .setSpecularColor(_direct.materials[materialId].specularColor)
                    .setShininess(_direct.materials[materialId].shininess)
                    .setTransformationMatrix(_direct.absoluteTransformations[i].transformationMatrix)
                    .setNormalMatrix(Matrix3x3{_direct.draws[i].normalMatrix})
                    .draw(*_direct.meshViews[i]);
            }

        } else if(_drawType == DrawType::UboPerDraw) {
            _uniformSingle.projectionUniform.setSubData(0, _direct.projection);
            _uniformSingle.lightUniform.setSubData(0, _direct.lights);
            _shaderUniformBufferSingle
                .bindProjectionBuffer(_uniformSingle.projectionUniform)
                .bindLightBuffer(_uniformSingle.lightUniform);

            for(std::size_t i = 0; i != _direct.draws.size(); ++i) {
                _uniformSingle.transformationUniform.setSubData(0, _direct.absoluteTransformations.sliceSize(i, 1));
                _uniformSingle.materialUniform.setSubData(0, _direct.materials.sliceSize(_direct.draws[i].materialId, 1));
                /* The shader is using a single material, so the material ID is
                   unused */
                _uniformSingle.drawUniform.setSubData(0, _direct.draws.sliceSize(i, 1));
                _shaderUniformBufferSingle
                    .bindTransformationBuffer(_uniformSingle.transformationUniform)
                    .bindMaterialBuffer(_uniformSingle.materialUniform)
                    .bindDrawBuffer(_uniformSingle.drawUniform)
                    .draw(*_direct.meshViews[i]);
            }

        } else if(_drawType == DrawType::UboBindOffset ||
                  _drawType == DrawType::UboDrawOffset ||
                  _drawType == DrawType::MultiDraw)
        {
            #ifndef MAGNUM_TARGET_WEBGL
            if(_stagingBuffers) {
                _uniformMulti.lightUniformStaging.setSubData(0, _direct.lights);
                _uniformMulti.transformationUniformStaging.setSubData(0, _direct.absoluteTransformations);
                _uniformMulti.drawUniformStaging.setSubData(0, _direct.draws);
                GL::Buffer::copy(
                    _uniformMulti.lightUniformStaging,
                    _uniformMulti.lightUniformStorage[_uniformMultiFrameId],
                    0, 0, _uniformMulti.lightUniformStaging.size());
                GL::Buffer::copy(
                    _uniformMulti.transformationUniformStaging,
                    _uniformMulti.transformationUniformStorage[_uniformMultiFrameId],
                    0, 0, _uniformMulti.transformationUniformStaging.size());
                GL::Buffer::copy(
                    _uniformMulti.drawUniformStaging,
                    _uniformMulti.drawUniformStorage[_uniformMultiFrameId],
                    0, 0, _uniformMulti.drawUniformStaging.size());
            } else
            #endif
            {
                // TODO somehow here it stopped working?! are the buffers too small or wat??
                _uniformMulti.lightUniform[_uniformMultiFrameId].setSubData(0, _direct.lights);
                _uniformMulti.transformationUniform[_uniformMultiFrameId].setSubData(0, _direct.absoluteTransformations);
                _uniformMulti.drawUniform[_uniformMultiFrameId].setSubData(0, _direct.draws);
            }

            if(_drawType == DrawType::UboBindOffset) {
                _shaderUniformBufferSingle
                    .bindProjectionBuffer(_uniformMulti.projectionUniform)
                    .bindLightBuffer(
                        #ifndef MAGNUM_TARGET_WEBGL
                        _stagingBuffers ?
                            _uniformMulti.lightUniformStorage[_uniformMultiFrameId] :
                        #endif
                            _uniformMulti.lightUniform[_uniformMultiFrameId]);
                for(std::size_t i = 0; i != _direct.draws.size(); ++i) {
                    _shaderUniformBufferSingle
                        .bindTransformationBuffer(
                            #ifndef MAGNUM_TARGET_WEBGL
                            _stagingBuffers ?
                                _uniformMulti.transformationUniformStorage[_uniformMultiFrameId] :
                            #endif
                                _uniformMulti.transformationUniform[_uniformMultiFrameId],
                            i*sizeof(Shaders::TransformationUniform3D), sizeof(Shaders::TransformationUniform3D))
                        .bindMaterialBuffer(_uniformMulti.materialUniform, _direct.draws[i].materialId*sizeof(Shaders::PhongMaterialUniform), sizeof(Shaders::PhongMaterialUniform))
                        .bindDrawBuffer(
                            #ifndef MAGNUM_TARGET_WEBGL
                            _stagingBuffers ?
                                _uniformMulti.drawUniformStorage[_uniformMultiFrameId] :
                            #endif
                                _uniformMulti.drawUniform[_uniformMultiFrameId],
                            i*sizeof(Shaders::PhongDrawUniform), sizeof(Shaders::PhongDrawUniform))
                        .draw(*_direct.meshViews[i]);
                }
            } else {
                Shaders::PhongGL* shader;
                if(_drawType == DrawType::UboDrawOffset)
                    shader =
                        #ifndef MAGNUM_TARGET_WEBGL
                        _shaderStorageBuffers ?
                            &_shaderUniformBufferMultipleShaderStorage :
                        #endif
                            &_shaderUniformBufferMultiple;
                else if(_drawType == DrawType::MultiDraw)
                    shader =
                        #ifndef MAGNUM_TARGET_WEBGL
                        _shaderStorageBuffers ?
                            &_shaderUniformBufferMultiDrawShaderStorage :
                        #endif
                            &_shaderUniformBufferMultiDraw;
                else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
                (*shader)
                    .bindProjectionBuffer(_uniformMulti.projectionUniform)
                    .bindMaterialBuffer(_uniformMulti.materialUniform)
                    .bindLightBuffer(
                        #ifndef MAGNUM_TARGET_WEBGL
                        _stagingBuffers ?
                            _uniformMulti.lightUniformStorage[_uniformMultiFrameId] :
                        #endif
                            _uniformMulti.lightUniform[_uniformMultiFrameId])
                    .bindTransformationBuffer(
                        #ifndef MAGNUM_TARGET_WEBGL
                        _stagingBuffers ?
                            _uniformMulti.transformationUniformStorage[_uniformMultiFrameId] :
                        #endif
                            _uniformMulti.transformationUniform[_uniformMultiFrameId])
                    .bindDrawBuffer(
                        #ifndef MAGNUM_TARGET_WEBGL
                        _stagingBuffers ?
                            _uniformMulti.drawUniformStorage[_uniformMultiFrameId] :
                        #endif
                            _uniformMulti.drawUniform[_uniformMultiFrameId]);

                /* With UBOs we're limited to 64k per uniform buffer, which is
                   1024 draws at most. With SSBOs the sky is the limit, but the
                   perf isn't as great. */
                const std::size_t drawCount =
                    #ifndef MAGNUM_TARGET_WEBGL
                    _shaderStorageBuffers ?
                        _direct.draws.size() :
                    #endif
                    Math::min<UnsignedInt>(1024, _direct.draws.size());
                if(_drawType == DrawType::UboDrawOffset) for(std::size_t i = 0; i != drawCount; ++i) {
                    (*shader)
                        .setDrawOffset(i)
                        .draw(*_direct.meshViews[i]);
                } else if(_drawType == DrawType::MultiDraw) {
                    // TODO eh wait, what, why not pass them all directly?
                    // TODO use the indirect thing maybe?
                    (*shader)
                        .draw(_direct.meshViews.prefix(drawCount));
                } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();

                _uniformMultiFrameId = (_uniformMultiFrameId + 1) % UniformMultiCount;
            }

        } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }

    _profiler.endFrame();
    // TODO uh any uhh any helper for this?
        // TODO what does it do for the very first frame?
    if(_profiler.isEnabled() && _profiler.measuredFrameCount() % 50 == 0)
        _profilerOutput.setText(_profiler.statistics(), Text::Alignment::MiddleLeft);

    /* Draw the UI, deliberately outside of the profiled scope */
    GL::Renderer::disable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::Blending);
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::OneMinusSourceAlpha);
    _ui.advanceAnimations(now())
       .draw();
    GL::Renderer::setBlendFunction(GL::Renderer::BlendFunction::One, GL::Renderer::BlendFunction::One);
    GL::Renderer::disable(GL::Renderer::Feature::Blending);
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);

    /* Redraw only if the UI wants to or if profiling */
    swapBuffers();
    if(_ui || _profiler.isEnabled())
        redraw(); // TODO oh the UI could trigger this too
}

void MultiDrawExample::pointerPressEvent(PointerEvent& event) {
    /* If the UI handles the press, don't propagate it to the scene */
    if(!_ui.pointerPressEvent(event, now())) {
        _pinchToZoom.pressEvent(event);

        if(event.isPrimary() && (event.pointer() & (Pointer::MouseLeft|Pointer::Finger)))
            _previousPosition = positionOnSphere(event.position());
    }

    if(_ui)
        redraw();
}

void MultiDrawExample::pointerReleaseEvent(PointerEvent& event) {
    /* On release reset the movement position and handle touch gestures
       regardless of whether the UI handles it as well so the releases aren't
       lost when e.g. dragging from the scene to the UI */
    _pinchToZoom.releaseEvent(event);

    if(event.isPrimary() && (event.pointer() & (Pointer::MouseLeft|Pointer::Finger)))
        _previousPosition = Vector3{};

    /* Pass the event to the UI unconditionally */
    _ui.pointerReleaseEvent(event, now());

    if(_ui)
        redraw();
}

void MultiDrawExample::pointerMoveEvent(PointerMoveEvent& event) {
    _pinchToZoom.moveEvent(event);

    /* If we have a consistent state for a pinch gesture, handle it. This won't
       happen if the press happened on the UI, as in that case the press isn't
       propagated to the gesture. */
    if(_pinchToZoom) {
        /* Distance to origin */
        const Float distance = _cameraTransformation.translation().z();

        /* Move 15% of the distance back or forward */
        _cameraTransformation =
            Matrix4::translation(Vector3::zAxis(
            distance*(1.0f - _pinchToZoom.relativeScaling())))*
            _cameraTransformation;

        event.setAccepted();
        redraw();
    }

    /* If a press happened outside of the UI (and thus the previous position is
       recorded), perform a movement regardless of whether the pointer is over
       the UI so it doesn't get randomly stuck  */
    if(!_previousPosition.isZero() && event.isPrimary() && (event.pointers() & (Pointer::MouseLeft|Pointer::Finger))) {
        const Vector3 currentPosition = positionOnSphere(event.position());
        const Vector3 axis = Math::cross(_previousPosition, currentPosition);

        // TODO the exis being zero triggers a blank screen on webgl somehow?

        _manipulatorTransformation =
            Matrix4::rotation(Math::angle(_previousPosition, currentPosition), axis.normalized())*
            _manipulatorTransformation;
        _previousPosition = currentPosition;

        event.setAccepted();
        redraw();
    }

    /* If neither of the above accepted the event, let the UI handle it */
    if(!event.isAccepted())
        _ui.pointerMoveEvent(event, now());

    if(_ui)
        redraw();
}

void MultiDrawExample::scrollEvent(ScrollEvent& event) {
    if(_ui.scrollEvent(event, now())) {
        /* UI handles it */

    } else if(event.offset().y()) {
        /* Distance to origin */
        const Float distance = _cameraTransformation.translation().z();

        /* Move 15% of the distance back or forward */
        _cameraTransformation =
            Matrix4::translation(Vector3::zAxis(
            distance*(1.0f - (event.offset().y() > 0 ? 1/0.85f : 0.85f))))*
            _cameraTransformation;

        redraw();
    }

    if(_ui)
        redraw();
}

Vector3 MultiDrawExample::positionOnSphere(const Vector2& position) const {
    const Vector2 positionNormalized = position/Vector2{windowSize()} - Vector2{0.5f};
    const Float length = positionNormalized.length();
    const Vector3 result(length > 1.0f ? Vector3(positionNormalized, 0.0f) : Vector3(positionNormalized, 1.0f - length));
    return (result*Vector3::yScale(-1.0f)).normalized();
}

}}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::MultiDrawExample)
