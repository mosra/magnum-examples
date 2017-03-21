#ifndef Magnum_Examples_HmdCamera_h
#define Magnum_Examples_HmdCamera_h
/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017 —
            Vladimír Vondruš <mosra@centrum.cz>
        2015, 2016 — Jonathan Hale <squareys@googlemail.com>

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

#include <memory>
#include <Magnum/SceneGraph/Camera.h>
#include <Magnum/OvrIntegration/OvrIntegration.h>

#include "Types.h"

namespace Magnum { namespace Examples {

/**
@brief HMD camera

Camera which renders a scene to a @ref OvrIntegration::TextureSwapChain.
Handles framebuffer creation and activation.
*/
class HmdCamera: public SceneGraph::Camera3D {
    public:
        /**
         * @brief Constructor
         * @param session   Oculus session for the HMD to create this camera
         *      for
         * @param eye       Eye index associated with this camera. (0 for left,
         *      1 for right eye)
         * @param object    Object holding this feature
         */
        explicit HmdCamera(OvrIntegration::Session& session, const int eye, SceneGraph::AbstractObject3D& object);

        void draw(SceneGraph::DrawableGroup3D& group) override;

        /** @brief Reference to the texture set used for rendering */
        OvrIntegration::TextureSwapChain& textureSwapChain() const {
            return *_textureSwapChain;
        }

    private:
        void createEyeRenderTexture();

        std::unique_ptr<Texture2D> _depth;

        OvrIntegration::Session& _session;
        std::unique_ptr<OvrIntegration::TextureSwapChain> _textureSwapChain;

        Vector2i _textureSize;
        std::unique_ptr<Framebuffer> _framebuffer;
};

}}

#endif
