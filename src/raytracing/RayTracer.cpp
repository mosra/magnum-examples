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

#include <Magnum/Math/Vector4.h>

#include "RayTracer.h"
#include "Camera.h"
#include "Objects.h"
#include "Materials.h"

namespace Magnum { namespace Examples {
RayTracer::RayTracer(const Vector3& eye, const Vector3& viewCenter, const Vector3& upDir,
                     Deg fov, Float aspectRatio,
                     const Vector2i& imageSize) {
    /* Set a fixed seed number for random generator,
     *   so the results should look the same each time running the program */
    srand(0);
    setViewParameters(eye, viewCenter, upDir, fov, aspectRatio);
    resizeBuffers(imageSize);
    generateScene();
}

void RayTracer::setViewParameters(const Vector3& eye, const Vector3& viewCenter, const Vector3& upDir,
                                  Deg fov, Float aspectRatio) {
    while(_busy.load()) {}
    _busy.store(true);
    _camera.reset(new Camera(eye, viewCenter, upDir, fov, aspectRatio, CameraAperture));
    clearBuffers(); /* clear buffer as camera has changed */
    _busy.store(false);
}

void RayTracer::resizeBuffers(const Vector2i& imageSize) {
    /* Should not touch the buffers while rendering */
    while(_busy.load()) {}
    _busy.store(true);
    _imageSize = imageSize;
    Containers::arrayResize(_pixels, imageSize.x() * imageSize.y() * 4);
    Containers::arrayResize(_buffer, imageSize.x() * imageSize.y());
    _numBlocks = Vector2i((imageSize.x() + BlockSize - 1) / BlockSize,
                          (imageSize.y() + BlockSize - 1) / BlockSize);
    clearBuffers();
    _busy.store(false);
}

void RayTracer::clearBuffers() {
    _currentBlock   = Vector2i(0, _numBlocks.y() - 1);
    _blockMovingDir = 1;
    for(std::size_t i = 0; i < _buffer.size(); ++i) {
        _buffer[i] = Vector4(0);
    }
    _maxSamplesPerPixels = 0;
}

void RayTracer::renderBlock() {
    if(_maxSamplesPerPixels >= MaxSamplesPerPixel) {
        return;
    }

    /* Wait if there's any buffer is currently resized */
    while(_busy.load()) {}
    _busy.store(true);

    /* Render the current block */
    Vector2i blockStart = _currentBlock * BlockSize;
    loopBlock(blockStart, [&](Int x, Int y) {
                  const Float u       = (x + Rnd::rand01()) / static_cast<Float>(_imageSize.x());
                  const Float v       = (y + Rnd::rand01()) / static_cast<Float>(_imageSize.y());
                  const Ray r         = _camera->getRay(u, v);
                  const Vector3 color = shade(r, *_sceneObjects, 0);

                  /* Accumulate into the render buffer */
                  const Int pixelIdx    = (y * _imageSize.x() + x);
                  _buffer[pixelIdx][0] += color[0];
                  _buffer[pixelIdx][1] += color[1];
                  _buffer[pixelIdx][2] += color[2];
                  _buffer[pixelIdx][3] += 1.0f;

                  /* Update the pixel buffer */
                  const Float nSamples = _buffer[pixelIdx][3];
                  if(nSamples > 0) {
                      _pixels[pixelIdx * 4 + 0] = static_cast<UnsignedByte>(255.99f * (std::sqrt(_buffer[pixelIdx][0] / nSamples)));
                      _pixels[pixelIdx * 4 + 1] = static_cast<UnsignedByte>(255.99f * (std::sqrt(_buffer[pixelIdx][1] / nSamples)));
                      _pixels[pixelIdx * 4 + 2] = static_cast<UnsignedByte>(255.99f * (std::sqrt(_buffer[pixelIdx][2] / nSamples)));
                      _pixels[pixelIdx * 4 + 3] = 255u;
                  }
              });
    _currentBlock = getNextBlock(_currentBlock);

    /* Mark out the next block to display */
    if(_bMarkNextBlock && _maxSamplesPerPixels < MaxSamplesPerPixel) {
        blockStart = _currentBlock * BlockSize;
        loopBlock(blockStart, [&](Int x, Int y) {
                      const Int pixelIdx        = (y * _imageSize.x() + x);
                      _pixels[pixelIdx * 4 + 0] = 100u;
                      _pixels[pixelIdx * 4 + 1] = 100u;
                      _pixels[pixelIdx * 4 + 2] = 100u;
                      _pixels[pixelIdx * 4 + 2] = 255u;
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
        ++_maxSamplesPerPixels;
    }
    return nextBlock;
}

Vector3 RayTracer::shade(const Ray& r, ObjectList& world, Int depth) const {
    HitInfo hitInfo;
    if(world.intersect(r, 0.001f, 1e10f, hitInfo)) {
        Ray     scatteredRay;
        Vector3 attenuation;
        if(depth < MaxRayDepth && hitInfo.material->scatter(r, hitInfo, attenuation, scatteredRay)) {
            return attenuation * shade(scatteredRay, world, depth + 1);
        } else {
            return Vector3(0, 0, 0);
        }
    }
    const Vector3 dir = r.direction.normalized();
    Float         t   = 0.5f * (dir.y() + 1.0f);
    return (1.0f - t) * SkyColor + t * FloorColor;
}

void RayTracer::generateScene() {
    _sceneObjects.reset(new ObjectList);
    _sceneObjects->addObject(new Sphere(Vector3{ 0, -1000, 0 }, 1000, new Lambertian(Vector3{ 0.5f, 0.5f, 0.5f })));

    for(Int a = -10; a <= 10; a++) {
        for(Int b = -10; b <= 10; b++) {
            const Vector3 center(a + 0.9f * Rnd::rand01(), 0.2f, b + 0.9f * Rnd::rand01());
            const Float   selectMat = Rnd::rand01();
            if((center - Vector3(4.f, 0.2f, 0.f)).length() > 0.9f) {
                if(selectMat < 0.8f) { // diffuse
                    _sceneObjects->addObject(new Sphere(
                                                 center, 0.2f,
                                                 new Lambertian(Vector3(Rnd::rand01() * Rnd::rand01(),
                                                                        Rnd::rand01() * Rnd::rand01(),
                                                                        Rnd::rand01() * Rnd::rand01())
                                                                )
                                                 )
                                             );
                } else if(selectMat < 0.95f) { // metal
                    _sceneObjects->addObject(new Sphere(
                                                 center, 0.2f,
                                                 new Metal(Vector3(0.5f * (1.f + Rnd::rand01()),
                                                                   0.5f * (1.f + Rnd::rand01()),
                                                                   0.5f * (1.f + Rnd::rand01())),
                                                           0.5f * Rnd::rand01()
                                                           )
                                                 )
                                             );
                } else { // dielectric
                    _sceneObjects->addObject(new Sphere(center, 0.2f, new Dielectric(1.f + 3.f * Rnd::rand01())));
                }
            }
        }
    }

    _sceneObjects->addObject(new Sphere(Vector3(0.f, 1.f, 0.f), 1.f, new Dielectric(1.5f)));
    _sceneObjects->addObject(new Sphere(Vector3(-4.f, 1.f, 0.f), 1.f, new Lambertian(Vector3(0.4f, 0.2f, 0.1f))));
    _sceneObjects->addObject(new Sphere(Vector3(4.f, 1.f, 0.f), 1.f, new Metal(Vector3(0.7f, 0.6f, 0.5f), 0.0f)));
}
} }
