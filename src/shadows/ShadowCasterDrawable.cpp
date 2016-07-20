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
#include "ShadowCasterDrawable.h"
#include "ShadowCasterShader.h"
#include "Magnum/SceneGraph/Camera.h"


ShadowCasterDrawable::ShadowCasterDrawable(Magnum::SceneGraph::AbstractObject3D& parent,
                                           Magnum::SceneGraph::DrawableGroup3D *drawables)
:   Magnum::SceneGraph::Drawable3D(parent, drawables)
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

