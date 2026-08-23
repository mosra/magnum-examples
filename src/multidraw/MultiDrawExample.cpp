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
    SceneGraph,
    TrivialLoop,
    DeduplicatedLoop,
    DeduplicatedLoopMeshViews,
    UboUploadEach,
    UboUploadOnceSetOffset,
    UboUploadOnceSetOffsetMeshViews,
    UboUploadOnceSetOffsetMultiDraw
};

class MultiDrawExample: public Platform::Application {
    public:
        explicit MultiDrawExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void mouseScrollEvent(MouseScrollEvent& event) override;

        Vector3 positionOnSphere(const Vector2i& position) const;

        Shaders::PhongGL
            _shader{NoCreate},
            _shaderUniformBufferSingle{NoCreate},
            _shaderUniformBufferMultiple{NoCreate},
            _shaderUniformBufferMultiDraw{NoCreate};
        Containers::Array<GL::Mesh> _meshes;
        GL::Mesh _combinedMesh;
        Containers::Optional<GL::MeshView> _emptyView;
        Containers::Array<GL::MeshView> _meshViews;

        struct {
            Scene3D scene;
            Object3D manipulator, cameraObject;
            SceneGraph::Camera3D* camera;
            SceneGraph::DrawableGroup3D drawables;
        } _sceneGraph;

        // TODO actually all this is needed only for _direct also, figure out naming
            // TODO the absolute trasnforms could also be a temp array maybe
        Containers::Array<Containers::Pair<UnsignedInt, Int>> _parentOrder;
        Containers::Array<Matrix4> _transformations, _absoluteTransformations;

        struct {
            Containers::Array<UnsignedInt> transformationIds;
            Containers::Array<GL::Mesh*> meshes;
            Containers::Array<Containers::Reference<GL::MeshView>> meshViews;

            Containers::Array<Shaders::ProjectionUniform3D> projections;
            Containers::Array<Shaders::TransformationUniform3D> absoluteTransformations;
            Containers::Array<Shaders::PhongDrawUniform> draws;
            // TODO ffs the materials array isn't related to _direct in any way, move out
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

            GL::Buffer lightUniform[UniformMultiCount];
            GL::Buffer lightUniformStaging;
        } _uniformMulti;
        UnsignedInt _uniformMultiFrameId = 0;

        Vector3 _previousPosition;
        Matrix4
            _projection{Matrix4::perspectiveProjection(35.0_degf, 4.0f/3.0f, 0.01f, 1000.0f)},
            // TODO hardcoded for the Buggy
            _cameraTransformation{Matrix4::translation(Vector3::zAxis(450.0f))},
            _manipulatorTransformation{
                Matrix4::rotationX(20.0_degf)*
                Matrix4::rotationY(-40.0_degf)};
        DrawType _drawType = DrawType::SceneGraph;

        DebugTools::FrameProfilerGL _profiler{
            DebugTools::FrameProfilerGL::Value::FrameTime|
            DebugTools::FrameProfilerGL::Value::CpuDuration|
            DebugTools::FrameProfilerGL::Value::GpuDuration, 150};

        Ui::UserInterfaceGL _ui{NoCreate};
        Ui::Label _profilerOutput{NoCreate};
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
        // TODO: fix the macro to correctly propagate && ohuh?
        const Containers::Optional<Trade::MaterialData> material = importer->material(i);
        auto&& phong = CORRADE_INTERNAL_ASSERT_EXPRESSION(material)->as<Trade::PhongMaterialData>();
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
    _emptyView.emplace(_combinedMesh).setCount(0);
    for(UnsignedInt i = 0; i != _meshes.size(); ++i) {
        arrayAppend(_meshViews, GL::MeshView{_combinedMesh})
            .setIndexOffset(meshOffsets[i])
            .setCount(meshOffsets[i + 1] - meshOffsets[i]);
    }

    /* Load the scene */
    const Trade::SceneData scene = *CORRADE_INTERNAL_ASSERT_EXPRESSION(importer->scene(importer->defaultScene()));

    /* (Object ID, parent ID or -1) mapping ordered in a way that puts parents
       before their children */
    // TODO don't need the order here yet, only subsequently for calculating transforms...
    _parentOrder = SceneTools::parentsBreadthFirst(scene);
    _absoluteTransformations = Containers::Array<Matrix4>{NoInit, std::size_t(scene.mappingBound()) + 1};

