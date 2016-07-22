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
#include <Magnum/Primitives/Icosphere.h>
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

	bool isDebugCameraActive() const;
};

const int NUM_SHADER_LEVELS = 3;

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
    addModel(Primitives::Capsule3D::solid(1,1,4,1));
	addModel(Primitives::Capsule3D::solid(6,1,9,1));
//    addModel(Primitives::Cylinder::solid(6,6,1,Primitives::Cylinder::Flag::CapEnds)); // The caps were floating for me

	auto ground = createSceneObject(models[0], false, true);
	ground->setTransformation(Magnum::Matrix4::scaling({100,1,100}));

	for (int i = 0; i < 200; i++) {
		auto& model = models[rand() % models.size()];
		auto object = createSceneObject(model, true, true);
		object->setTransformation(Magnum::Matrix4::translation({rand()*100.0f/RAND_MAX - 50, rand()*10.0f/RAND_MAX - 5, rand()*100.0f/RAND_MAX - 50}));
	}

    float near = 0.01f;
    float far = 100.0f;

    shadowLight.setCutPlanes(near, far, 3.0f);
    mainCamera.setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, Vector2{defaultFramebuffer.viewport().size()}.aspectRatio(), near, far));
	mainCameraObject.setTransformation(Matrix4::translation({0,3,0}));

	debugCamera.setProjectionMatrix(Matrix4::perspectiveProjection(35.0_degf, Vector2{defaultFramebuffer.viewport().size()}.aspectRatio(), near / 4.0f, far * 4.0f));
	debugCameraObject.setTransformation(Matrix4::lookAt({100,50,0},{0,0,-30},{0,1,0}));

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
	Matrix4 imvp;
	if (isDebugCameraActive()) {
		debugLines.reset();
		imvp = (mainCamera.projectionMatrix() * mainCamera.cameraMatrix()).inverted();
//		debugLines.addFrustum(imvp, {1, 0, 0});
//		debugLines.addFrustum((shadowLight.projectionMatrix() * shadowLight.cameraMatrix()).inverted(), {0,0,1});
	}


	shadowLight.setTarget({1,2,1}, mainCameraObject.transformation()[2].xyz(), mainCamera);

    shadowLight.render(shadowCasterDrawables);

	Magnum::Renderer::setClearColor({0.1f,0.1f,0.4f,1.0f});
	defaultFramebuffer.clear(FramebufferClear::Color|FramebufferClear::Depth);

    Corrade::Containers::Array<Magnum::Matrix4> shadowMatrices(Corrade::Containers::NoInit, shadowLight.getNumLayers());
    for (auto layerIndex = 0u; layerIndex < shadowLight.getNumLayers(); layerIndex++) {
        shadowMatrices[layerIndex] = shadowLight.getLayerMatrix(layerIndex);
		if (isDebugCameraActive()) {
			debugLines.addFrustum((Matrix4{{2,  0,  0,  0},
										   {0,  2,  0,  0},
										   {0,  0,  2,  0},
										   {-1, -1, -1, 1}} * shadowMatrices[layerIndex]).inverted(),
								  Color3::fromHSV(layerIndex * 360.0_degf / shadowLight.getNumLayers(), 1.0f, 0.5f));
			debugLines.addFrustum(imvp,
								  Color3::fromHSV(layerIndex * 360.0_degf / shadowLight.getNumLayers(), 1.0f, 1.0f),
								  layerIndex == 0 ? 0 : shadowLight.getCutZ(layerIndex - 1),
								  shadowLight.getCutZ(layerIndex));
		}
    }
    shadowReceiverShader.setShadowmapMatrices(shadowMatrices);
    shadowReceiverShader.setShadowmapTexture(*shadowLight.getShadowTexture());
	shadowReceiverShader.setLightDirection(shadowLightObject.transformation()[2].xyz());

    activeCamera->draw(shadowReceiverDrawables);

	if (isDebugCameraActive()) {
		debugLines.draw(activeCamera->projectionMatrix() * activeCamera->cameraMatrix());
	}

    swapBuffers();
	if (!mainCameraVelocity.isZero()) {
		auto transform = activeCameraObject->transformation();
		transform.translation() += transform.rotation() * mainCameraVelocity * 0.3f;
		activeCameraObject->setTransformation(transform);
		redraw();
	}
}

bool ShadowsExample::isDebugCameraActive() const { return activeCamera == &debugCamera; }

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
	else if (event.key() == KeyEvent::Key::PageUp) {
		mainCameraVelocity.y() = 1;
	}
	else if (event.key() == KeyEvent::Key::PageDown) {
		mainCameraVelocity.y() = -1;
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
	else if (event.key() == KeyEvent::Key::PageDown || event.key() == KeyEvent::Key::PageUp) {
		mainCameraVelocity.y() = 0;
	}
	else if (event.key() == KeyEvent::Key::Right || event.key() == KeyEvent::Key::Left) {
		mainCameraVelocity.x() = 0;
	}
	redraw();
}

}}

MAGNUM_APPLICATION_MAIN(Magnum::Examples::ShadowsExample)
