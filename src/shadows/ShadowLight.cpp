#include "ShadowLight.h"
#include <Magnum/SceneGraph/FeatureGroup.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/ImageView.h>
#include <Magnum/Renderer.h>
#include <Magnum/TextureArray.h>
#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Vector3.h>
#include <algorithm>
#include "ShadowCasterDrawable.h"

using namespace Magnum;

ShadowLight::ShadowLight(Magnum::SceneGraph::Object<Magnum::SceneGraph::MatrixTransformation3D>& parent)
:	Magnum::SceneGraph::Camera3D(parent)
,	object(parent)
,	shadowTexture()
{
	setAspectRatioPolicy(Magnum::SceneGraph::AspectRatioPolicy::NotPreserved);
}

void ShadowLight::setupShadowmaps(int numShadowLevels, Magnum::Vector2i size) {
	layers.clear();
	shadowTexture = new Texture2DArray();
	shadowTexture->setLabel("Shadow texture");
	shadowTexture->setImage(0, TextureFormat::DepthComponent, ImageView3D(PixelFormat::DepthComponent, PixelType::Float, { size, numShadowLevels}, nullptr));
	shadowTexture->setMaxLevel(0);

	shadowTexture->setCompareFunction(Sampler::CompareFunction::LessOrEqual);
	shadowTexture->setCompareMode(Sampler::CompareMode::CompareRefToTexture);

	shadowTexture->setMinificationFilter(Sampler::Filter::Linear, Sampler::Mipmap::Base);
	shadowTexture->setMagnificationFilter(Sampler::Filter::Linear);

	for (int i = 0; i < numShadowLevels; i++) {
		layers.emplace_back(size);
		auto& shadowFramebuffer = this->layers.back().shadowFramebuffer;
		shadowFramebuffer.setLabel("Shadow framebuffer " + std::__cxx11::to_string(i));
		shadowFramebuffer.bind();
		shadowFramebuffer.attachTextureLayer(Framebuffer::BufferAttachment::Depth, *this->shadowTexture, 0, i);
		shadowFramebuffer.mapForDraw(Framebuffer::DrawAttachment::None);
		auto status = shadowFramebuffer.checkStatus(FramebufferTarget::ReadDraw);
		Magnum::Debug() << "Framebuffer status: " << status;
	}
}

ShadowLight::ShadowLayerData::ShadowLayerData(Magnum::Vector2i size)
:	shadowFramebuffer({{0,0},size}) { }

ShadowLight::~ShadowLight()
{
}

void ShadowLight::setTarget(Vector3 lightDirection, Vector3 screenDirection,
							SceneGraph::Camera3D &mainCamera)
{
	auto cameraMatrix = Magnum::Matrix4::lookAt({0,0,0}, -lightDirection, screenDirection);
	auto cameraRotationMatrix = cameraMatrix.rotation();
	auto inverseCameraRotationMatrix = cameraRotationMatrix.inverted();

	for (auto i = 0u; i < layers.size(); i++) {
		auto mainCameraFrustumCorners = getCameraFrustumCorners(mainCamera, i);
		auto& d = layers[i];
		Magnum::Vector3 min(std::numeric_limits<float>::max()), max(std::numeric_limits<float>::lowest());
		for (auto worldPoint : mainCameraFrustumCorners) {
			auto cameraPoint = inverseCameraRotationMatrix * worldPoint;
			for (size_t i = 0; i < 3; i++) {
				if (cameraPoint[i] < min[i]) {
					min[i] = cameraPoint[i];
				}
				if (cameraPoint[i] > max[i]) {
					max[i] = cameraPoint[i];
				}
			}
		}
		auto mid = (min+max) * 0.5f;

		auto cameraPosition = cameraRotationMatrix * mid;

		auto range = max - min;
		d.orthographicSize = range.xy();
		d.orthographicNear = -0.5f * range.z();
		d.orthographicFar =  0.5f * range.z();
		cameraMatrix.translation() = cameraPosition;
		d.shadowCameraMatrix = cameraMatrix;
	}
}

void ShadowLight::setNearFar(float zNear, float zFar) {
	cutPlanes.clear();
	cutPlanes.reserve(layers.size());
	//props http://stackoverflow.com/a/33465663
	for (auto i = 1u; i <= layers.size(); i++) {
//		float linearDepth = zNear + i * (zFar - zNear) / numLayers;
//		float linearDepth = zNear + (numLayers - i) * (zFar) / numLayers;
		float linearDepth = zNear + std::pow(float(i) / layers.size(), 3.0f) * (zFar - zNear);
		float nonLinearDepth = (zFar + zNear - 2.0f * zNear * zFar / linearDepth) / (zFar - zNear);
		cutPlanes.push_back((nonLinearDepth + 1.0f) / 2.0f);
	}
}

std::vector<Magnum::Vector3> ShadowLight::getCameraFrustumCorners(Magnum::SceneGraph::Camera3D &mainCamera, int layer) {
	auto imvp = (mainCamera.projectionMatrix() * mainCamera.cameraMatrix()).inverted();
	auto projectImvpAndDivide = [&](Vector4 vec) -> Magnum::Vector3 {
		auto vec2 = imvp * vec;
		return vec2.xyz() / vec2.w();
	};
	auto z0 = layer == 0 ? 0 : cutPlanes[layer-1];
	auto z1 = cutPlanes[layer];
	return std::vector<Vector3>{
		projectImvpAndDivide({-1,-1, z0, 1}),
		projectImvpAndDivide({ 1,-1, z0, 1}),
		projectImvpAndDivide({-1, 1, z0, 1}),
		projectImvpAndDivide({ 1, 1, z0, 1}),
		projectImvpAndDivide({-1,-1, z1, 1}),
		projectImvpAndDivide({ 1,-1, z1, 1}),
		projectImvpAndDivide({-1, 1, z1, 1}),
		projectImvpAndDivide({ 1, 1, z1, 1}),
	};
}

