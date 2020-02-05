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

#include <ctime>

#include <Magnum/Math/Vector4.h>

#include "RayTracer.h"
#include "Camera.h"
#include "Objects.h"
#include "Materials.h"

namespace Magnum { namespace Examples {
namespace  {
constexpr Int BlockSize          = 64;
constexpr Int MaxSamplesPerPixel = 512;
constexpr Int MaxRayDepth        = 16;

const Vector3 BackgroundColor1 = Vector3{ 1.0f, 1.0f, 1.0f };
const Vector3 BackgroundColor2 = Vector3{ 0.5f, 0.7f, 1.0f };

constexpr bool bConsistentScene = false;

/* Perform operation on the entire block of pixels */
template<class Function>
void loopBlock(const Vector2i& blockStart, const Vector2i& imageSize, Function&& func) {
    for(Int y = blockStart.y(), yend = blockStart.y() + BlockSize; y < yend; ++y) {
        for(Int x = blockStart.x(), xend = blockStart.x() + BlockSize; x < xend; ++x) {
            if(x < 0
               || y < 0
               || x >= imageSize.x()
               || y >= imageSize.y()) {
                continue;
            }
            func(x, y);
        }
    }
}

/* Shade the objects */
inline Vector3 shade(const Ray& r, ObjectList& objects, Int depth) {
    HitInfo hitInfo;
    if(objects.intersect(r, 0.001f, 1e10f, hitInfo)) {
        Ray     scatteredRay;
        Vector3 attenuation(0, 0, 0);
        if(depth < MaxRayDepth && hitInfo.material->scatter(r, hitInfo, attenuation, scatteredRay)) {
            attenuation *= shade(scatteredRay, objects, depth + 1);
        }
        return attenuation;
    }
    const Vector3 dir = r.direction.normalized();
    Float         t   = 0.5f * (dir.y() + 1.0f);
    return (1.0f - t) * BackgroundColor1 + t * BackgroundColor2;
}
}

RayTracer::RayTracer(const Vector3& eye, const Vector3& viewCenter, const Vector3& upDir,
                     Deg fov, Float aspectRatio,  Float lensRadius,
                     const Vector2i& imageSize) {
    /* If bConsistentScene == true, then set a fixed seed number for random generator,
     *   so the render image will look the same each time running the program */
    srand(bConsistentScene ? 0 : time(nullptr));
    setViewParameters(eye, viewCenter, upDir, fov, aspectRatio, lensRadius);
    resizeBuffers(imageSize);
    generateSceneObjects();
}

void RayTracer::setViewParameters(const Vector3& eye, const Vector3& viewCenter, const Vector3& upDir,
                                  Deg fov, Float aspectRatio, Float lensRadius) {
    while(_busy.load()) {}
    _busy.store(true);
    _camera.reset(new Camera(eye, viewCenter, upDir, fov, aspectRatio, lensRadius));
    clearBuffers(); /* clear buffer as camera has changed */
    _busy.store(false);
}

void RayTracer::resizeBuffers(const Vector2i& imageSize) {
    /* Should not touch the buffers while rendering */
    while(_busy.load()) {}
    _busy.store(true);
    _imageSize = imageSize;
    Containers::arrayResize(_pixels, imageSize.x() * imageSize.y());
    Containers::arrayResize(_buffer, imageSize.x() * imageSize.y());
    _numBlocks = (imageSize + Vector2i{ BlockSize - 1 }) / BlockSize;
    clearBuffers();
    _busy.store(false);
}

void RayTracer::clearBuffers() {
    _currentBlock   = Vector2i(0, _numBlocks.y() - 1);
    _blockMovingDir = 1;
    for(std::size_t i = 0; i < _buffer.size(); ++i) {
        _buffer[i] = Vector4(0);
    }
    _numRenderPass = 0;
}

void RayTracer::renderBlock() {
    if(_numRenderPass >= MaxSamplesPerPixel) {
        return;
    }

    /* Wait if there's any buffer is currently resized */
    while(_busy.load()) {}
    _busy.store(true);

    /* Render the current block */
    Vector2i blockStart = _currentBlock * BlockSize;
    loopBlock(blockStart, _imageSize, [&](Int x, Int y) {
                  const Float u       = (x + Rnd::rand01()) / static_cast<Float>(_imageSize.x());
                  const Float v       = (y + Rnd::rand01()) / static_cast<Float>(_imageSize.y());
                  const Ray r         = _camera->getRay(u, v);
                  const Vector3 color = shade(r, *_sceneObjects, 0);
                  const Int pixelIdx  = y * _imageSize.x() + x;
                  Vector4& pixelColor = _buffer[pixelIdx];

                  /* Accumulate into the render buffer */
                  pixelColor += Vector4{ color, 1.0f };

                  /* Update the pixel buffer */
                  _pixels[pixelIdx].xyz() = Math::Vector3<UnsignedByte>{ 255.99f * Math::sqrt(pixelColor.xyz() / pixelColor.w()) };
                  _pixels[pixelIdx].w()   = 255u;
              });
    _currentBlock = getNextBlock(_currentBlock);

    /* Mark out the next block to display */
    if(_bMarkNextBlock && _numRenderPass < MaxSamplesPerPixel) {
        blockStart = _currentBlock * BlockSize;
        loopBlock(blockStart, _imageSize, [&](Int x, Int y) {
                      const Int pixelIdx = y * _imageSize.x() + x;
                      _pixels[pixelIdx]  = { 100u, 100u, 255u, 255u };
                  });
    }

    _busy.store(false);
}

Vector2i RayTracer::getNextBlock(const Vector2i& currentBlock) {
    Vector2i nextBlock = currentBlock;
    nextBlock.x() += _blockMovingDir;
    if(nextBlock.x() == _numBlocks.x() || nextBlock.x() == -1) {
        nextBlock.x()   = Math::clamp(nextBlock.x(), 0, _numBlocks.x() - 1);
        nextBlock.y()   = currentBlock.y() - 1;
        _blockMovingDir = -_blockMovingDir;
    }
    if(nextBlock.y() == -1) {
        nextBlock.x() = 0;
        nextBlock.y()   = _numBlocks.y() - 1;
        _blockMovingDir = 1;
        ++_numRenderPass;
    }
    return nextBlock;
}

void RayTracer::generateSceneObjects() {
    _sceneObjects.reset(new ObjectList);

    /* Big sphere as floor */
    _sceneObjects->addObject(new Sphere(Vector3{ 0, -1000, 0 }, 1000, new Lambertian(Vector3{ 0.5f, 0.5f, 0.5f })));
    const Vector3 centerBigSphere1{ 0, 1, 0 };
    const Vector3 centerBigSphere2{ -4, 1, 0 };
    const Vector3 centerBigSphere3{ 4, 1, 0 };

    for(Int a = -10; a <= 10; a++) {
        for(Int b = -10; b <= 10; b++) {
            const Float   radius = 0.2f + (2 * Rnd::rand01() - 1) * 0.05f;
            const Vector3 center(a + 0.9f * Rnd::rand01(), radius, b + 0.9f * Rnd::rand01());
            if((center - centerBigSphere1).length() > 1 + radius
               && (center - centerBigSphere2).length() > 1 + radius
               && (center - centerBigSphere3).length() > 1 + radius) {
                Material*   material;
                const Float selectMat = Rnd::rand01();
                if(selectMat < 0.7f) { // diffuse
                    material = new Lambertian(Vector3(Rnd::rand01() * Rnd::rand01(),
                                                      Rnd::rand01() * Rnd::rand01(),
                                                      Rnd::rand01() * Rnd::rand01())
                                              );
                } else if(selectMat < 0.9f) { // metal
                    material = new Metal(Vector3(0.5f * (1.f + Rnd::rand01()),
                                                 0.5f * (1.f + Rnd::rand01()),
                                                 0.5f * (1.f + Rnd::rand01())),
                                         0.5f * Rnd::rand01());
                } else { // dielectric
                    material = new Dielectric(1.1f + 3.f * Rnd::rand01());
                }
                _sceneObjects->addObject(new Sphere(center, radius, material));
            }
        }
    }

    _sceneObjects->addObject(new Sphere(centerBigSphere1, 1.f, new Dielectric(1.5f)));
    _sceneObjects->addObject(new Sphere(centerBigSphere2, 1.f, new Lambertian(Vector3(Rnd::rand01() * Rnd::rand01(),
                                                                                      Rnd::rand01() * Rnd::rand01(),
                                                                                      Rnd::rand01() * Rnd::rand01())
                                                                              )));
    _sceneObjects->addObject(new Sphere(centerBigSphere3, 1.f, new Metal(Vector3(0.5f * (1.f + Rnd::rand01()),
                                                                                 0.5f * (1.f + Rnd::rand01()),
                                                                                 0.5f * (1.f + Rnd::rand01())),
                                                                         0
                                                                         )));
}
} }
