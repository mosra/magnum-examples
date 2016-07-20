#pragma once

#include <memory>

#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/Framebuffer.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/Resource.h>
#include <Magnum/SceneGraph/AbstractFeature.h>


class ShadowLight : public Magnum::SceneGraph::Camera3D
{
public:
	ShadowLight(Magnum::SceneGraph::Object<Magnum::SceneGraph::MatrixTransformation3D>& parent);
    virtual ~ShadowLight();

	void setupShadowmaps(int numShadowLevels, Magnum::Vector2i size);

	void setNearFar(float cameraNear, float cameraFar);

	void setTarget(Magnum::Vector3 lightDirection, Magnum::Vector3 screenDirection,
				   Magnum::SceneGraph::Camera3D &mainCamera);
	
	void render(Magnum::SceneGraph::DrawableGroup3D& drawables);
	
	std::vector<Magnum::Vector3> getCameraFrustumCorners(Magnum::SceneGraph::Camera3D &mainCamera, int layer);

	size_t getNumLayers() const { return layers.size(); }

	const Magnum::Matrix4& getLayerMatrix(int layer) const {
		return layers[layer].shadowMatrix;
	}

	const std::vector<float>& getCutPlanes() const {
		return cutPlanes;
	}

	std::vector<Magnum::Vector4> calculateClipPlanes();

	Magnum::Texture2DArray *getShadowTexture() const {
		return shadowTexture;
	}

private:
	Magnum::SceneGraph::Object<Magnum::SceneGraph::MatrixTransformation3D>& object;
	Magnum::Texture2DArray* shadowTexture;
	std::vector<float> cutPlanes;

	struct ShadowLayerData {
		Magnum::Framebuffer shadowFramebuffer;
		Magnum::Matrix4 shadowCameraMatrix;
		Magnum::Matrix4 shadowMatrix;
		Magnum::Vector2 orthographicSize;
		float orthographicNear, orthographicFar;

		ShadowLayerData(Magnum::Vector2i size);
	};

	std::vector<ShadowLayerData> layers;
};

