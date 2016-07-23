/* magnum-shadows - A Cascading/Parallel-Split Shadow Mapping example
 * Written in 2016 by Bill Robinson airbaggins@gmail.com
 *
 * To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to
 * this software to the public domain worldwide. This software is distributed without any warranty.
 *
 * See <http://creativecommons.org/publicdomain/zero/1.0/>.
 *
 * Credit is appreciated, but not required.
 * */
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

