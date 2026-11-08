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

#include <Corrade/Utility/Arguments.h>
#include <Corrade/Utility/Path.h>
#include <Corrade/Utility/Resource.h>
#include <Corrade/PluginManager/Manager.h>
#include <Magnum/ImageView.h>
#include <Magnum/GL/AbstractShaderProgram.h>
#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/Context.h>
#include <Magnum/GL/CubeMapTexture.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/GL/Texture.h>
#include <Magnum/GL/TextureFormat.h>
#include <Magnum/Math/Color.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/MeshTools/FlipNormals.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/UVSphere.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/ImageData.h>
#include <Magnum/Trade/MeshData.h>

#include "CubeMapShader.h"
#include "ReflectorShader.h"

#include "configure.h"

namespace Magnum { namespace Examples {

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

class CubeMap: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit CubeMap(GL::Mesh& mesh, GL::CubeMapTexture& texture, CubeMapShader& shader, Object3D* parent, SceneGraph::DrawableGroup3D* group): Object3D{parent}, SceneGraph::Drawable3D{*this, group}, _mesh(mesh), _texture(texture), _shader(shader) {}

        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override {
            _shader
                .setTransformationProjectionMatrix(camera.projectionMatrix()*transformationMatrix)
                .bindTexture(_texture)
                .draw(_mesh);
        }

    private:
        GL::Mesh& _mesh;
        GL::CubeMapTexture& _texture;
        CubeMapShader& _shader;
};

class Reflector: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit Reflector(GL::Mesh& mesh, GL::CubeMapTexture& cubeTexture, GL::Texture2D& tarnishTexture, ReflectorShader& shader, Object3D* parent, SceneGraph::DrawableGroup3D* group): Object3D(parent), SceneGraph::Drawable3D(*this, group), _mesh(mesh), _cubeMapTexture(cubeTexture), _tarnishTexture(tarnishTexture), _shader(shader) {}

        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override {
            _shader
                .setTransformationMatrix(transformationMatrix)
                .setNormalMatrix(transformationMatrix.normalMatrix())
                .setProjectionMatrix(camera.projectionMatrix())
                .setReflectivity(2.0f)
                .setDiffuseColor(Color3(0.3f))
                .setCameraMatrix(camera.object().absoluteTransformationMatrix().rotationScaling())
                .bindCubeMapTexture(_cubeMapTexture)
                .bindTarnishTexture(_tarnishTexture)
                .draw(_mesh);
        }

    private:
        GL::Mesh& _mesh;
        GL::CubeMapTexture& _cubeMapTexture;
        GL::Texture2D& _tarnishTexture;
        ReflectorShader& _shader;
};

class CubeMapExample: public Platform::Application {
    public:
        explicit CubeMapExample(const Arguments& arguments);

    private:
        void viewportEvent(ViewportEvent& event) override;
        void drawEvent() override;
        void keyPressEvent(KeyEvent& event) override;

        /* The GL context creation is delayed, so delay GL object creation as
           well */
        GL::Mesh _cubeMesh{NoCreate};
        GL::Mesh _reflectorMesh{NoCreate};
        GL::CubeMapTexture _cubeMapTexture{NoCreate};
        GL::Texture2D _tarnishTexture{NoCreate};
        CubeMapShader _cubeMapShader{NoCreate};
        ReflectorShader _reflectorShader{NoCreate};

        Scene3D _scene;
        SceneGraph::DrawableGroup3D _drawables;
        Object3D* _cameraObject;
        SceneGraph::Camera3D* _camera;
};

using namespace Math::Literals;

