/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014
              Vladimír Vondruš <mosra@centrum.cz>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include "Icosphere.h"

#include <Magnum/Mesh.h>
#include <Magnum/SceneGraph/Camera3D.h>
#include <Magnum/Shaders/Phong.h>

namespace Magnum { namespace Examples {

Icosphere::Icosphere(Mesh* mesh, Shaders::Phong* shader, const Vector3& color, Object3D* parent, SceneGraph::DrawableGroup3D* group): Object3D(parent), SceneGraph::Drawable3D(*this, group), mesh(mesh), shader(shader), color(color) {
    scale(Vector3(0.1f));
}

void Icosphere::draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D& camera) {
    shader->setDiffuseColor(color)
        .setSpecularColor(Color3(1.0f))
        .setShininess(20)
        .setLightPosition({3.0f, -3.0f, 3.0f})
        .setTransformationMatrix(transformationMatrix)
        .setNormalMatrix(transformationMatrix.rotationScaling())
        .setProjectionMatrix(camera.projectionMatrix())
        .use();

    mesh->draw();
}

}}
