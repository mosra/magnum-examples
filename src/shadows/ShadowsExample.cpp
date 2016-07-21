#include <Magnum/Buffer.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/Renderer.h>
#include <Magnum/MeshTools/Interleave.h>
#include <Magnum/MeshTools/CompressIndices.h>
#include <Magnum/Platform/Sdl2Application.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Cylinder.h>
#include <Magnum/Primitives/Capsule.h>
#include <Magnum/Primitives/Plane.h>
#include <Magnum/Shaders/Phong.h>
#include <Magnum/Trade/MeshData3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/AbstractObject.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include "ShadowCasterShader.h"
#include "ShadowReceiverShader.h"
#include "ShadowLight.h"
#include "ShadowCasterDrawable.h"
#include "ShadowReceiverDrawable.h"
#include "DebugLines.h"

namespace Magnum { namespace Examples {

using namespace Magnum::Math::Literals;

typedef Magnum::SceneGraph::Object<Magnum::SceneGraph::MatrixTransformation3D> Object3D;
typedef Magnum::SceneGraph::Scene<Magnum::SceneGraph::MatrixTransformation3D> Scene3D;

class ShadowsExample: public Platform::Application {
    public:
        explicit ShadowsExample(const Arguments& arguments);

    private:
        void drawEvent() override;
        void mousePressEvent(MouseEvent& event) override;
        void mouseReleaseEvent(MouseEvent& event) override;
        void mouseMoveEvent(MouseMoveEvent& event) override;
		void keyPressEvent(KeyEvent &event) override;
		void keyReleaseEvent(KeyEvent &event) override;

		void addModel(const Trade::MeshData3D &meshData3D);

		struct Model {
			Buffer indexBuffer, vertexBuffer;
			Mesh mesh;
			float radius;
		};
		Object3D* createSceneObject(Model &model, bool makeCaster, bool makeReceiver);

		Scene3D scene;
	    Magnum::SceneGraph::DrawableGroup3D shadowCasterDrawables;
		Magnum::SceneGraph::DrawableGroup3D shadowReceiverDrawables;
		ShadowCasterShader shadowCasterShader;
		ShadowReceiverShader shadowReceiverShader;

		DebugLines debugLines;

		Object3D shadowLightObject;
		ShadowLight shadowLight;
        Object3D mainCameraObject;
		Magnum::SceneGraph::Camera3D mainCamera;
		Object3D debugCameraObject;
		Magnum::SceneGraph::Camera3D debugCamera;

		Object3D* activeCameraObject;
		Magnum::SceneGraph::Camera3D* activeCamera;

		std::vector<Model> models;

		Magnum::Vector3 mainCameraVelocity;
};

const int NUM_SHADER_LEVELS = 4;

ShadowsExample::ShadowsExample(const Arguments& arguments): Platform::Application{arguments, Configuration{}.setTitle("Magnum Shadows Example")}
,   scene()
,	shadowReceiverShader(NUM_SHADER_LEVELS)
,	shadowLightObject(&scene)
,   shadowLight(shadowLightObject)
,   mainCameraObject(&scene)
,   mainCamera(mainCameraObject)
,   debugCameraObject(&scene)
,   debugCamera(debugCameraObject)
{
    shadowLight.setupShadowmaps(NUM_SHADER_LEVELS, {1024,1024});

    Renderer::enable(Renderer::Feature::DepthTest);
    Renderer::enable(Renderer::Feature::FaceCulling);

	addModel(Primitives::Cube::solid());
    addModel(Primitives::Capsule3D::solid(6,6,6,1));
    addModel(Primitives::Cylinder::solid(6,6,0.5f,Primitives::Cylinder::Flag::CapEnds));

	auto ground = createSceneObject(models[0], false, true);
	ground->setTransformation(Magnum::Matrix4::scaling({100,1,100}));

	for (int i = 0; i < 200; i++) {
		auto& model = models[rand() % models.size()];
		auto object = createSceneObject(model, true, true);
		object->setTransformation(Magnum::Matrix4::translation({rand()*100.0f/RAND_MAX - 50, rand()*10.0f/RAND_MAX, rand()*100.0f/RAND_MAX - 50}));
	}

    float near = 0.01f;
    float far = 100.0f;

    shadowLight.setCutPlanes(near, far, 3.0f);
    mainCamera.setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, Vector2{defaultFramebuffer.viewport().size()}.aspectRatio(), near, far));
	mainCameraObject.setTransformation(Matrix4::translation({0,4,0}));

	debugCamera.setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, Vector2{defaultFramebuffer.viewport().size()}.aspectRatio(), near / 4.0f, far * 4.0f));
	debugCameraObject.setTransformation(Matrix4::translation({0,10,50}));

	activeCamera = &mainCamera;
	activeCameraObject = &mainCameraObject;

	shadowLightObject.setTransformation(Matrix4::lookAt(
			{0.1f,1,0.1f},
			{0,0,0},
			{0,1,0}
	));
}

Object3D* ShadowsExample::createSceneObject(ShadowsExample::Model &model, bool makeCaster, bool makeReceiver) {
	auto object = new Object3D(&scene);

	if (makeCaster) {
		auto caster = new ShadowCasterDrawable(*object, &shadowCasterDrawables);
		caster->setShader(&shadowCasterShader);
		caster->setMesh(&model.mesh, model.radius);
	}

	if (makeReceiver) {
		auto receiver = new ShadowReceiverDrawable(*object, &shadowReceiverDrawables);
		receiver->setShader(&shadowReceiverShader);
		receiver->setMesh(&model.mesh);
	}
	return object;
}

