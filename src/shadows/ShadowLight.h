#ifndef Magnum_Examples_Shadows_ShadowLight_h
#define Magnum_Examples_Shadows_ShadowLight_h
/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>
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

#include <Magnum/Resource.h>
#include <Magnum/GL/Framebuffer.h>
#include <Magnum/GL/TextureArray.h>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/SceneGraph/Drawable.h>
#include <Magnum/SceneGraph/AbstractFeature.h>

#include "Types.h"

namespace Magnum { namespace Examples {

/**
@brief A special camera used to render shadow maps

The object it's attached to should face the direction that the light travels.
*/
class ShadowLight: public SceneGraph::Camera3D {
    public:
        static std::vector<Vector3> cameraFrustumCorners(SceneGraph::Camera3D& mainCamera, Float z0 = -1.0f, Float z1 = 1.0f);

        static std::vector<Vector3> frustumCorners(const Matrix4& imvp, Float z0, Float z1);

        explicit ShadowLight(SceneGraph::Object<SceneGraph::MatrixTransformation3D>& parent);

        /**
         * @brief Initialize the shadow map texture array and framebuffers
         *
         * Should be called before @ref setupSplitDistances().
         */
        void setupShadowmaps(Int numShadowLevels, const Vector2i& size);

        /**
         * @brief Set up the distances we should cut the view frustum along
         *
         * The distances will be distributed along a power series. Should be
         * called after @ref setupShadowmaps().
         */
        void setupSplitDistances(Float cameraNear, Float cameraFar, Float power);

        /**
         * @brief Computes all the matrices for the shadow map splits
         * @param lightDirection    Direction of travel of the light
         * @param screenDirection   Crossed with light direction to determine
         *      orientation of the shadow maps. Use the forward direction of
         *      the camera for best resolution use, or use a constant value for
         *      more stable shadows.
         * @param mainCamera        The camera to use to determine the optimal
         *      splits (normally, the main camera that the shadows will be
         *      rendered to)
         *
         * Should be called whenever your camera moves.
         */
        void setTarget(const Vector3& lightDirection, const Vector3& screenDirection, SceneGraph::Camera3D& mainCamera);

        /**
         * @brief Render a group of shadow-casting drawables to the shadow maps
         */
        void render(SceneGraph::DrawableGroup3D& drawables);

        std::vector<Vector3> layerFrustumCorners(SceneGraph::Camera3D& mainCamera, Int layer);

        Float cutZ(Int layer) const;

        Float cutDistance(Float zNear, Float zFar, Int layer) const;

        std::size_t layerCount() const { return _layers.size(); }

        const Matrix4& layerMatrix(Int layer) const {
            return _layers[layer].shadowMatrix;
        }

        std::vector<Vector4> calculateClipPlanes();

        GL::Texture2DArray& shadowTexture() { return _shadowTexture; }

    private:
        Object3D& _object;
        GL::Texture2DArray _shadowTexture;

        struct ShadowLayerData {
            GL::Framebuffer shadowFramebuffer;
            Matrix4 shadowCameraMatrix;
            Matrix4 shadowMatrix;
            Vector2 orthographicSize;
            Float orthographicNear, orthographicFar;
            Float cutPlane;

            explicit ShadowLayerData(const Vector2i& size);
        };

        std::vector<ShadowLayerData> _layers;
};

}}

#endif
