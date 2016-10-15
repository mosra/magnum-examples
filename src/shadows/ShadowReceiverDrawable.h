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

    void setShader(ShadowReceiverShader* shader) {
        this->shader = shader;
    }

private:
    Magnum::Mesh* mesh;
    ShadowReceiverShader* shader;
};