    // TODO explain this
    _transformations = Containers::Array<Matrix4>{ValueInit, std::size_t(scene.mappingBound())};
    for(const Containers::Pair<UnsignedInt, Matrix4>& transformation: scene.transformations3DAsArray())
        _transformations[transformation.first()] = transformation.second();

    // TODO document that this list is not ordered according to the hierarchy in any way
    Containers::Array<Containers::Pair<UnsignedInt, Containers::Pair<UnsignedInt, Int>>> meshesMaterials = scene.meshesMaterialsAsArray();
    _direct.transformationIds = Containers::Array<UnsignedInt>{NoInit, meshesMaterials.size()};
    _direct.absoluteTransformations = Containers::Array<Shaders::TransformationUniform3D>{ValueInit, meshesMaterials.size()};
    _direct.draws = Containers::Array<Shaders::PhongDrawUniform>{ValueInit, meshesMaterials.size()};
    _direct.meshes = Containers::Array<GL::Mesh*>{ValueInit, meshesMaterials.size()};
    _direct.meshViews = Containers::Array<Containers::Reference<GL::MeshView>>{DirectInit, meshesMaterials.size(), *_emptyView};
    for(std::size_t i = 0; i != meshesMaterials.size(); ++i) {
        _direct.transformationIds[i] = meshesMaterials[i].first();
        CORRADE_INTERNAL_ASSERT(meshesMaterials[i].second().second() != -1);
        _direct.draws[i].materialId = meshesMaterials[i].second().second();
        _direct.meshes[i] = &_meshes[meshesMaterials[i].second().first()];
        _direct.meshViews[i] = _meshViews[meshesMaterials[i].second().first()];
    }

    /* Projection, light setup. Just two lights right now. */
    _direct.projections = Containers::Array<Shaders::ProjectionUniform3D>{ValueInit, 1};
    _direct.lights = Containers::Array<Shaders::PhongLightUniform>{ValueInit, 2};

    /* Set up shaders. The multi-draw shaders and uniform storage are set up
       based on the data count we have */
    _shader = Shaders::PhongGL{Shaders::PhongGL::Configuration{}
        .setLightCount(2)};
    _shaderUniformBufferSingle = Shaders::PhongGL{Shaders::PhongGL::Configuration{}
        .setFlags(Shaders::PhongGL::Flag::UniformBuffers)
        .setLightCount(2)};
    _shaderUniformBufferMultiple = Shaders::PhongGL{Shaders::PhongGL::Configuration{}
        .setFlags(Shaders::PhongGL::Flag::UniformBuffers)
        .setLightCount(_direct.lights.size())
        .setMaterialCount(_direct.materials.size())
        .setDrawCount(Math::min<UnsignedInt>(1024, _direct.draws.size()))};
    _shaderUniformBufferMultiDraw = Shaders::PhongGL{Shaders::PhongGL::Configuration{}
        .setFlags(Shaders::PhongGL::Flag::UniformBuffers|
                  Shaders::PhongGL::Flag::MultiDraw)
        .setLightCount(_direct.lights.size())
        .setMaterialCount(_direct.materials.size())
        .setDrawCount(Math::min<UnsignedInt>(1024, _direct.draws.size()))};
    _direct.projections[0].projectionMatrix = _projection;
    #ifndef MAGNUM_TARGET_GLES
    // TODO do the buffer storage thing in a later commit, add a toggle for it
    if(GL::Context::current().isExtensionSupported<GL::Extensions::ARB::buffer_storage>()) {
        _uniformMulti.projectionUniform.setStorage(_direct.projections, {});
        _uniformMulti.materialUniform.setStorage(_direct.materials, {});
        for(std::size_t i = 0; i != UniformMultiCount; ++i) {
            _uniformMulti.transformationUniform[i].setStorage(_direct.draws.size()*sizeof(Shaders::TransformationUniform3D), {});
            _uniformMulti.drawUniform[i].setStorage(_direct.draws.size()*sizeof(Shaders::PhongDrawUniform), {});
            _uniformMulti.lightUniform[i].setStorage(_direct.lights.size()*sizeof(Shaders::PhongLightUniform), {});
        }
        _uniformMulti.transformationUniformStaging.setStorage(_direct.absoluteTransformations.size()*sizeof(Shaders::TransformationUniform3D), GL::Buffer::StorageFlag::DynamicStorage);
        _uniformMulti.drawUniformStaging.setStorage(_direct.draws.size()*sizeof(Shaders::PhongDrawUniform), GL::Buffer::StorageFlag::DynamicStorage);
        _uniformMulti.lightUniformStaging.setStorage(_direct.lights.size()*sizeof(Shaders::PhongLightUniform), GL::Buffer::StorageFlag::DynamicStorage);
    } else
    #endif
    {
        _uniformMulti.projectionUniform.setData(_direct.projections);
        _uniformMulti.materialUniform.setData(_direct.materials);
        for(std::size_t i = 0; i != UniformMultiCount; ++i) {
            _uniformMulti.transformationUniform[i].setData({nullptr, _direct.draws.size()*sizeof(Shaders::TransformationUniform3D)});
            _uniformMulti.drawUniform[i].setData({nullptr, _direct.draws.size()*sizeof(Shaders::PhongDrawUniform)});
            _uniformMulti.lightUniform[i].setData({nullptr, _direct.lights.size()*sizeof(Shaders::PhongLightUniform)});
        }
        _uniformMulti.transformationUniformStaging.setData({nullptr, _direct.absoluteTransformations.size()*sizeof(Shaders::TransformationUniform3D)}, GL::BufferUsage::DynamicDraw);
        _uniformMulti.drawUniformStaging.setData({nullptr, _direct.draws.size()*sizeof(Shaders::PhongDrawUniform)}, GL::BufferUsage::DynamicDraw);
        _uniformMulti.lightUniformStaging.setData({nullptr, _direct.lights.size()*sizeof(Shaders::PhongLightUniform)}, GL::BufferUsage::DynamicDraw);
    }

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

