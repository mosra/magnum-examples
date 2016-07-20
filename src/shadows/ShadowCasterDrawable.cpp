#include "ShadowCasterDrawable.h"
#include "ShadowCasterShader.h"
#include "Magnum/SceneGraph/AbstractCamera.h"


ShadowCasterDrawable::ShadowCasterDrawable(Magnum::SceneGraph::AbstractObject3D& parent,
										   Magnum::SceneGraph::DrawableGroup3D *drawables)
:	Magnum::SceneGraph::Drawable3D(parent, drawables)
{
}

ShadowCasterDrawable::~ShadowCasterDrawable()
{

}

void ShadowCasterDrawable::draw(const Magnum::Matrix4& transformationMatrix, Magnum::SceneGraph::Camera3D& shadowCamera)
{
	shader->setTransformationMatrix(shadowCamera.projectionMatrix() * transformationMatrix);
	mesh->draw(*shader);
}