std::vector<Magnum::Vector4> ShadowLight::calculateClipPlanes() {
	Magnum::Matrix4 pm = projectionMatrix();
	std::vector<Magnum::Vector4> clipPlanes = {{
		Magnum::Vector4( pm[3][0]-pm[2][0], pm[3][1]-pm[2][1], pm[3][2]-pm[2][2], pm[3][3]-pm[2][3] ), // far
		Magnum::Vector4( pm[3][0]+pm[2][0], pm[3][1]+pm[2][1], pm[3][2]+pm[2][2], pm[3][3]+pm[2][3] ), // near

		Magnum::Vector4( pm[3][0]+pm[0][0], pm[3][1]+pm[0][1], pm[3][2]+pm[0][2], pm[3][3]+pm[0][3] ), // left
		Magnum::Vector4( pm[3][0]-pm[0][0], pm[3][1]-pm[0][1], pm[3][2]-pm[0][2], pm[3][3]-pm[0][3] ), // right

		Magnum::Vector4( pm[3][0]+pm[1][0], pm[3][1]+pm[1][1], pm[3][2]+pm[1][2], pm[3][3]+pm[1][3] ), // bottom
		Magnum::Vector4( pm[3][0]-pm[1][0], pm[3][1]-pm[1][1], pm[3][2]-pm[1][2], pm[3][3]-pm[1][3] ), // top
	}};
	for (auto& plane : clipPlanes) {
		plane *= plane.xyz().lengthInverted();
	}
	return clipPlanes;
}

void ShadowLight::render(Magnum::SceneGraph::DrawableGroup3D& drawables)
{
	/* Compute transformations of all objects in the group relative to the camera */
	std::vector<std::reference_wrapper<Magnum::SceneGraph::AbstractObject3D>> objects;
	objects.reserve(drawables.size());
	for (size_t i = 0; i < drawables.size(); i++) {
		objects.push_back(drawables[i].object());
	}
	std::vector<ShadowCasterDrawable*> filteredDrawables;

	auto bias = Magnum::Matrix4{
			{0.5f, 0.0f, 0.0f, 0.0f},
			{0.0f, 0.5f, 0.0f, 0.0f},
			{0.0f, 0.0f, 0.5f, 0.0f},
			{0.5f, 0.5f, 0.5f, 1.0f}
	};

	for (auto layer = 0u; layer < layers.size(); layer++) {
		auto& d = layers[layer];
		auto orthographicNear = d.orthographicNear;
		auto orthographicFar = d.orthographicFar;
		object.setTransformation(d.shadowCameraMatrix);
		object.setClean();
		setProjectionMatrix(Magnum::Matrix4::orthographicProjection(d.orthographicSize, orthographicNear, orthographicFar));
		auto clipPlanes = calculateClipPlanes();

		auto transformations = object.scene()->AbstractObject<3,Float>::transformationMatrices(objects, cameraMatrix());
		auto transformationsOutIndex = 0u;

		filteredDrawables.clear();
		for (size_t drawableIndex = 0; drawableIndex < drawables.size(); drawableIndex++) {
			auto& drawable = static_cast<ShadowCasterDrawable&>(drawables[drawableIndex]);
			auto transform = transformations[drawableIndex];
			// If your centre is offset, inject it here
			const Vector4 &localCentre = Magnum::Vector4{0, 0, 0, 1};
			Magnum::Vector4 drawableCentre = transform * localCentre;
			for (size_t clipPlaneIndex = 1; clipPlaneIndex < clipPlanes.size(); clipPlaneIndex++) {
				auto distance = Magnum::Math::dot(clipPlanes[clipPlaneIndex], drawableCentre);
				if (distance < -drawable.getRadius()) {
					goto next;
				}
			}
			{
				auto nearestPoint = drawableCentre.z() + drawable.getRadius();
				if (nearestPoint > orthographicFar) {
					orthographicFar = nearestPoint;
				}
				filteredDrawables.push_back(&drawable);
				transformations[transformationsOutIndex++] = transform;
			}
			next:;
		}

		auto shadowCameraProjectionMatrix = Magnum::Matrix4::orthographicProjection(d.orthographicSize, -orthographicNear, -orthographicFar);
		d.shadowMatrix = bias * shadowCameraProjectionMatrix * cameraMatrix();
		setProjectionMatrix(shadowCameraProjectionMatrix);

		Magnum::Renderer::enable(Magnum::Renderer::Feature::DepthTest);
		Magnum::Renderer::setDepthMask(true);
		d.shadowFramebuffer.clear(Magnum::FramebufferClear::Depth);
		d.shadowFramebuffer.bind();
		for(std::size_t i = 0; i != transformationsOutIndex; ++i) {
			filteredDrawables[i]->draw(transformations[i], *this);
		}

		Magnum::defaultFramebuffer.bind();
	}

}

