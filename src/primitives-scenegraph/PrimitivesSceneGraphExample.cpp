/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
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

#include <Magnum/GL/Buffer.h>
#include <Magnum/GL/DefaultFramebuffer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/GL/Renderer.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/UVSphere.h>
#include <Magnum/Primitives/Plane.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/MeshData.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>

namespace Magnum { namespace Examples {

using namespace Magnum::Math::Literals;

typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D> Scene3D;

class DrawableObject : public Object3D, SceneGraph::Drawable3D {
	public:
		explicit DrawableObject(Shaders::Phong& shader, const Color3& color, GL::Mesh& mesh, Object3D& parent, SceneGraph::DrawableGroup3D& drawables) : Object3D{ &parent }, SceneGraph::Drawable3D{ *this, &drawables }, _shader(shader), _color{ color }, _mesh(mesh) {}

		void setColor(Color3 color) { _color = color; }
		Color3 getColor() { return _color; }

	private:
		virtual void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D& camera) {
			_shader.setTransformationMatrix(transformationMatrix)
				.setNormalMatrix(transformationMatrix.normalMatrix())
				.setProjectionMatrix(camera.projectionMatrix())
				.setAmbientColor(_color*0.3f)
				.setDiffuseColor(_color)
				/* relative to the camera */
				.setLightPosition({ 13.0f, 2.0f, 5.0f })
				.draw(_mesh);
		}
		
		GL::Mesh& _mesh;
		Shaders::Phong& _shader;
		Color3 _color;
};


class PrimitivesSceneGraphExample: public Platform::Application {
    public:
        explicit PrimitivesSceneGraphExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;

		Scene3D _scene;
		Object3D* _cameraObject;
		SceneGraph::Camera3D* _camera;
		SceneGraph::DrawableGroup3D _drawables;

		Shaders::Phong _shader;
		GL::Mesh _cube, _plane, _sphere, _sphereWireframe;

		enum { ObjectCount = 4 };
		DrawableObject* _objects[ObjectCount];

        Matrix4 _transformation, _projection;
		Vector2i _previousMousePosition, _mousePressPosition;
};



PrimitivesSceneGraphExample::PrimitivesSceneGraphExample(const Arguments& arguments):
    Platform::Application{arguments, Configuration{}
        .setTitle("Magnum Primitives SceneGraph Example")}
{
	/* Global renderer configuration */
    GL::Renderer::enable(GL::Renderer::Feature::DepthTest);
    GL::Renderer::enable(GL::Renderer::Feature::FaceCulling);

	/* Set up meshes */
	_cube = MeshTools::compile(Primitives::cubeSolid());
	_sphere = MeshTools::compile(Primitives::uvSphereSolid(16, 32));
	_sphereWireframe = MeshTools::compile(Primitives::uvSphereWireframe(16, 32));
	_plane = MeshTools::compile(Primitives::planeSolid());

    _projection =
        Matrix4::perspectiveProjection(
            35.0_degf, Vector2{windowSize()}.aspectRatio(), 0.01f, 100.0f)*
        Matrix4::translation(Vector3::zAxis(-10.0f));

	/* Set up objects*/
	(*(_objects[0] = new DrawableObject{_shader, 0x3bd267_rgbf, _cube, _scene, _drawables }))
		.rotateZ(45.0_degf)
		.rotateX(-45.0_degf)
		.translate({ 0.5f, 0.0f, -2.0f });

	(*(_objects[1] = new DrawableObject{_shader, 0x2f83cc_rgbf, _sphere, _scene, _drawables }))
		.scale(Vector3(0.75f))
		.translate({ -1.2f, -0.3f, -0.2f });

	/* This uvSphereWireframe object has its parent set to the uvSphereSolid object, rather than
		the _scene*/
	(*(_objects[2] = new DrawableObject{ _shader, 0x2f83cc_rgbf, _sphereWireframe, (*_objects[1]), _drawables }))
		.scale(Vector3(1.01f));

	(*(_objects[3] = new DrawableObject{_shader, 0xcd3431_rgbf, _plane, _scene, _drawables }))
		.rotate(30.0_degf, Vector3(1.0f).normalized())
		.rotateX(-30.0_degf)
		.scale(Vector3(2.0f))
		.translate({ 0.0f, -0.30f, -3.3f });

	/* Configure camera */
	_cameraObject = new Object3D{ &_scene };
	_cameraObject->translate(Vector3::zAxis(8.0f));
	_camera = new SceneGraph::Camera3D{ *_cameraObject };
	_camera->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
		.setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, 4.0f / 3.0f, 0.001f, 100.0f))
		.setViewport(GL::defaultFramebuffer.viewport().size());
}

void PrimitivesSceneGraphExample::drawEvent() {
    GL::defaultFramebuffer.clear(
        GL::FramebufferClear::Color|GL::FramebufferClear::Depth);

	_camera->draw(_drawables);
    swapBuffers();
}

void PrimitivesSceneGraphExample::mousePressEvent(MouseEvent& event) {
    if(event.button() != MouseEvent::Button::Left) return;

	_previousMousePosition = _mousePressPosition = event.position();
    event.setAccepted();
}

void PrimitivesSceneGraphExample::mouseReleaseEvent(MouseEvent& event) {	
	if (event.button() != MouseEvent::Button::Left || _mousePressPosition != event.position()) return;
	//Change the color of each object
	for (auto* o : _objects) o->setColor(Color3::fromHsv({ o->getColor().hue() + 50.0_degf, 1.0f, 1.0f }));

    event.setAccepted();
    redraw();
}

void PrimitivesSceneGraphExample::mouseMoveEvent(MouseMoveEvent& event) {
	/* We have to take window size, not framebuffer size, since the position is
	in window coordinates and the two can be different on HiDPI systems */
	const Vector2 delta = 3.0f*
		Vector2{ event.position() - _previousMousePosition } /
		Vector2{ windowSize() };

	//
	if ((event.buttons() & MouseMoveEvent::Button::Right)) {
		(*_cameraObject)
			.rotate(Rad{ -delta.y() }, _cameraObject->transformation().right().normalized())
			.rotateY(Rad{ -delta.x() });
	}
	
	if ((event.buttons() & MouseMoveEvent::Button::Left)) {
		for (auto* o : _objects) {
			(*o).transformLocal(Matrix4::rotationX(Rad{ delta.y() }) * Matrix4::rotationY(Rad{ delta.x() }));
		}
	}
	
	_previousMousePosition = event.position();
	event.setAccepted();
	redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::PrimitivesSceneGraphExample)