CubeMapExample::CubeMapExample(const Arguments& arguments): Platform::Application{arguments, NoCreate} {
    /* Try to locate the bundled images, first in the source directory, then in
       the installation directory and as a fallback next to the executable */
    Containers::String defaultPath;
    if(Utility::Path::exists(CUBEMAP_EXAMPLE_DIR))
        defaultPath = CUBEMAP_EXAMPLE_DIR;
    else if(Utility::Path::exists(CUBEMAP_EXAMPLE_INSTALL_DIR))
        defaultPath = CUBEMAP_EXAMPLE_INSTALL_DIR;
    else
        defaultPath = Utility::Path::split(*Utility::Path::executableLocation()).first();

    /* Finally, provide a way for the user to override the path */
    Utility::Arguments args;
    args.addFinalOptionalArgument("path", defaultPath)
            .setHelp("path", "a combined cube map file (such as an EXR) or a directory where the +x.jpg, +y.jpg, ... files are")
        .addSkippedPrefix("magnum", "engine-specific options")
        .setGlobalHelp("Cube map rendering example")
        .parse(arguments.argc, arguments.argv);

    /* Create the context after parsing arguments to avoid a flickering window
       in case parse() exits */
    create(Configuration{}.setTitle("Magnum Cube Map Example"));

    /* Image importer */
    PluginManager::Manager<Trade::AbstractImporter> manager;
    Containers::Pointer<Trade::AbstractImporter> importer = manager.loadAndInstantiate("AnyImageImporter");
    if(!importer)
        Fatal{} << "Cannot load an image importer plugin";

    /* Cube mesh. We'll look at it from the inside, so flip face winding */
    _cubeMesh = MeshTools::compile(MeshTools::flipFaceWinding(Primitives::cubeSolid()));

    /* Reflector mesh */
    _reflectorMesh = MeshTools::compile(MeshTools::compressIndices(
        Primitives::uvSphereSolid(16, 32,
            Primitives::UVSphereFlag::TextureCoordinates)));

    /* Cube map texture */
    {
        _cubeMapTexture = GL::CubeMapTexture{};
        _cubeMapTexture
            .setWrapping(GL::SamplerWrapping::ClampToEdge)
            .setMagnificationFilter(GL::SamplerFilter::Linear)
            .setMinificationFilter(GL::SamplerFilter::Linear, GL::SamplerMipmap::Linear);

        /* If this is already a file, maybe it's a cubemap file already? */
        const Containers::StringView prefix = args.value<Containers::StringView>("path");
        if(Utility::Path::exists(prefix) && !Utility::Path::isDirectory(prefix)) {
            /** @todo clean up once there's ImageFlag::CubeMap and all that */
            if(!importer->openFile(prefix))
                Fatal{} << "Cannot open" << prefix;
            if(!importer->image3DCount())
                Fatal{} << "No 3D images in" << prefix;
            Containers::Optional<Trade::ImageData3D> image;
            if(!(image = importer->image3D(0)) || image->size().z() != 6)
                Fatal{} << "Cannot open the image as a cube map";

            const Vector2i size = image->size().xy();
            /** @todo mips?! */
            if(image->isCompressed())
                _cubeMapTexture
                    .setStorage(Math::log2(size.min())+1, GL::textureFormat(image->compressedFormat()), size)
                    .setCompressedSubImage(0, {}, *image);
            else
                _cubeMapTexture
                    .setStorage(Math::log2(size.min())+1, GL::textureFormat(image->format()), size)
                    .setSubImage(0, {}, *image);

        /* Otherwise open six different files */
        } else {
            /* Configure texture storage using size of first image */
            if(!importer->openFile(Utility::Path::join(prefix, "+x.jpg")))
                Fatal{} << "Cannot open a cube map face image";
            Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
            CORRADE_INTERNAL_ASSERT(image);
            Vector2i size = image->size();
            _cubeMapTexture
                .setStorage(Math::log2(size.min())+1, GL::TextureFormat::RGB8, size)
                .setSubImage(GL::CubeMapCoordinate::PositiveX, 0, {}, *image);

            if(!importer->openFile(Utility::Path::join(prefix, "-x.jpg")))
                Fatal{} << "Cannot open a cube map face image";
            _cubeMapTexture.setSubImage(GL::CubeMapCoordinate::NegativeX, 0, {},
                *CORRADE_INTERNAL_ASSERT_EXPRESSION(importer->image2D(0)));

            if(!importer->openFile(Utility::Path::join(prefix, "+y.jpg")))
                Fatal{} << "Cannot open a cube map face image";
            _cubeMapTexture.setSubImage(GL::CubeMapCoordinate::PositiveY, 0, {},
                *CORRADE_INTERNAL_ASSERT_EXPRESSION(importer->image2D(0)));

            if(!importer->openFile(Utility::Path::join(prefix, "-y.jpg")))
                Fatal{} << "Cannot open a cube map face image";
            _cubeMapTexture.setSubImage(GL::CubeMapCoordinate::NegativeY, 0, {},
                *CORRADE_INTERNAL_ASSERT_EXPRESSION(importer->image2D(0)));

            if(!importer->openFile(Utility::Path::join(prefix, "+z.jpg")))
                Fatal{} << "Cannot open a cube map face image";
            _cubeMapTexture.setSubImage(GL::CubeMapCoordinate::PositiveZ, 0, {},
                *CORRADE_INTERNAL_ASSERT_EXPRESSION(importer->image2D(0)));

            if(!importer->openFile(Utility::Path::join(prefix, "-z.jpg")))
                Fatal{} << "Cannot open a cube map face image";
            _cubeMapTexture.setSubImage(GL::CubeMapCoordinate::NegativeZ, 0, {},
                *CORRADE_INTERNAL_ASSERT_EXPRESSION(importer->image2D(0)));
        }

        _cubeMapTexture.generateMipmap();
    }

    /* Tarnish texture */
    {
        Utility::Resource rs{"data"};
        if(!importer->openData(rs.getRaw("tarnish.jpg")))
            Fatal{} << "Cannot open a reflector tarnish image";

        Containers::Optional<Trade::ImageData2D> image = importer->image2D(0);
        CORRADE_INTERNAL_ASSERT(image);
        _tarnishTexture = GL::Texture2D{};
        _tarnishTexture
            .setWrapping(GL::SamplerWrapping::ClampToEdge)
            .setMagnificationFilter(GL::SamplerFilter::Linear)
            .setMinificationFilter(GL::SamplerFilter::Linear, GL::SamplerMipmap::Linear)
            .setStorage(Math::log2(image->size().min())+1, GL::TextureFormat::RGB8, image->size())
            .setSubImage(0, {}, *image)
            .generateMipmap();
    }

    /* Create shaders */
    _cubeMapShader = CubeMapShader{};
    _reflectorShader = ReflectorShader{};

    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

    /* Set up a perspective camera */
    (_cameraObject = new Object3D(&_scene))
        ->translate(Vector3::zAxis(3.0f));
    (_camera = new SceneGraph::Camera3D(*_cameraObject))
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(55.0_degf, 1.0f, 0.001f, 100.0f))
        .setViewport(GL::defaultFramebuffer.viewport().size());

    /* Add objects to scene */
    (new CubeMap(_cubeMesh, _cubeMapTexture, _cubeMapShader, &_scene, &_drawables))
        ->scale(Vector3(20.0f));

    (new Reflector(_reflectorMesh, _cubeMapTexture, _tarnishTexture, _reflectorShader, &_scene, &_drawables))
        ->scale(Vector3(0.5f))
        .translate(Vector3::xAxis(-0.5f));

    (new Reflector(_reflectorMesh, _cubeMapTexture, _tarnishTexture, _reflectorShader, &_scene, &_drawables))
        ->scale(Vector3(0.3f))
        .rotate(37.0_degf, Vector3::xAxis())
        .translate(Vector3::xAxis(0.3f));
}

void CubeMapExample::viewportEvent(ViewportEvent& event) {
    GL::defaultFramebuffer.setViewport({{}, event.framebufferSize()});
    _camera->setViewport(event.windowSize());
}

void CubeMapExample::drawEvent() {
    GL::defaultFramebuffer.clear(GL::FramebufferClear::Depth);
    GL::defaultFramebuffer.invalidate({GL::DefaultFramebuffer::InvalidationAttachment::Color});

    _camera->draw(_drawables);
    swapBuffers();
}

void CubeMapExample::keyPressEvent(KeyEvent& event) {
    if(event.key() == Key::Up)
        _cameraObject->rotate(-10.0_degf, _cameraObject->transformation().right().normalized());

    else if(event.key() == Key::Down)
        _cameraObject->rotate(10.0_degf, _cameraObject->transformation().right().normalized());

    else if(event.key() == Key::Left || event.key() == Key::Right) {
        Float translationY = _cameraObject->transformation().translation().y();
        _cameraObject->translate(Vector3::yAxis(-translationY))
            .rotateY(event.key() == Key::Left ? 10.0_degf : -10.0_degf)
            .translate(Vector3::yAxis(translationY));

    } else return;

    redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::CubeMapExample)