        /* Create transformed objects based on the hierarchy */
        Containers::Array<Object3D*> objects{ValueInit, std::size_t(scene.mappingBound())};
        Containers::Array<Containers::Pair<UnsignedInt, Int>> parents = scene.parentsAsArray();
        for(Containers::Pair<UnsignedInt, Int>& parent: parents)
            (objects[parent.first()] = new Object3D)->setTransformation(
                _transformations[parent.first()]);
        for(Containers::Pair<UnsignedInt, Int>& parent: parents)
            objects[parent.first()]->setParent(
                parent.second() == -1 ? &_sceneGraph.manipulator : objects[parent.second()]);

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

    if(args.isSet("no-profile")) // TODO document why, make sure the checkbox matches that
        _profiler.disable();

    /* Create the UI */
    {
        _ui.create(*this, Ui::DarkTheme{Ui::DarkTheme::Feature::Animations});
        /** @todo make a builtin API for this, or, better, make it automatic */
        CORRADE_INTERNAL_ASSERT(_ui.textLayer().shared().font(Ui::fontHandle(0, 1)).fillGlyphCache(_ui.textLayer().shared().glyphCache(), "μ"));

        /* The draw type enum is updated only from the UI but read in every
           draw event so it makes sense to just reference a member variable.
           Every time it changes the profiler needs to be reset to not display
           stale numbers. */
        Ui::EnumStorage<DrawType> drawType{_ui, Ui::NonOwned, _drawType};
        drawType->onUpdate([&](DrawType) {
            _profiler.enable();
        });
        // TODO srsly bools have to work here.... and be an enum set by default
        Ui::EnumStorage<Int> profile{_ui, DirectInit, true};
        profile.setEnumSet(true);
        profile->onUpdate([&](Int enabled) {
            enabled ? _profiler.enable() : _profiler.disable();
            if(!enabled)
                _profilerOutput.setText({}); // TODO or hide it? eventually when the layouter can handle that
        });

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
        }

