#ifndef Magnum_Examples_ArcBallCamera_h
#define Magnum_Examples_ArcBallCamera_h

/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019 —
            Vladimír Vondruš <mosra@centrum.cz>
        2020 — Nghia Truong <nghiatruong.vn@gmail.com>

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

#include "ArcBall.h"

#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Object.h>
#include <Magnum/SceneGraph/Scene.h>

namespace Magnum {
using Object3D = SceneGraph::Object<SceneGraph::MatrixTransformation3D>;
using Scene3D  = SceneGraph::Scene<SceneGraph::MatrixTransformation3D>;

/*
 * SceneGraph arcball camera implementation
 */
class ArcBallCamera : public ArcBall {
public:
    ArcBallCamera(Scene3D* scene, const Vector3& cameraPosition, const Vector3& viewCenter, const Vector3& upDir,
                  Deg fov, Vector2i windowSize, Vector2i viewportSize) :
        ArcBall(cameraPosition, viewCenter, upDir, fov, windowSize) {
        _cameraObject = new Object3D{ scene };
        _cameraObject->setTransformation(transformation()); /* initialize the camera position */
        _camera = new SceneGraph::Camera3D{ *_cameraObject };
        _camera->setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::Extend)
            .setProjectionMatrix(Matrix4::perspectiveProjection(
                                     fov, Vector2{ windowSize }.aspectRatio(), 0.01f, 100.0f))
            .setViewport(viewportSize);
    }

    /* Update screen size and viewport size after the window has been resized */
    void reshape(const Vector2i& windowSize, const Vector2i& viewportSize) {
        _windowSize = windowSize;
        _camera->setViewport(viewportSize);
    }

    /* Update the scenegraph camera if arcball has been changed */
    bool update() { return ArcBall::update(*_cameraObject); }

    /* Draw objects using the internal scenegraph camera */
    void draw(SceneGraph::DrawableGroup3D& drawables) {
        _camera->draw(drawables);
    }

private:
    Object3D*             _cameraObject { nullptr };
    SceneGraph::Camera3D* _camera { nullptr };
};
}
#endif
