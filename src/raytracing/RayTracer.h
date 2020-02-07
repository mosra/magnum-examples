#ifndef Magnum_Examples_RayTracing_RayTracer_h
#define Magnum_Examples_RayTracing_RayTracer_h

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

#include <Corrade/Containers/Array.h>
#include <Corrade/Containers/GrowableArray.h>
#include <Corrade/Containers/Pointer.h>
#include <Magnum/Magnum.h>
#include <Magnum/Math/Vector3.h>
#include <Magnum/Math/Color.h>

#include <atomic>

namespace Magnum { namespace Examples {
struct Ray;
class ObjectList;
class Camera;

class RayTracer {
public:
    RayTracer(const Vector3& eye, const Vector3& viewCenter, const Vector3& upDir,
              Deg fov, Float aspectRatio, Float lensRadius, const Vector2i& imageSize);

    /* Render a block in the buffer image. This should be called in every drawEvent */
    void renderBlock();

    /* Toggle marking next render block by a different color */
    bool& markNextBlock() { return _bMarkNextBlock; }
    const bool& markNextBlock() const { return _bMarkNextBlock; }

    /* Set the camera view parameters */
    void setViewParameters(const Vector3& eye, const Vector3& viewCenter, const Vector3& upDir,
                           Deg fov, Float aspectRatio, Float lensRadius);

    /* Update size of the render buffer, should be called in the viewportEvent */
    void resizeBuffers(const Vector2i& imageSize);

    /* Clear the render buffer data */
    void clearBuffers();

    /* Generate scene, will produce a new, different scene if bConsistentScene is false */
    void generateSceneObjects();

    /* Get the rendered image
     * This should be called after renderBlock() in every drawEvent */
    const Corrade::Containers::Array<Color4ub>& renderedBuffer() const { return _pixels; }

private:
    /* Identify the next pixel block to render */
    Vector2i getNextBlock(const Vector2i& currentBlock);

    Containers::Pointer<Camera>     _camera;
    Containers::Pointer<ObjectList> _sceneObjects;
    Containers::Array<Color4ub>     _pixels;
    Containers::Array<Vector4>      _buffer;

    Vector2i _imageSize;
    Vector2i _numBlocks;
    Vector2i _currentBlock;
    Int      _blockMovingDir { 1 };
    Int      _numRenderPass { 0 };

    bool              _bMarkNextBlock { true };
    std::atomic<bool> _busy { false };
};
} }
#endif