        /* Two columns of radio buttons to toggle what draw type is used */
        Ui::SnapLayoutRow toggles = Ui::SnapLayout::snapRoot(_ui, Ui::Snap::Bottom);
        {
            Ui::SnapLayoutColumnLeft column = toggles.child(Ui::Snap::FillX);
            Ui::radioButton(column.child(),
                drawType.value<DrawType::SceneGraph>(), "SceneGraph");
            Ui::radioButton(column.child(),
                drawType.value<DrawType::DeduplicatedLoop>(), "Loop, deduplicated uniform setters");
            // TODO do all UBOs with views also, otherwise it makes no sense
            Ui::radioButton(column.child(),
                drawType.value<DrawType::UboUploadEach>(), "Loop, UBOs for each draw");
            // TODO also UBOs vs SSBOs?
            Ui::radioButton(column.child(),
                drawType.value<DrawType::UboUploadOnceSetOffsetMeshViews>(), "Loop, one UBO + offset, mesh views");
        } {
            Ui::SnapLayoutColumnLeft column = toggles.child(Ui::Snap::FillX);
            Ui::radioButton(column.child(),
                drawType.value<DrawType::TrivialLoop>(), "Trivial loop");
            Ui::radioButton(column.child(),
                drawType.value<DrawType::DeduplicatedLoopMeshViews>(), "Loop, deduplicated setters, mesh views");
            Ui::radioButton(column.child(),
                drawType.value<DrawType::UboUploadOnceSetOffset>(), "Loop, one UBO + draw offset");
            // TODO make disabled if it cannot be used
                // TODO didn't the original code have some extension checks?
            Ui::radioButton(column.child(),
                drawType.value<DrawType::UboUploadOnceSetOffsetMultiDraw>(), "UBOs + multidraw");
        }
    }

    // setSwapInterval(0); TODO make this toggleable as well?
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
        /* Calculate absolute transformations based on the parent order
           first. The first index in absoluteTransformations is the root
           transform that's applied to all others. */
        _absoluteTransformations[0] = _cameraTransformation.invertedRigid()*_manipulatorTransformation;
        for(Containers::Pair<UnsignedInt, Int> objectParent: _parentOrder)
            _absoluteTransformations[objectParent.first() + 1] = _absoluteTransformations[objectParent.second() + 1]*_transformations[objectParent.first()];

        /* Then copy those to corresponding draws. The mapping is not 1:1 so
           a single transformation may be used for multiple meshes but also
           none at all. */
        for(std::size_t i = 0; i != _direct.absoluteTransformations.size(); ++i) {
            // TODO explain the + 1
            const UnsignedInt transformationId = _direct.transformationIds[i] + 1;
            _direct.absoluteTransformations[i].setTransformationMatrix(_absoluteTransformations[transformationId]);
            _direct.draws[i].setNormalMatrix(_absoluteTransformations[transformationId].normalMatrix());
        }

        _direct.lights[0].position = {-300.0f, 100.0f, 100.0f, 0.0f};
        _direct.lights[1].position = {300.0f, 100.0f, 100.0f, 0.0f};

        /* Render all objects that have a mesh in a simple loop */
        const std::size_t objectCount = _direct.draws.size();
        if(_drawType == DrawType::TrivialLoop) {
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
                    .setTransformationMatrix(_direct.absoluteTransformations[i].transformationMatrix)
                    .setNormalMatrix({Vector4{_direct.draws[i].normalMatrix[0]}.xyz(),
                                      Vector4{_direct.draws[i].normalMatrix[1]}.xyz(),
                                      Vector4{_direct.draws[i].normalMatrix[2]}.xyz()})
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

            for(std::size_t i = 0; i != objectCount; ++i) {
                if(!_direct.meshes[i]) continue;
                const std::size_t materialId = _direct.draws[i].materialId;
                _shader
                    .setAmbientColor(_direct.materials[materialId].ambientColor)
                    .setDiffuseColor(_direct.materials[materialId].diffuseColor)
                    .setSpecularColor(_direct.materials[materialId].specularColor)
                    .setShininess(_direct.materials[materialId].shininess)
                    .setTransformationMatrix(_direct.absoluteTransformations[i].transformationMatrix)
                    .setNormalMatrix({Vector4{_direct.draws[i].normalMatrix[0]}.xyz(),
                                      Vector4{_direct.draws[i].normalMatrix[1]}.xyz(),
                                      Vector4{_direct.draws[i].normalMatrix[2]}.xyz()})
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

            for(std::size_t i = 0; i != objectCount; ++i) {
                if(!_direct.meshViews[i]->count()) continue;
                const std::size_t materialId = _direct.draws[i].materialId;
                _shader
                    .setAmbientColor(_direct.materials[materialId].ambientColor)
                    .setDiffuseColor(_direct.materials[materialId].diffuseColor)
                    .setSpecularColor(_direct.materials[materialId].specularColor)
                    .setShininess(_direct.materials[materialId].shininess)
                    .setTransformationMatrix(_direct.absoluteTransformations[i].transformationMatrix)
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
                _uniformSingle[_uniformSingleFrameId].transformationUniform.setSubData(0, _direct.absoluteTransformations.slice<1>(i));
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
            // TODO option to not do this even if ext available
            if(GL::Context::current().isExtensionSupported<GL::Extensions::ARB::buffer_storage>()) {
                _uniformMulti.lightUniformStaging.setSubData(0, _direct.lights);
                _uniformMulti.transformationUniformStaging.setSubData(0, _direct.absoluteTransformations);
                _uniformMulti.drawUniformStaging.setSubData(0, _direct.draws);
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
            } else
            #endif
            {
                _uniformMulti.lightUniform[_uniformMultiFrameId].setSubData(0, _direct.lights);
                _uniformMulti.transformationUniform[_uniformMultiFrameId].setSubData(0, _direct.absoluteTransformations);
                _uniformMulti.drawUniform[_uniformMultiFrameId].setSubData(0, _direct.draws);
            }

            ((_drawType == DrawType::UboUploadOnceSetOffset || _drawType == DrawType::UboUploadOnceSetOffsetMeshViews) ? _shaderUniformBufferMultiple : _shaderUniformBufferMultiDraw)
                .bindProjectionBuffer(_uniformMulti.projectionUniform)
                .bindMaterialBuffer(_uniformMulti.materialUniform)
                .bindLightBuffer(_uniformMulti.lightUniform[_uniformMultiFrameId])
                .bindTransformationBuffer(_uniformMulti.transformationUniform[_uniformMultiFrameId])
                .bindDrawBuffer(_uniformMulti.drawUniform[_uniformMultiFrameId]);

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
                // TODO eh wait, what, why not pass them all directly?
                    // TODO also this is inefficient, likely not calling
                // TODO use the indirect thing maybe?
                _shaderUniformBufferMultiDraw.draw(_direct.meshViews);
            }

            _uniformMultiFrameId = (_uniformMultiFrameId + 1) % UniformMultiCount;

        } else CORRADE_INTERNAL_ASSERT_UNREACHABLE();
    }

    _profiler.endFrame();
    // TODO uh any uhh any helper for this?
        // TODO what does it do for the very first frame?
    if(_profiler.measuredFrameCount() % 50 == 0)
        _profilerOutput.setText(_profiler.statistics(), Text::Alignment::MiddleLeft);

    /* Draw the UI */
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
        redraw();
}

