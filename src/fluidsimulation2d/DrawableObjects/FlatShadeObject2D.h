#ifndef Magnum_Examples_DrawableObjects_FlatShadeObject2D_h
#define Magnum_Examples_DrawableObjects_FlatShadeObject2D_h
/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
            Vladimír Vondruš <mosra@centrum.cz>
        2019 — Nghia Truong <nghiatruong.vn@gmail.com>

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <Magnum/GL/Mesh.h>
#include <Magnum/Math/Color.h>
#include <Magnum/Shaders/Flat.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation2D.h>

namespace Magnum { namespace Examples {

using Object2D = SceneGraph::Object<SceneGraph::MatrixTransformation2D>;

class FlatShadeObject2D: public SceneGraph::Drawable2D {
    public:
        explicit FlatShadeObject2D(Object2D& object, Shaders::Flat2D& shader, const Color3& color, GL::Mesh& mesh, SceneGraph::DrawableGroup2D* const drawables):
            SceneGraph::Drawable2D{object, drawables}, _shader(shader), _color(color), _mesh(mesh) {}

        void draw(const Matrix3& transformation, SceneGraph::Camera2D& camera) override {
            if(!_bEnabled) return;

            _shader
                .setColor(_color)
                .setTransformationProjectionMatrix(camera.projectionMatrix()*transformation)
                .draw(_mesh);
        }

        FlatShadeObject2D& setColor(const Color3& color) {
            _color = color;
            return *this;
        }
        FlatShadeObject2D& setEnabled(bool bEnabled) {
            _bEnabled = bEnabled;
            return *this;
        }

    private:
        Shaders::Flat2D& _shader;
        Color3 _color;
        GL::Mesh& _mesh;
        bool _bEnabled = true;
};

}}

#endif
