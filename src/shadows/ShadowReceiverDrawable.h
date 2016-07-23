#pragma once
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/Mesh.h>

class ShadowReceiverShader;
class ShadowLight;

/// Drawable that should render shadows cast by casters
class ShadowReceiverDrawable : public Magnum::SceneGraph::Drawable3D {
public:
	ShadowReceiverDrawable(Magnum::SceneGraph::AbstractObject3D &object,
						   Magnum::SceneGraph::DrawableGroup3D *drawables);

	virtual void draw(const Magnum::Matrix4 &transformationMatrix,
					  Magnum::SceneGraph::Camera3D &camera) override;

	void setMesh(Magnum::Mesh *mesh) {
		this->mesh = mesh;
	}

	void setShader(ShadowReceiverShader *shader) {
		this->shader = shader;
	}

private:
	Magnum::Mesh* mesh;
	ShadowReceiverShader* shader;
};



