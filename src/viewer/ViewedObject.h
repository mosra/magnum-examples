#ifndef Magnum_Examples_ViewedObject_h
#define Magnum_Examples_ViewedObject_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013 Vladimír Vondruš <mosra@centrum.cz>

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

#include "Mesh.h"
#include <SceneGraph/AbstractCamera.h>
#include <SceneGraph/Drawable.h>
#include "SceneGraph/Object.h"
#include "Shaders/Phong.h"
#include "Trade/PhongMaterialData.h"

#include "Types.h"

namespace Magnum { namespace Examples {

class ViewedObject: public Object3D, SceneGraph::Drawable3D {
    public:
        ViewedObject(Mesh* mesh, Trade::PhongMaterialData* material, Shaders::Phong* shader, Object3D* parent, SceneGraph::DrawableGroup3D* group): Object3D(parent), SceneGraph::Drawable3D(this, group), mesh(mesh), ambientColor(material->ambientColor()), diffuseColor(material->diffuseColor()), specularColor(material->specularColor()), shininess(material->shininess()), shader(shader) {}

        void draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D* camera) override {
            shader->setAmbientColor(ambientColor)
                ->setDiffuseColor(diffuseColor)
                ->setSpecularColor(specularColor)
                ->setShininess(shininess)
                ->setLightPosition({-3.0f, 10.0f, 10.0f})
                ->setTransformationMatrix(transformationMatrix)
                ->setProjectionMatrix(camera->projectionMatrix())
                ->use();

            mesh->draw();
        }

    private:
        Mesh* mesh;
        Vector3 ambientColor,
            diffuseColor,
            specularColor;
        Float shininess;
        Shaders::Phong* shader;
};

}}

#endif