void MultiDrawExample::mousePressEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left)
        _previousPosition = positionOnSphere(event.position());

    _ui.pointerPressEvent(event, now());

    if(_ui)
        redraw();
}

void MultiDrawExample::mouseReleaseEvent(MouseEvent& event) {
    if(event.button() == MouseEvent::Button::Left)
        _previousPosition = Vector3{};

    _ui.pointerReleaseEvent(event, now());

    if(_ui)
        redraw();
}

void MultiDrawExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(_ui.pointerMoveEvent(event, now())) {
        /* UI handles it */

    } else if((event.buttons() & MouseMoveEvent::Button::Left)) {
        // TODO er should be moving if started outside of the UI ...
        const Vector3 currentPosition = positionOnSphere(event.position());
        const Vector3 axis = Math::cross(_previousPosition, currentPosition);

        if(_previousPosition.length() < 0.001f || axis.length() < 0.001f) return;

        _manipulatorTransformation =
            Matrix4::rotation(Math::angle(_previousPosition, currentPosition), axis.normalized())*
            _manipulatorTransformation;
        _previousPosition = currentPosition;

        redraw();
    }

    if(_ui)
        redraw();

}

void MultiDrawExample::mouseScrollEvent(MouseScrollEvent& event) {
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

Vector3 MultiDrawExample::positionOnSphere(const Vector2i& position) const {
    const Vector2 positionNormalized = Vector2{position}/Vector2{windowSize()} - Vector2{0.5f};
    const Float length = positionNormalized.length();
    const Vector3 result(length > 1.0f ? Vector3(positionNormalized, 0.0f) : Vector3(positionNormalized, 1.0f - length));
    return (result*Vector3::yScale(-1.0f)).normalized();
}

}}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::MultiDrawExample)
