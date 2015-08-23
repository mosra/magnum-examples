/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015
              Vladimír Vondruš <mosra@centrum.cz>
    Copyright © 2015
              Jonathan Hale <squareys@googlemail.com>

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

#include "TexturedDrawable2D.h"

#include <Magnum/Mesh.h>
#include <Magnum/Texture.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/Shaders/Flat.h>

namespace Magnum { namespace Examples {

TexturedDrawable2D::TexturedDrawable2D(Mesh* mesh, Shaders::Flat2D* shader, Texture2D* texture, Object2D* parent, SceneGraph::DrawableGroup2D* group):
    Object2D(parent),
    SceneGraph::Drawable2D(*this, group),
    _mesh(mesh),
    _shader(shader),
    _texture(texture)
{
}

void TexturedDrawable2D::draw(const Matrix3& transformationMatrix, SceneGraph::Camera2D& camera) {
    _shader->setTransformationProjectionMatrix(transformationMatrix*camera.projectionMatrix())
            .setTexture(*_texture);

    _mesh->draw(*_shader);
}

}}
