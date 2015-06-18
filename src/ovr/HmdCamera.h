#ifndef Magnum_Examples_HmdCamera_h
#define Magnum_Examples_HmdCamera_h
/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015
              Vladimír Vondruš <mosra@centrum.cz>
    Copyright © 2015
              Jonathan Hale <squareys@googlemail.com>

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#include <memory>

#include <Magnum/SceneGraph/Camera3D.h>
#include <Magnum/LibOvrIntegration/LibOvrIntegration.h>

#include "Types.h"

namespace Magnum { namespace Examples {

class HmdCamera: public SceneGraph::Camera3D {
    public:
        /**
         * @brief Constructor.
         * @param hmd Hmd which this camera belongs to.
         * @param eye Eye index associated with this camera. (0 for left, 1 for right eye)
         * @param object Object holding this feature.
         */
        explicit HmdCamera(LibOvrIntegration::Hmd& hmd, const int eye, SceneGraph::AbstractObject3D& object);

        void draw(SceneGraph::DrawableGroup3D& group) override;

        /**
         * @return Reference to the texture set used for rendering.
         */
        LibOvrIntegration::SwapTextureSet& getTextureSet() const {
            return *_textureSet;
        }

    private:

        void createEyeRenderTexture();

        std::unique_ptr<Texture2D> _depth;

        LibOvrIntegration::Hmd& _hmd;
        std::unique_ptr<LibOvrIntegration::SwapTextureSet> _textureSet;

        Vector2i _textureSize;
        std::unique_ptr<Framebuffer> _framebuffer;
};

}}

#endif
