#pragma once

#include <memory>

#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/Framebuffer.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/Resource.h>
#include <Magnum/SceneGraph/AbstractFeature.h>


/** A special camera used to render shadow maps. The object it's attached to should face the direction that the light travels. */
class ShadowLight : public Magnum::SceneGraph::Camera3D
{
public:
	ShadowLight(Magnum::SceneGraph::Object<Magnum::SceneGraph::MatrixTransformation3D>& parent);
    virtual ~ShadowLight();

	/** Initialize the shadow map texture array and framebuffers, should be called before setupSplitDistances. */
	void setupShadowmaps(int numShadowLevels, Magnum::Vector2i size);

	/** Sets up the distances we should cut the view frustum along. The distances will be distributed along a power series.
	 * Should be called after setupShadowmaps */
	void setupSplitDistances(float cameraNear, float cameraFar, float power);

	/** Computes all the matrices for the shadow map splits. Should be called whenever your camera moves.
	 * @param lightDirection Direction of travel of the light.
	 * @param screenDirection Crossed with light direction to determine orientation of the shadow maps. Use the forward direction of the camera for best resolution use, or use a constant value for more stable shadows.
	 * @param mainCamera The camera to use to determine the optimal splits (normally, the main camera that the shadows will be rendered to). */

	void setTarget(Magnum::Vector3 lightDirection, Magnum::Vector3 screenDirection,
				   Magnum::SceneGraph::Camera3D &mainCamera);

	/** Render a group of shadow-casting drawables to the shadow maps */
	void render(Magnum::SceneGraph::DrawableGroup3D& drawables);
	
	std::vector<Magnum::Vector3> getLayerFrustumCorners(Magnum::SceneGraph::Camera3D &mainCamera, int layer);
	static std::vector<Magnum::Vector3> getCameraFrustumCorners(Magnum::SceneGraph::Camera3D &mainCamera, float z0 = -1, float z1 = 1);
	static std::vector<Magnum::Vector3> getFrustumCorners(const Magnum::Matrix4 &imvp, float z0, float z1);
	float getCutZ(int layer) const;

	size_t getNumLayers() const { return layers.size(); }

	const Magnum::Matrix4& getLayerMatrix(int layer) const {
		return layers[layer].shadowMatrix;
	}

	std::vector<Magnum::Vector4> calculateClipPlanes();

	Magnum::Texture2DArray *getShadowTexture() const {
		return shadowTexture;
	}

private:
	Magnum::SceneGraph::Object<Magnum::SceneGraph::MatrixTransformation3D>& object;
	Magnum::Texture2DArray* shadowTexture;

	struct ShadowLayerData {
		Magnum::Framebuffer shadowFramebuffer;
		Magnum::Matrix4 shadowCameraMatrix;
		Magnum::Matrix4 shadowMatrix;
		Magnum::Vector2 orthographicSize;
		float orthographicNear, orthographicFar;
		float cutPlane;

		ShadowLayerData(Magnum::Vector2i size);
	};

	std::vector<ShadowLayerData> layers;
};

