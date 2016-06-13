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

#include <Corrade/Utility/Resource.h>
#include <Magnum/AbstractShaderProgram.h>
#include <Magnum/Buffer.h>
#include <Magnum/Context.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Framebuffer.h>
#include <Magnum/Image.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Renderbuffer.h>
#include <Magnum/RenderbufferFormat.h>
#include <Magnum/Renderer.h>
#include <Magnum/Shader.h>
#include <Magnum/Texture.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/Version.h>
#include <Magnum/Math/Color.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Plane.h>
#include <Magnum/Primitives/UVSphere.h>
#include <Magnum/Trade/MeshData3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>

using namespace Magnum;
using namespace Magnum::Math::Literals;

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

class PhongIdShader: public AbstractShaderProgram {
    public:
        typedef Attribute<0, Vector3> Position;
        typedef Attribute<1, Vector3> Normal;

        enum: UnsignedInt {
            ColorOutput = 0,
            ObjectIdOutput = 1
        };

        explicit PhongIdShader();

        PhongIdShader& setObjectId(Int id) {
            setUniform(_objectIdUniform, id);
            return *this;
        }

        PhongIdShader& setLightPosition(const Vector3& position) {
            setUniform(_lightPositionUniform, position);
            return *this;
        }

        PhongIdShader& setAmbientColor(const Color3& color) {
            setUniform(_ambientColorUniform, color);
            return *this;
        }

        PhongIdShader& setColor(const Color3& color) {
            setUniform(_colorUniform, color);
            return *this;
        }

        PhongIdShader& setTransformationMatrix(const Matrix4& matrix) {
            setUniform(_transformationMatrixUniform, matrix);
            return *this;
        }

        PhongIdShader& setNormalMatrix(const Matrix3x3& matrix) {
            setUniform(_normalMatrixUniform, matrix);
            return *this;
        }

        PhongIdShader& setProjectionMatrix(const Matrix4& matrix) {
            setUniform(_projectionMatrixUniform, matrix);
            return *this;
        }

    private:
        Int _objectIdUniform,
            _lightPositionUniform,
            _ambientColorUniform,
            _colorUniform,
            _transformationMatrixUniform,
            _normalMatrixUniform,
            _projectionMatrixUniform;
};

PhongIdShader::PhongIdShader() {
    Utility::Resource rs("picking-data");

    Shader vert{Version::GL330, Shader::Type::Vertex},
        frag{Version::GL330, Shader::Type::Fragment};
    vert.addSource(rs.get("PhongId.vert"));
    frag.addSource(rs.get("PhongId.frag"));
    CORRADE_INTERNAL_ASSERT(Shader::compile({vert, frag}));
    attachShaders({vert, frag});
    CORRADE_INTERNAL_ASSERT(link());

    _objectIdUniform = uniformLocation("objectId");
    _lightPositionUniform = uniformLocation("light");
    _ambientColorUniform = uniformLocation("ambientColor");
    _colorUniform = uniformLocation("color");
    _transformationMatrixUniform = uniformLocation("transformationMatrix");
    _projectionMatrixUniform = uniformLocation("projectionMatrix");
    _normalMatrixUniform = uniformLocation("normalMatrix");
}

class PickableObject: public Object3D, SceneGraph::Drawable3D {
    public:
        explicit PickableObject(UnsignedByte id, PhongIdShader& shader, const Color3& color, Mesh& mesh, Object3D& parent, SceneGraph::DrawableGroup3D& drawables): Object3D{&parent}, SceneGraph::Drawable3D{*this, &drawables}, _id{id}, _selected{false}, _shader(shader), _color{color}, _mesh(mesh) {}

        void setSelected(bool selected) { _selected = selected; }

    private:
        virtual void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
            _shader.setTransformationMatrix(transformationMatrix)
                .setNormalMatrix(transformationMatrix.rotationScaling())
                .setProjectionMatrix(camera.projectionMatrix())
                .setAmbientColor(_selected ? _color*0.3f : Color3{})
                .setColor(_color*(_selected ? 2.0f : 1.0f))
                /* relative to the camera */
                .setLightPosition({13.0f, 2.0f, 5.0f})
                .setObjectId(_id);
            _mesh.draw(_shader);
        }

        UnsignedByte _id;
        bool _selected;
        PhongIdShader& _shader;
        Color3 _color;
        Mesh& _mesh;
};

