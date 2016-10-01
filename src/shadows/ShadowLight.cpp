/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016 —
            Vladimír Vondruš <mosra@centrum.cz>
        2016 — Bill Robinson <airbaggins@gmail.com>

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

#include "ShadowLight.h"

#include <algorithm>
#include <Magnum/DefaultFramebuffer.h>
#include <Magnum/ImageView.h>
#include <Magnum/PixelFormat.h>
#include <Magnum/Renderer.h>
#include <Magnum/TextureArray.h>
#include <Magnum/TextureFormat.h>
#include <Magnum/SceneGraph/FeatureGroup.h>
#include <Magnum/SceneGraph/MatrixTransformation3D.h>
#include <Magnum/SceneGraph/Scene.h>

#include "ShadowCasterDrawable.h"

namespace Magnum { namespace Examples {

ShadowLight::ShadowLight(SceneGraph::Object<SceneGraph::MatrixTransformation3D>& parent): SceneGraph::Camera3D{parent}, _object{parent}, _shadowTexture{} {
    setAspectRatioPolicy(SceneGraph::AspectRatioPolicy::NotPreserved);
}

void ShadowLight::setupShadowmaps(Int numShadowLevels, const Vector2i& size) {
    _layers.clear();

    (*(_shadowTexture = new Texture2DArray()))
        .setLabel("Shadow texture")
        .setImage(0, TextureFormat::DepthComponent, ImageView3D{PixelFormat::DepthComponent, PixelType::Float, {size, numShadowLevels}, nullptr})
        .setMaxLevel(0)
        .setCompareFunction(Sampler::CompareFunction::LessOrEqual)
        .setCompareMode(Sampler::CompareMode::CompareRefToTexture)
        .setMinificationFilter(Sampler::Filter::Linear, Sampler::Mipmap::Base)
        .setMagnificationFilter(Sampler::Filter::Linear);

    for(std::int_fast32_t i = 0; i < numShadowLevels; ++i) {
        _layers.emplace_back(size);
        Framebuffer& shadowFramebuffer = _layers.back().shadowFramebuffer;
        shadowFramebuffer.setLabel("Shadow framebuffer " + std::to_string(i))
            .attachTextureLayer(Framebuffer::BufferAttachment::Depth, *_shadowTexture, 0, i)
            .mapForDraw(Framebuffer::DrawAttachment::None)
            .bind();
        Debug() << "Framebuffer status:" << shadowFramebuffer.checkStatus(FramebufferTarget::Draw);
    }
}

ShadowLight::ShadowLayerData::ShadowLayerData(const Vector2i& size): shadowFramebuffer{{{}, size}} {}

void ShadowLight::setTarget(const Vector3& lightDirection, const Vector3& screenDirection, SceneGraph::Camera3D& mainCamera) {
    Matrix4 cameraMatrix = Matrix4::lookAt({}, -lightDirection, screenDirection);
    const Matrix3x3 cameraRotationMatrix = cameraMatrix.rotation();
    const Matrix3x3 inverseCameraRotationMatrix = cameraRotationMatrix.inverted();

    for(std::size_t layerIndex = 0; layerIndex != _layers.size(); ++layerIndex) {
        std::vector<Vector3> mainCameraFrustumCorners = layerFrustumCorners(mainCamera, Int(layerIndex));
        ShadowLayerData& layer = _layers[layerIndex];

        /* Calculate the AABB in shadow-camera space */
        Vector3 min{std::numeric_limits<Float>::max()}, max{std::numeric_limits<Float>::lowest()};
        for(Vector3 worldPoint: mainCameraFrustumCorners) {
            Vector3 cameraPoint = inverseCameraRotationMatrix*worldPoint;
            for(std::size_t i = 0; i < 3; i++) {
                if(cameraPoint[i] < min[i]) {
                    min[i] = cameraPoint[i];
                }
                if(cameraPoint[i] > max[i]) {
                    max[i] = cameraPoint[i];
                }
            }
        }

        /* Place the shadow camera at the mid-point of the camera box */
        const Vector3 mid = (min + max)*0.5f;
        const Vector3 cameraPosition = cameraRotationMatrix*mid;

        const Vector3 range = max - min;
        /* Set up the initial extends of the shadow map's render volume. Note
           we will adjust this later when we render. */
        layer.orthographicSize = range.xy();
        layer.orthographicNear = -0.5f*range.z();
        layer.orthographicFar =  0.5f*range.z();
        cameraMatrix.translation() = cameraPosition;
        layer.shadowCameraMatrix = cameraMatrix;
    }
}

Float ShadowLight::cutZ(const Int layer) const {
    return _layers[layer].cutPlane;
}

void ShadowLight::setupSplitDistances(const Float zNear, const Float zFar, const Float power) {
    /* props http://stackoverflow.com/a/33465663 */
    for(std::size_t i = 0; i != _layers.size(); ++i) {
        const Float linearDepth = zNear + std::pow(Float(i + 1)/_layers.size(), power)*(zFar - zNear);
        const Float nonLinearDepth = (zFar + zNear - 2.0f*zNear*zFar/linearDepth)/(zFar - zNear);
        _layers[i].cutPlane = (nonLinearDepth + 1.0f)/2.0f;
    }
}

Float ShadowLight::cutDistance(const Float zNear, const Float zFar, const Int layer) const {
    const Float depthSample = 2.0f*_layers[layer].cutPlane - 1.0f;
    const Float zLinear = 2.0f*zNear*zFar/(zFar + zNear - depthSample*(zFar - zNear));
    return zLinear;
}

std::vector<Vector3> ShadowLight::layerFrustumCorners(SceneGraph::Camera3D& mainCamera, const Int layer) {
    const Float z0 = layer == 0 ? 0 : _layers[layer - 1].cutPlane;
    const Float z1 = _layers[layer].cutPlane;
    return cameraFrustumCorners(mainCamera, z0, z1);
}

std::vector<Vector3> ShadowLight::cameraFrustumCorners(SceneGraph::Camera3D& mainCamera, const Float z0, const Float z1) {
    const Matrix4 imvp = (mainCamera.projectionMatrix()*mainCamera.cameraMatrix()).inverted();
    return frustumCorners(imvp, z0, z1);
}

std::vector<Vector3> ShadowLight::frustumCorners(const Matrix4& imvp, const Float z0, const Float z1) {
    auto projectImvpAndDivide = [&](Vector4 vec) -> Vector3 {
        const Vector4 vec2 = imvp*vec;
        return vec2.xyz()/vec2.w();
    };
    return {projectImvpAndDivide({-1,-1, z0, 1}),
            projectImvpAndDivide({ 1,-1, z0, 1}),
            projectImvpAndDivide({-1, 1, z0, 1}),
            projectImvpAndDivide({ 1, 1, z0, 1}),
            projectImvpAndDivide({-1,-1, z1, 1}),
            projectImvpAndDivide({ 1,-1, z1, 1}),
            projectImvpAndDivide({-1, 1, z1, 1}),
            projectImvpAndDivide({ 1, 1, z1, 1})};
}

std::vector<Vector4> ShadowLight::calculateClipPlanes() {
    const Matrix4 pm = projectionMatrix();
    std::vector<Vector4> clipPlanes{
        {pm[3][0] + pm[2][0], pm[3][1] + pm[2][1], pm[3][2] + pm[2][2], pm[3][3] + pm[2][3]},   /* near */
        {pm[3][0] - pm[2][0], pm[3][1] - pm[2][1], pm[3][2] - pm[2][2], pm[3][3] - pm[2][3]},   /* far */
        {pm[3][0] + pm[0][0], pm[3][1] + pm[0][1], pm[3][2] + pm[0][2], pm[3][3] + pm[0][3]},   /* left */
        {pm[3][0] - pm[0][0], pm[3][1] - pm[0][1], pm[3][2] - pm[0][2], pm[3][3] - pm[0][3]},   /* right */
        {pm[3][0] + pm[1][0], pm[3][1] + pm[1][1], pm[3][2] + pm[1][2], pm[3][3] + pm[1][3]},   /* bottom */
        {pm[3][0] - pm[1][0], pm[3][1] - pm[1][1], pm[3][2] - pm[1][2], pm[3][3] - pm[1][3]}};  /* top */
    for(Vector4& plane: clipPlanes)
        plane *= plane.xyz().lengthInverted();
    return clipPlanes;
}

void ShadowLight::render(SceneGraph::DrawableGroup3D& drawables) {
    /* Compute transformations of all objects in the group relative to the camera */
    std::vector<std::reference_wrapper<SceneGraph::AbstractObject3D>> objects;
    objects.reserve(drawables.size());
    for(std::size_t i = 0; i != drawables.size(); ++i)
        objects.push_back(drawables[i].object());
    std::vector<ShadowCasterDrawable*> filteredDrawables;

    /* Projecting world points normalized device coordinates means they range
       -1 -> 1. Use this bias matrix so we go straight from world -> texture
       space */
    constexpr const Matrix4 bias{{0.5f, 0.0f, 0.0f, 0.0f},
                                 {0.0f, 0.5f, 0.0f, 0.0f},
                                 {0.0f, 0.0f, 0.5f, 0.0f},
                                 {0.5f, 0.5f, 0.5f, 1.0f}};

    Renderer::setDepthMask(true);

    for(std::size_t layer = 0; layer != _layers.size(); ++layer) {
        ShadowLayerData& d = _layers[layer];
        Float orthographicNear = d.orthographicNear;
        const Float orthographicFar = d.orthographicFar;

        /* Move this whole object to the right place to render each layer */
        _object.setTransformation(d.shadowCameraMatrix)
            .setClean();
        setProjectionMatrix(Matrix4::orthographicProjection(d.orthographicSize, orthographicNear, orthographicFar));

        const std::vector<Vector4> clipPlanes = calculateClipPlanes();
        std::vector<Matrix4> transformations = _object.scene()->AbstractObject<3,Float>::transformationMatrices(objects, cameraMatrix());

        /* Rebuild the list of objects we will draw by clipping them with the
           shadow camera's planes */
        std::size_t transformationsOutIndex = 0;
        filteredDrawables.clear();
        for(std::size_t drawableIndex = 0; drawableIndex != drawables.size(); ++drawableIndex) {
            auto& drawable = static_cast<ShadowCasterDrawable&>(drawables[drawableIndex]);
            const Matrix4 transform = transformations[drawableIndex];

            /* If your centre is offset, inject it here */
            const Vector4 localCentre{0.0f, 0.0f, 0.0f, 1.0f};
            const Vector4 drawableCentre = transform*localCentre;

            /* Start at 1, not 0 to skip out the near plane because we need to
               include shadow casters traveling the direction the camera is
               facing. */
            for(std::size_t clipPlaneIndex = 1; clipPlaneIndex != clipPlanes.size(); ++clipPlaneIndex) {
                const Float distance = Math::dot(clipPlanes[clipPlaneIndex], drawableCentre);

                /* If the object is on the useless side of any one plane, we can skip it */
                if(distance < -drawable.radius())
                    goto next;
            }

            {
                /* If this object extends in front of the near plane, extend
                   the near plane. We negate the z because the negative z is
                   forward away from the camera, but the near/far planes are
                   measured forwards. */
                const Float nearestPoint = -drawableCentre.z() - drawable.radius();
                if(nearestPoint < orthographicNear)
                    orthographicNear = nearestPoint;
                filteredDrawables.push_back(&drawable);
                transformations[transformationsOutIndex++] = transform;
            }

            next:;
        }

        /* Recalculate the projection matrix with new near plane. */
        const Matrix4 shadowCameraProjectionMatrix =
            Matrix4::orthographicProjection(d.orthographicSize, orthographicNear, orthographicFar);
        d.shadowMatrix = bias*shadowCameraProjectionMatrix*cameraMatrix();
        setProjectionMatrix(shadowCameraProjectionMatrix);

        d.shadowFramebuffer.clear(FramebufferClear::Depth)
            .bind();
        for(std::size_t i = 0; i != transformationsOutIndex; ++i)
            filteredDrawables[i]->draw(transformations[i], *this);
    }

    defaultFramebuffer.bind();
}

}}
