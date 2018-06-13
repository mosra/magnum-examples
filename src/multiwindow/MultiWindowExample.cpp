/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015, 2016
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
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Mesh.h>
#include <Magnum/Renderer.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Trade/AbstractImporter.h>
#include <Magnum/Trade/MeshData3D.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Shaders/Flat.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/DualQuaternionTransformation.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/Scene.h>

#include "configure.h"

namespace Magnum { namespace Examples {

typedef SceneGraph::Object<SceneGraph::DualQuaternionTransformation> Object3D;
typedef SceneGraph::Scene<SceneGraph::DualQuaternionTransformation> Scene3D;

class ColoredDrawable: SceneGraph::Drawable3D {
    public:
        explicit ColoredDrawable(Mesh& mesh, Shaders::Phong& shader, Object3D& object, SceneGraph::DrawableGroup3D* group);

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Mesh& _mesh;
        Shaders::Phong& _shader;
};

class LineDrawable: SceneGraph::Drawable3D {
    public:
        explicit LineDrawable(Mesh& mesh, Shaders::Flat3D& shader, Object3D& object, SceneGraph::DrawableGroup3D* group);

    private:
        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) override;

        Mesh& _mesh;
        Shaders::Flat3D& _shader;
};

class SecondaryWindow: public Platform::ApplicationWindow {
    public:
        explicit SecondaryWindow(class MultiWindowExample& application);

    private:
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;

        MultiWindowExample& application();

        Shaders::Flat3D _shader;

        LineDrawable* _lineDrawable;
        SceneGraph::DrawableGroup3D _lineDrawables;
        Vector2i _previousPosition;
};

class MultiWindowExample: public Platform::Application {
    friend SecondaryWindow;

    public:
        explicit MultiWindowExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;

        std::unique_ptr<Buffer> _vertices, _indices;
        Mesh _mesh{NoCreate};
        Shaders::Phong _shader;

        Scene3D _scene;
        Object3D *_object, *_cameraObject;
        ColoredDrawable* _coloredDrawable;
        SceneGraph::Camera3D* _camera;
        SceneGraph::DrawableGroup3D _drawables;
        Vector2i _previousPosition;

        std::optional<SecondaryWindow> _secondaryWindow;
};

MultiWindowExample::MultiWindowExample(const Arguments& arguments): Platform::Application{arguments, Configuration{}.setTitle("Magnum MultiWindow Example")} {
    Utility::Arguments args;
    args.addArgument("file").setHelp("file", "file to load")
        .setHelp("Loads and displays 3D mesh file (such as OBJ, OpenGEX or COLLADA one) provided on command-line.")
        .parse(arguments.argc, arguments.argv);

    /* Every scene needs a camera */
    (_cameraObject = new Object3D{&_scene})
        ->translate(Vector3::zAxis(5.0f));
    (_camera = new SceneGraph::Camera3D{*_cameraObject})
        ->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(Deg(35.0f), 1.0f, 0.001f, 100))
        .setViewport(defaultFramebuffer.viewport().size());
    Renderer::enable(Renderer::Feature::DepthTest);
    Renderer::enable(Renderer::Feature::FaceCulling);

    /* Load scene importer plugin */
    PluginManager::Manager<Trade::AbstractImporter> manager{MAGNUM_PLUGINS_IMPORTER_DIR};
    std::unique_ptr<Trade::AbstractImporter> importer = manager.loadAndInstantiate("AnySceneImporter");
    if(!importer) std::exit(1);

    /* Open the scene */
    if(!importer->openFile(args.value("file")))
        Fatal{} << "Cannot open" << args.value("file");

    /* Load the mesh */
    if(!importer->mesh3DCount())
        Fatal{} << "No meshes found in the file";
    std::optional<Trade::MeshData3D> meshData = importer->mesh3D(0);
    if(!meshData || !meshData->hasNormals() || meshData->primitive() != MeshPrimitive::Triangles)
        Fatal{} << "Cannot load the mesh or the mesh doesn't have triangles with normals";

