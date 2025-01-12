/*
   This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>
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
#include <Magnum/Math/Packing.h>
#include <Magnum/Math/Vector4.h>

#include "RayTracer.h"
#include "Camera.h"
#include "Objects.h"
#include "Materials.h"

namespace Magnum { namespace Examples {

namespace  {

constexpr Vector3 BackgroundColor1{1.0f, 1.0f, 1.0f};
constexpr Vector3 BackgroundColor2{0.5f, 0.7f, 1.0f};

constexpr bool ConsistentScene = false;

/* Perform operation on the entire block of pixels */
template<class Function> void loopBlock(UnsignedInt blockSize,
    const Vector2i& blockStart, const Vector2i& imageSize, Function&& func)
{
    for(Int y = blockStart.y(), yend = blockStart.y() + blockSize; y < yend; ++y) {
        for(Int x = blockStart.x(), xend = blockStart.x() + blockSize; x < xend; ++x) {
            if(x < 0 || y < 0 || x >= imageSize.x() || y >= imageSize.y())
                continue;

            func(x, y);
        }
    }
}

/* Shade the objects */
inline Vector3 shade(UnsignedInt maxRayDepth, const Ray& r, ObjectList& objects, UnsignedInt depth) {
    HitInfo hitInfo;
    if(objects.intersect(r, 0.001f, 1e10f, hitInfo)) {
        Ray scatteredRay;
        Vector3 attenuation{0.0f, 0.0f, 0.0f};
        if(depth < maxRayDepth &&
            hitInfo.material->scatter(r, hitInfo, attenuation, scatteredRay))
        {
            attenuation *= shade(maxRayDepth, scatteredRay, objects, depth + 1);
        }

        return attenuation;
    }

    const Float t = 0.5f*(r.unitDirection.y() + 1.0f);
    return (1.0f - t)*BackgroundColor1 + t*BackgroundColor2;
}

}

RayTracer::RayTracer(const Vector3& eye, const Vector3& viewCenter,
    const Vector3& upDir, Deg fov, Float aspectRatio,  Float lensRadius,
    const Vector2i& imageSize, UnsignedInt blockSize,
    UnsignedInt maxSamplesPerPixel, UnsignedInt maxRayDepth):
    _blockSize{blockSize}, _maxSamplesPerPixel{maxSamplesPerPixel},
    _maxRayDepth{maxRayDepth}
{
    /* If ConsistentScene == true, then set a fixed seed number for random
       generator, so the render image will look the same each time running the
       program */
    std::srand(ConsistentScene ? 0 : std::time(nullptr));

    setViewParameters(eye, viewCenter, upDir, fov, aspectRatio, lensRadius);
    resizeBuffers(imageSize);
    generateSceneObjects();
}

RayTracer::~RayTracer() = default;

void RayTracer::setViewParameters(const Vector3& eye,
    const Vector3& viewCenter, const Vector3& upDir, Deg fov,
    Float aspectRatio, Float lensRadius)
{
    while(_busy.load()) {}

    _busy.store(true);
    _camera.emplace(eye, viewCenter, upDir, fov, aspectRatio, lensRadius);
    clearBuffers(); /* clear buffer as camera has changed */
    _busy.store(false);
}

void RayTracer::resizeBuffers(const Vector2i& imageSize) {
    /* Should not touch the buffers while rendering */
    while(_busy.load()) {}

    _busy.store(true);
    _imageSize = imageSize;
    arrayResize(_pixels, imageSize.product());
    arrayResize(_buffer, imageSize.product());
    _numBlocks = (imageSize + Vector2i{Int(_blockSize - 1)})/_blockSize;
    clearBuffers();
    _busy.store(false);
}

void RayTracer::clearBuffers() {
    _currentBlock = Vector2i(0, _numBlocks.y() - 1);
    _blockMovingDir = 1;
    for(std::size_t i = 0; i != _buffer.size(); ++i)
        _buffer[i] = Vector4{0.0f};

    _numRenderPass = 0;
}

