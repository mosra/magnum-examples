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

#include <atomic>

namespace Magnum { namespace Examples {
struct Ray;
class ObjectList;
class Camera;

/* Parameter for ray tracing */
namespace  {
constexpr Int   BlockSize          = 64;
constexpr Int   MaxSamplesPerPixel = 512;
constexpr Int   MaxRayDepth        = 32;
constexpr Float CameraAperture     = 0.01f;

const Vector3 SkyColor   = Vector3{ 1.0f, 1.0f, 1.0f };
const Vector3 FloorColor = Vector3{ 0.5f, 0.7f, 1.0f };
}

class RayTracer {
public:
    RayTracer(const Vector3& eye, const Vector3& viewCenter, const Vector3& upDir,
              Deg fov, Float aspectRatio, const Vector2i& imageSize);

    /* Render a block in the buffer image. This should be called in every drawEvent */
    void renderBlock();

    /* Mark next render block by a different color */
    bool& markNextBlock() { return _bMarkNextBlock; }
    const bool& markNextBlock() const { return _bMarkNextBlock; }

    /* Set the camera view parameters: eye position, view center, up direction */
    void setViewParameters(const Vector3& eye, const Vector3& viewCenter, const Vector3& upDir,
                           Deg fov, Float aspectRatio);

    /* Update size of the render buffer, should be called in the viewportEvent */
    void resizeBuffers(const Vector2i& imageSize);

    /* Get the rendered image
     * This should be called after renderNextBlock() in every drawEvent */
    const Corrade::Containers::Array<UnsignedByte>& renderedBuffer() const { return _pixels; }

    /* Return number of render pass */
    Int maxSamplesPerPixels() const { return _maxSamplesPerPixels; }

private:
    /* Clear data in the render buffer */
    void clearBuffers();

    /* Identify next block to render */
    Vector2i getNextBlock(const Vector2i& currentBlock);

    /* Perform operation on the entire block of pixels */
    template<class Function>
    void loopBlock(const Vector2i& blockStart, Function&& func) {
        for(Int y = blockStart.y(), yend = blockStart.y() + BlockSize; y < yend; ++y) {
            for(Int x = blockStart.x(), xend = blockStart.x() + BlockSize; x < xend; ++x) {
                if(x < 0
                   || y < 0
                   || x >= _imageSize.x()
                   || y >= _imageSize.y()) {
                    continue;
                }
                func(x, y);
            }
        }
    }

    /* Shade the objects */
    Vector3 shade(const Ray& r, ObjectList& world, Int depth) const;

    /* Generate objects in the scene */
    void generateScene();

    Corrade::Containers::Pointer<Camera>     _camera;
    Corrade::Containers::Pointer<ObjectList> _sceneObjects;
    Corrade::Containers::Array<UnsignedByte> _pixels;
    Corrade::Containers::Array<Vector4>      _buffer;

    Vector2i _imageSize;
    Vector2i _numBlocks;
    Vector2i _currentBlock;
    Int      _blockMovingDir { 1 };
    Int      _maxSamplesPerPixels { 0 };

    bool              _bMarkNextBlock { true };
    std::atomic<bool> _busy { false };
};
} }
#endif
