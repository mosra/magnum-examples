#ifndef Magnum_Examples_ArcBall_h
#define Magnum_Examples_ArcBall_h

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

#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector2.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Matrix4.h>
#include <Magnum/Math/Quaternion.h>

namespace Magnum { namespace Examples {
/*
 * Implementation of Ken Shoemake's arcball camera with smooth navigation feature
 * https://www.talisman.org/~erlkonig/misc/shoemake92-arcball.pdf
 */
class ArcBall {
public:
    ArcBall(const Vector3& cameraPosition, const Vector3& viewCenter, const Vector3& upDir,
            Deg fov, Vector2i windowSize);

    /* Set the camera view parameters: eye position, view center, up direction */
    void setViewParameters(const Vector3& eye, const Vector3& viewCenter, const Vector3& upDir);

    /* Reset the camera to its initial position, view center, and up dir */
    void reset();

    /* Update screen size after the window has been resized */
    void reshape(const Vector2i& windowSize) { _windowSize = windowSize; }

    /* Update any unfinished transformation due to lagging, return true if the camera matrices have changed */
    bool updateTransformation();

    /* Get/Set the amount of lagging such that the camera will (slowly) smoothly navigate
     * Lagging must be in [0, 1) */
    Float lagging() const { return _lagging; }
    void setLagging(Float lagging);

    /* Initialize the first (screen) mouse position for camera transformation.
     * This should be called in mouse pressed event */
    void initTransformation(const Vector2i& mousePos);

    /* Rotate the camera from the previous (screen) mouse position to the current (screen) position */
    void rotate(const Vector2i& mousePos);

    /* Translate the camera from the previous (screen) mouse position to the current (screen) mouse position */
    void translate(const Vector2i& mousePos);

    /* Translate the camera by the delta amount of (NDC) mouse position
     * Note that NDC position must be in [-1, -1] to [1, 1] */
    void translateDelta(const Vector2& translationNDC);

    /* Zoom the camera (positive zoom delta = zoom in, negative delta = zoom out) */
    void zoom(Float delta);

    /* Get the camera's view matrix */
    const Matrix4& viewMatrix() const { return _viewMatrix; }

    /* Get the camera's inverse view matrix (which also produces transformation of the camera) */
    const Matrix4& inverseViewMatrix() const { return _inverseViewMatrix; }

    /* Get the camera's transformation matrix */
    const Matrix4& transformation() const { return _inverseViewMatrix; }

protected:
    /* Update the camera matrices */
    void updateMatrices();

    /* Transform from screen coordinate to NDC - normalized device coordinate
     * The top-left of the screen corresponds to [-1, 1] NDC,
     *   and the bottom right is [1, -1] NDC  */
    Vector2 screenCoordToNDC(const Vector2i& mousePos) const;

    Deg      _fov;
    Vector2i _windowSize;

    Vector2 _prevMousePosNDC;
    Float   _lagging { 0 };

    Vector3    _targetPosition, _currentPosition, _positionT0;
    Quaternion _targetQRotation, _currentQRotation, _qRotationT0;
    Float      _targetZooming, _currentZooming, _zoomingT0;
    Matrix4    _viewMatrix, _inverseViewMatrix;
};
} }
#endif
