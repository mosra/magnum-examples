#ifndef Magnum_Examples_DrawableObjects_WireframeObject2D_h
#define Magnum_Examples_DrawableObjects_WireframeObject2D_h
/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>
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

#include <Corrade/Containers/Pointer.h>
#include <Magnum/GL/Mesh.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/MatrixTransformation2D.h>
#include <Magnum/Shaders/FlatGL.h>

#include "DrawableObjects/FlatShadeObject2D.h"

namespace Magnum { namespace Examples {

using Scene2D = SceneGraph::Scene<SceneGraph::MatrixTransformation2D>;

class WireframeObject2D {
    public:
        explicit WireframeObject2D(Scene2D* const scene, SceneGraph::DrawableGroup2D* const drawableGroup, GL::Mesh&& mesh) :
            _mesh(std::move(mesh)) {
            _obj2D.reset(new Object2D{ scene });
            _flatShader = Shaders::FlatGL2D{};
            _drawableObj.reset(new FlatShadeObject2D{ *_obj2D, _flatShader, Color3{ 1.0f }, _mesh, drawableGroup });
        }

        WireframeObject2D& setColor(const Color3& color) {
            _drawableObj->setColor(color);
            return *this;
        }

        WireframeObject2D& setTransformation(const Matrix3& matrix) {
            _obj2D->setTransformation(matrix);
            return *this;
        }

        WireframeObject2D& setEnabled(bool bEnabled) {
            _drawableObj->setEnabled(bEnabled);
            return *this;
        }

    protected:
        GL::Mesh _mesh{NoCreate};
        Shaders::FlatGL2D _flatShader{NoCreate};
        Containers::Pointer<Object2D> _obj2D;
        Containers::Pointer<FlatShadeObject2D> _drawableObj;
};

}}

#endif
