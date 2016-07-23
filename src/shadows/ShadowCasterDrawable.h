#pragma once
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/Resource.h>
#include <Magnum/Mesh.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/SceneGraph/Object.h>

class ShadowCasterShader;

class ShadowCasterDrawable
:	public Magnum::SceneGraph::Drawable3D
{
public:
	ShadowCasterDrawable(Magnum::SceneGraph::AbstractObject3D& parent, Magnum::SceneGraph::DrawableGroup3D *drawables);

	virtual ~ShadowCasterDrawable();

	/* Mesh to use for this drawable and its bounding sphere radius. */
    void setMesh(Magnum::Mesh* mesh, float radius) {
		this->mesh = mesh;
		this->radius = radius;
	}
	void setShader(ShadowCasterShader* shader) {
		this->shader = shader;
	}

	float getRadius() const { return radius; }

	virtual void draw(const Magnum::Matrix4& transformationMatrix, Magnum::SceneGraph::Camera3D& shadowCamera) override;

private:
	Magnum::Mesh* mesh;
	ShadowCasterShader* shader;
	float radius;
};