    /* Compile the mesh */
    std::tie(_mesh, _vertices, _indices) = MeshTools::compile(*meshData, BufferUsage::StaticDraw);

    /* Add it as object to the scene */
    _object = new Object3D{&_scene};

    /* Configure the drawable and shader */
    _shader.setDiffuseColor(Color3{0.80f});
    new ColoredDrawable{_mesh, _shader, *_object, &_drawables};

    /* Set up secondary window */
    _secondaryWindow.emplace(*this);
}

SecondaryWindow::SecondaryWindow(MultiWindowExample& application): Platform::ApplicationWindow{application, WindowConfiguration{}.setTitle("Magnum MultiWindow Example: secondary window").setSize({640, 480})} {
    /* Configure the secondary shader */
    _shader.setColor(Color3::red(0.80f));
    new LineDrawable{application._mesh, _shader, *application._object, &_lineDrawables};
}

void MultiWindowExample::mousePressEvent(MouseEvent& event) {
    if(event.button() != MouseEvent::Button::Left) return;

    _previousPosition = event.position();
    event.setAccepted();
}

void MultiWindowExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    const Vector2 delta = 3.0f*
        Vector2{event.position() - _previousPosition}/
        Vector2{defaultFramebuffer.viewport().size()};

    _object->rotateYLocal(Rad{delta.x()})
        .rotateX(Rad{delta.y()})
        .normalizeRotation();

    _previousPosition = event.position();
    event.setAccepted();
    redraw();
    _secondaryWindow->redraw();
}

void MultiWindowExample::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color|FramebufferClear::Depth);

    _camera->draw(_drawables);

    swapBuffers();
}

inline MultiWindowExample& SecondaryWindow::application() {
    return static_cast<MultiWindowExample&>(Platform::ApplicationWindow::application());
}

void SecondaryWindow::mousePressEvent(MouseEvent& event) {
    if(event.button() != MouseEvent::Button::Left) return;

    _previousPosition = event.position();
    event.setAccepted();
}

void SecondaryWindow::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    const Vector2 delta = 3.0f*
        Vector2{event.position() - _previousPosition}/
        Vector2{defaultFramebuffer.viewport().size()};

    application()._object->rotateYLocal(Rad{delta.x()})
        .rotateX(Rad{delta.y()})
        .normalizeRotation();

    _previousPosition = event.position();
    event.setAccepted();
    redraw();
    application().redraw();
}

void SecondaryWindow::drawEvent() {
    defaultFramebuffer.clear(FramebufferClear::Color|FramebufferClear::Depth);

    /* Draw the object as lines */
    Renderer::setPolygonMode(Renderer::PolygonMode::Line);
    application()._camera->draw(_lineDrawables);
    Renderer::setPolygonMode(Renderer::PolygonMode::Fill);

    swapBuffers();
}

ColoredDrawable::ColoredDrawable(Mesh& mesh, Shaders::Phong& shader, Object3D& object, SceneGraph::DrawableGroup3D* const group): SceneGraph::Drawable3D{object, group}, _mesh{mesh}, _shader{shader} {}

void ColoredDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setLightPosition(camera.cameraMatrix().transformPoint({-3.0f, 10.0f, 10.0f}))
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.rotation())
        .setProjectionMatrix(camera.projectionMatrix());
    _mesh.draw(_shader);
}

LineDrawable::LineDrawable(Mesh& mesh, Shaders::Flat3D& shader, Object3D& object, SceneGraph::DrawableGroup3D* const group): SceneGraph::Drawable3D{object, group}, _mesh{mesh}, _shader{shader} {}

void LineDrawable::draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
    _shader
        .setTransformationProjectionMatrix(camera.projectionMatrix()*transformationMatrix);
    _mesh.draw(_shader);
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::MultiWindowExample)