void ShadowsExample::addModel(const Trade::MeshData3D &meshData3D) {
    models.emplace_back();
    auto& model = this->models.back();

    model.vertexBuffer.setData(MeshTools::interleave(meshData3D.positions(0), meshData3D.normals(0)), BufferUsage::StaticDraw);

    float maxMagnitudeSquared = 0.0f;
    for (auto position : meshData3D.positions(0)) {
        auto magnitudeSquared = Math::dot(position,position);
        if (magnitudeSquared > maxMagnitudeSquared) {
            maxMagnitudeSquared = magnitudeSquared;
        }
    }
    model.radius = std::sqrt(maxMagnitudeSquared);

    Containers::Array<char> indexData;
    Mesh::IndexType indexType;
    UnsignedInt indexStart, indexEnd;
    std::tie(indexData, indexType, indexStart, indexEnd) = MeshTools::compressIndices(meshData3D.indices());
    model.indexBuffer.setData(indexData, BufferUsage::StaticDraw);

    model.mesh.setPrimitive(meshData3D.primitive())
        .setCount(meshData3D.indices().size())
        .addVertexBuffer(model.vertexBuffer, 0, Shaders::Phong::Position{}, Shaders::Phong::Normal{})
        .setIndexBuffer(model.indexBuffer, 0, indexType, indexStart, indexEnd);

}

void ShadowsExample::drawEvent() {

	debugLines.reset();
	debugLines.addFrustum((mainCamera.projectionMatrix() * mainCamera.cameraMatrix()).inverted(), {1,0,0});
	debugLines.addFrustum((shadowLight.projectionMatrix() * shadowLight.cameraMatrix()).inverted(), {1,0,0});


	shadowLight.setTarget({0.2f,0.2f,1.0f}, mainCameraObject.transformation()[2].xyz(), mainCamera);

    shadowLight.render(shadowCasterDrawables);

	Magnum::Renderer::setClearColor({0.1f,0.1f,0.4f,1.0f});
	defaultFramebuffer.clear(FramebufferClear::Color|FramebufferClear::Depth);

    Corrade::Containers::Array<Magnum::Matrix4> shadowMatrices(Corrade::Containers::NoInit, shadowLight.getNumLayers());
    for (auto layerIndex = 0u; layerIndex < shadowLight.getNumLayers(); layerIndex++) {
        shadowMatrices[layerIndex] = shadowLight.getLayerMatrix(layerIndex);
		debugLines.addFrustum((Matrix4{{2,0,0,0},{0,2,0,0},{0,0,2,0},{-1,-1,-1,1}} * shadowMatrices[layerIndex]).inverted(), {1,0,0});
    }
    shadowReceiverShader.setShadowmapMatrices(shadowMatrices);
    shadowReceiverShader.setShadowmapTexture(*shadowLight.getShadowTexture());

    activeCamera->draw(shadowReceiverDrawables);

	shadowReceiverShader.setLightDirection(activeCamera->cameraMatrix().rotationScaling() * shadowLightObject.transformation()[2].xyz());
	debugLines.draw(activeCamera->projectionMatrix() * activeCamera->cameraMatrix());

    swapBuffers();
	if (!mainCameraVelocity.isZero()) {
		auto transform = activeCameraObject->transformation();
		transform.translation() += transform.rotation() * mainCameraVelocity * 0.1f;
		activeCameraObject->setTransformation(transform);
		redraw();
	}
}

void ShadowsExample::mousePressEvent(MouseEvent& event) {
    if(event.button() != MouseEvent::Button::Left) return;

    event.setAccepted();
}

void ShadowsExample::mouseReleaseEvent(MouseEvent& event) {

    event.setAccepted();
    redraw();
}

void ShadowsExample::mouseMoveEvent(MouseMoveEvent& event) {
    if(!(event.buttons() & MouseMoveEvent::Button::Left)) return;

    auto transform = activeCameraObject->transformation();

	constexpr float angleScale = 0.01f;
	auto angleX = event.relativePosition().x() * angleScale;
	auto angleY = event.relativePosition().y() * angleScale;
	if (angleX != 0 || angleY != 0) {
		transform = Matrix4::lookAt(transform.translation(), transform.translation() - transform.rotationScaling() * Magnum::Vector3{-angleX, angleY, 1}, {0,1,0});

		activeCameraObject->setTransformation(transform);
	}


    event.setAccepted();
    redraw();
}

void ShadowsExample::keyPressEvent(Platform::Sdl2Application::KeyEvent &event) {
	if (event.key() == KeyEvent::Key::Up) {
		mainCameraVelocity.z() = -1;
	}
	else if (event.key() == KeyEvent::Key::Down) {
		mainCameraVelocity.z() = 1;
	}
	else if (event.key() == KeyEvent::Key::Right) {
		mainCameraVelocity.x() = 1;
	}
	else if (event.key() == KeyEvent::Key::Left) {
		mainCameraVelocity.x() = -1;
	}
	else if (event.key() == KeyEvent::Key::F1) {
		activeCamera = &mainCamera;
		activeCameraObject = &mainCameraObject;
	}
	else if (event.key() == KeyEvent::Key::F2) {
		activeCamera = &debugCamera;
		activeCameraObject = &debugCameraObject;
	}
	redraw();
}

void ShadowsExample::keyReleaseEvent(Platform::Sdl2Application::KeyEvent &event) {
	if (event.key() == KeyEvent::Key::Up || event.key() == KeyEvent::Key::Down) {
		mainCameraVelocity.z() = 0;
	}
	else if (event.key() == KeyEvent::Key::Right || event.key() == KeyEvent::Key::Left) {
		mainCameraVelocity.x() = 0;
	}
	redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::ShadowsExample)
