#ifndef Magnum_Examples_Icosphere_h
#define Magnum_Examples_Icosphere_h
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

#include <Magnum/Color.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/Shaders/Shaders.h>

#include "Types.h"

namespace Magnum { namespace Examples {

class Icosphere: public Object3D, SceneGraph::Drawable3D {
    public:
        Icosphere(Mesh* mesh, Shaders::Phong* shader, const Vector3& color, Object3D* parent, SceneGraph::DrawableGroup3D* group);

        inline void advance(Rad angle) { rotate(angle, Vector3::zAxis()); }

        void draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D& camera) override;

    private:
        Mesh* mesh;
        Shaders::Phong* shader;
        Color3 color;
};

}}

#endif