class PickingExample: public Platform::Application {
    public:
        explicit PickingExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;

        Scene3D _scene;
        Object3D* _cameraObject;
        SceneGraph::Camera3D* _camera;
        SceneGraph::DrawableGroup3D _drawables;

        PhongIdShader _shader;
        Buffer _cubeVertices, _cubeIndices,
            _sphereVertices, _sphereIndices,
            _planeVertices;
        Mesh _cube, _plane, _sphere;

        enum { ObjectCount = 6 };
        PickableObject* _objects[ObjectCount];

        Framebuffer _framebuffer;
        Renderbuffer _color, _objectId, _depth;

        Vector2i _previousMousePosition, _mousePressPosition;
};

PickingExample::PickingExample(const Arguments& arguments): Platform::Application{arguments, Configuration{}.setTitle("Magnum object picking example")}, _framebuffer{defaultFramebuffer.viewport()} {
    MAGNUM_ASSERT_VERSION_SUPPORTED(Version::GL330);

    /* Global renderer configuration */
    Renderer::enable(Renderer::Feature::DepthTest);

    /* Configure framebuffer (using R8UI for object ID which means 255 objects max) */
    _color.setStorage(RenderbufferFormat::RGBA8, defaultFramebuffer.viewport().size());
    _objectId.setStorage(RenderbufferFormat::R8UI, defaultFramebuffer.viewport().size());
    _depth.setStorage(RenderbufferFormat::DepthComponent24, defaultFramebuffer.viewport().size());
    _framebuffer.attachRenderbuffer(Framebuffer::ColorAttachment{0}, _color)
               .attachRenderbuffer(Framebuffer::ColorAttachment{1}, _objectId)
               .attachRenderbuffer(Framebuffer::BufferAttachment::Depth, _depth)
               .mapForDraw({{PhongIdShader::ColorOutput, Framebuffer::ColorAttachment{0}},
                            {PhongIdShader::ObjectIdOutput, Framebuffer::ColorAttachment{1}}});
    CORRADE_INTERNAL_ASSERT(_framebuffer.checkStatus(FramebufferTarget::Draw) == Framebuffer::Status::Complete);

    /* Set up meshes */
    {
        Trade::MeshData3D data = Primitives::Cube::solid();
        _cubeVertices.setData(MeshTools::interleave(data.positions(0), data.normals(0)), BufferUsage::StaticDraw);
        _cubeIndices.setData(MeshTools::compressIndicesAs<UnsignedShort>(data.indices()), BufferUsage::StaticDraw);
        _cube.setCount(data.indices().size())
            .setPrimitive(data.primitive())
            .addVertexBuffer(_cubeVertices, 0, PhongIdShader::Position{}, PhongIdShader::Normal{})
            .setIndexBuffer(_cubeIndices, 0, Mesh::IndexType::UnsignedShort);
    } {
        Trade::MeshData3D data = Primitives::UVSphere::solid(16, 32);
        _sphereVertices.setData(MeshTools::interleave(data.positions(0), data.normals(0)), BufferUsage::StaticDraw);
        _sphereIndices.setData(MeshTools::compressIndicesAs<UnsignedShort>(data.indices()), BufferUsage::StaticDraw);
        _sphere.setCount(data.indices().size())
            .setPrimitive(data.primitive())
            .addVertexBuffer(_sphereVertices, 0, PhongIdShader::Position{}, PhongIdShader::Normal{})
            .setIndexBuffer(_sphereIndices, 0, Mesh::IndexType::UnsignedShort);
    } {
        Trade::MeshData3D data = Primitives::Plane::solid();
        _planeVertices.setData(MeshTools::interleave(data.positions(0), data.normals(0)), BufferUsage::StaticDraw);
        _plane.setCount(data.positions(0).size())
            .setPrimitive(data.primitive())
            .addVertexBuffer(_planeVertices, 0, PhongIdShader::Position{}, PhongIdShader::Normal{});
    }

    /* Set up objects */
    (*(_objects[0] = new PickableObject{1, _shader, Color3::fromHSV(25.0_degf, 0.9f, 0.9f), _cube, _scene, _drawables}))
        .rotate(34.0_degf, Vector3(1.0f).normalized())
        .translate({1.0f, 0.3f, -1.2f});
    (*(_objects[1] = new PickableObject{2, _shader, Color3::fromHSV(54.0_degf, 0.9f, 0.9f), _sphere, _scene, _drawables}))
        .translate({-1.2f, -0.3f, -0.2f});
    (*(_objects[2] = new PickableObject{3, _shader, Color3::fromHSV(105.0_degf, 0.9f, 0.9f), _plane, _scene, _drawables}))
        .rotate(254.0_degf, Vector3(1.0f).normalized())
        .scale(Vector3(0.45f))
        .translate({0.5f, 1.3f, 1.5f});
    (*(_objects[3] = new PickableObject{4, _shader, Color3::fromHSV(162.0_degf, 0.9f, 0.9f), _sphere, _scene, _drawables}))
        .translate({-0.2f, -1.7f, -2.7f});
    (*(_objects[4] = new PickableObject{5, _shader, Color3::fromHSV(210.0_degf, 0.9f, 0.9f), _sphere, _scene, _drawables}))
        .translate({0.7f, 0.6f, 2.2f})
        .scale(Vector3(0.75f));
    (*(_objects[5] = new PickableObject{6, _shader, Color3::fromHSV(280.0_degf, 0.9f, 0.9f), _cube, _scene, _drawables}))
        .rotate(-92.0_degf, Vector3(1.0f).normalized())
        .scale(Vector3(0.25f))
        .translate({-0.5f, -0.3f, 1.8f});

    /* Configure camera */
    _cameraObject = new Object3D{&_scene};
    _cameraObject->translate(Vector3::zAxis(8.0f));
    _camera = new SceneGraph::Camera3D{*_cameraObject};
    _camera->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
        .setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 4.0f/3.0f, 0.001f, 100.0f))
        .setViewport(defaultFramebuffer.viewport().size());
}