void RayTracer::renderBlock() {
    if(_numRenderPass >= _maxSamplesPerPixel) return;

    /* Wait if there's any buffer is currently resized */
    while(_busy.load()) {}
    _busy.store(true);

    /* Render the current block */
    Vector2i blockStart = _currentBlock*_blockSize;
    loopBlock(_blockSize, blockStart, _imageSize, [&](Int x, Int y) {
        const Float u = (x + Rnd::rand01())/Float(_imageSize.x());
        const Float v = (y + Rnd::rand01())/Float(_imageSize.y());
        const Ray r = _camera->ray(u, v);
        const Vector3 color = shade(_maxRayDepth, r, *_sceneObjects, 0);
        const Int pixelIdx  = y*_imageSize.x() + x;
        Color4& pixelColor = _buffer[pixelIdx];

        /* Accumulate into the render buffer */
        pixelColor += Color4{color, 1.0f};
        /* Update the pixel buffer */
        _pixels[pixelIdx] = {
            Math::pack<Color3ub>(Math::sqrt(pixelColor.rgb()/pixelColor.a())),
            UnsignedByte(255)
        };
    });

    /* Mark out the next block to display */
    _currentBlock = nextBlock(_currentBlock);
    if(_markNextBlock && _numRenderPass < _maxSamplesPerPixel) {
        blockStart = _currentBlock*_blockSize;
        loopBlock(_blockSize, blockStart, _imageSize, [&](Int x, Int y) {
            const Int pixelIdx = y*_imageSize.x() + x;
            _pixels[pixelIdx] = {100u, 100u, 255u, 255u};
        });
    }

    _busy.store(false);
}

Vector2i RayTracer::nextBlock(const Vector2i& currentBlock) {
    Vector2i nextBlock = currentBlock;
    nextBlock.x() += _blockMovingDir;

    if(nextBlock.x() == _numBlocks.x() || nextBlock.x() == -1) {
        nextBlock.x() = Math::clamp(nextBlock.x(), 0, _numBlocks.x() - 1);
        nextBlock.y() = currentBlock.y() - 1;
        _blockMovingDir = -_blockMovingDir;
    }

    if(nextBlock.y() == -1) {
        nextBlock.x() = 0;
        nextBlock.y() = _numBlocks.y() - 1;
        _blockMovingDir = 1;
        ++_numRenderPass;
    }

    return nextBlock;
}

void RayTracer::generateSceneObjects() {
    _sceneObjects.emplace();

    /* Big sphere as floor */
    _sceneObjects->addObject(Containers::pointer<Sphere>(
        Vector3{0.0f, -1000.0f, 0.0f}, 1000.0f,
        Containers::pointer<Lambertian>(Vector3{0.5f, 0.5f, 0.5f})));

    const Vector3 centerBigSphere1{0.0f, 1.0f, 0.0f};
    const Vector3 centerBigSphere2{-4.0f, 1.0f, 0.0f};
    const Vector3 centerBigSphere3{4.0f, 1.0f, 0.0f};
    for(Int a = -10; a <= 10; a++) {
        for(Int b = -10; b <= 10; b++) {
            const Float radius = 0.2f + (2.0f*Rnd::rand01() - 1)*0.05f;
            const Vector3 center(a + 0.9f*Rnd::rand01(), radius, b + 0.9f*Rnd::rand01());
            if((center - centerBigSphere1).length() > 1.0f + radius &&
               (center - centerBigSphere2).length() > 1.0f + radius &&
               (center - centerBigSphere3).length() > 1.0f + radius)
            {
                Containers::Pointer<Material> material;
                const Float selectMat = Rnd::rand01();

                /* Diffuse */
                if(selectMat < 0.7f)
                    material = Containers::pointer<Lambertian>(Vector3{
                        Rnd::rand01()*Rnd::rand01(),
                        Rnd::rand01()*Rnd::rand01(),
                        Rnd::rand01()*Rnd::rand01()});
                /* \m/ */
                else if(selectMat < 0.9f)
                    material = Containers::pointer<Metal>(Vector3{
                        0.5f*(1.0f + Rnd::rand01()),
                        0.5f*(1.0f + Rnd::rand01()),
                        0.5f*(1.0f + Rnd::rand01())}, 0.5f*Rnd::rand01());
                /* Dielectric */
                else material = Containers::pointer<Dielectric>(
                    1.1f + 3.0f*Rnd::rand01());

                _sceneObjects->addObject(Containers::pointer<Sphere>(
                    center, radius, Utility::move(material)));
            }
        }
    }

    _sceneObjects->addObject(Containers::pointer<Sphere>(centerBigSphere1,
        1.0f, Containers::pointer<Dielectric>(1.5f)));
    _sceneObjects->addObject(Containers::pointer<Sphere>(centerBigSphere2,
        1.0f, Containers::pointer<Lambertian>(Vector3{
            Rnd::rand01()*Rnd::rand01(),
            Rnd::rand01()*Rnd::rand01(),
            Rnd::rand01()*Rnd::rand01()})));
    _sceneObjects->addObject(Containers::pointer<Sphere>(centerBigSphere3,
        1.0f, Containers::pointer<Metal>(Vector3{
            0.5f*(1.0f + Rnd::rand01()),
            0.5f*(1.0f + Rnd::rand01()),
            0.5f*(1.0f + Rnd::rand01())}, 0.0f)));
}

}}
