//
// Created by bill on 20/07/16.
//

#include <Corrade/Containers/Array.h>
#include "ShadowReceiverDrawable.h"
#include "ShadowReceiverShader.h"
#include "ShadowLight.h"

ShadowReceiverDrawable::ShadowReceiverDrawable(Magnum::SceneGraph::AbstractObject3D &object,
											   Magnum::SceneGraph::DrawableGroup3D *drawables)
: Drawable(object, drawables) {

}

void ShadowReceiverDrawable::draw(const Magnum::Matrix4 &transformationMatrix,
								  Magnum::SceneGraph::Camera<3, Magnum::Float> &camera) {
	shader->setTransformationProjectionMatrix(camera.projectionMatrix() * transformationMatrix);
	shader->setModelMatrix(object().transformationMatrix());

	mesh->draw(*shader);
}