void PickingExample::drawEvent() {
    /* Draw to custom framebuffer */
    _framebuffer.clear(FramebufferClear::Color|FramebufferClear::Depth)
        .bind();
    _camera->draw(_drawables);

    /* Bind the main buffer back */
    defaultFramebuffer.bind();

    /* Blit color to window framebuffer */
    _framebuffer.mapForRead(Framebuffer::ColorAttachment{0});
    AbstractFramebuffer::blit(_framebuffer, defaultFramebuffer,
        {{}, _framebuffer.viewport().size()}, FramebufferBlit::Color);

    swapBuffers();
}

void PickingExample::mousePressEvent(MouseEvent& event) {
    if(event.button() != MouseEvent::Button::Left) return;

    _previousMousePosition = _mousePressPosition = event.position();
    event.setAccepted();
}

void PickingExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    const Vector2 delta = 3.0f*
        Vector2{event.position() - _previousMousePosition}/
        Vector2{defaultFramebuffer.viewport().size()};

    (*_cameraObject)
        .rotate(Rad{-delta.y()}, _cameraObject->transformation().right().normalized())
        .rotateY(Rad{-delta.x()});

    _previousMousePosition = event.position();
    event.setAccepted();
    redraw();
}

void PickingExample::mouseReleaseEvent(MouseEvent& event) {
    if(event.button() != MouseEvent::Button::Left || _mousePressPosition != event.position()) return;

    /* Read object ID at given click position (framebuffer has Y up while windowing system Y down) */
    _framebuffer.mapForRead(Framebuffer::ColorAttachment{1});
    Image2D data = _framebuffer.read(
        Range2Di::fromSize({event.position().x(), _framebuffer.viewport().sizeY() - event.position().y() - 1}, {1, 1}),
        {PixelFormat::RedInteger, PixelType::UnsignedByte});

    /* Highlight object under mouse and deselect all other */
    for(auto* o: _objects) o->setSelected(false);
    UnsignedByte id = data.data<UnsignedByte>()[0];
    if(id > 0 && id < ObjectCount + 1)
        _objects[id - 1]->setSelected(true);

    event.setAccepted();
    redraw();
}

MAGNUM_APPLICATION_MAIN(PickingExample)
