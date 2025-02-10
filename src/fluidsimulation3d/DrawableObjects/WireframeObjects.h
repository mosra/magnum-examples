#ifndef Magnum_Examples_FluidSimulation3D_DrawableObjects_WireframeObjects_h
#define Magnum_Examples_FluidSimulation3D_DrawableObjects_WireframeObjects_h
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
#include <Magnum/MeshTools/Compile.h>
#include <Magnum/Primitives/Cube.h>
#include <Magnum/Primitives/Grid.h>
#include <Magnum/Shaders/FlatGL.h>
#include <Magnum/SceneGraph/Scene.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/Trade/MeshData.h>

#include "DrawableObjects/FlatShadeObject.h"

namespace Magnum { namespace Examples {

using Object3D = SceneGraph::Object<SceneGraph::MatrixTransformation3D>;
using Scene3D  = SceneGraph::Scene<SceneGraph::MatrixTransformation3D>;

class WireframeObject {
    public:
        explicit WireframeObject(Scene3D* const scene, SceneGraph::DrawableGroup3D* const drawableGroup) {
            _obj3D.reset(new Object3D{scene});
            _flatShader = Shaders::FlatGL3D{};
            _drawableObj.reset(new FlatShadeObject{*_obj3D, _flatShader, Color3{0.75f}, _mesh, drawableGroup});
        }

        WireframeObject& setColor(const Color3& color) {
            _drawableObj->setColor(color);
            return *this;
        }
        WireframeObject& transform(const Matrix4& matrix) {
            _obj3D->transform(matrix);
            return *this;
        }
        WireframeObject& setTransformation(const Matrix4& matrix) {
            _obj3D->setTransformation(matrix);
            return *this;
        }

    protected:
        GL::Mesh _mesh{NoCreate};
        Shaders::FlatGL3D _flatShader{NoCreate};
        Containers::Pointer<Object3D> _obj3D;
        Containers::Pointer<FlatShadeObject> _drawableObj;
};

class WireframeBox: public WireframeObject {
    public:
        explicit WireframeBox(Scene3D* const scene, SceneGraph::DrawableGroup3D* const drawableGroup): WireframeObject{scene, drawableGroup} {
            _mesh = MeshTools::compile(Primitives::cubeWireframe());
        }
};

class WireframeGrid: public WireframeObject {
    public:
        explicit WireframeGrid(Scene3D* const scene, SceneGraph::DrawableGroup3D* const drawableGroup): WireframeObject{scene, drawableGroup} {
            using namespace Magnum::Math::Literals;

            _mesh = MeshTools::compile(Primitives::grid3DWireframe({ 20, 20 }));
            _obj3D->scale(Vector3(10.0f));
            _obj3D->rotateX(90.0_degf);
    }
};

}}

#endif
