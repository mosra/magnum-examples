#ifndef Magnum_Examples_Billboard_h
#define Magnum_Examples_Billboard_h
/*
    Copyright © 2010, 2011, 2012 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Magnum.

    Magnum is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Magnum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

#include "BufferedTexture.h"
#include "Mesh.h"
#include "Texture.h"
#include "SceneGraph/Object.h"
#include "Trade/ImageData.h"

#include "ColorCorrectionShader.h"

namespace Magnum { namespace Examples {

class Billboard: public SceneGraph::Object3D {
    public:
        Billboard(Trade::ImageData2D* image, Buffer* colorCorrectionBuffer, SceneGraph::Object3D* parent = nullptr);

        void draw(const Matrix4& transformationMatrix, SceneGraph::Camera3D* camera);

    private:
        Mesh mesh;
        Texture2D texture;
        BufferedTexture colorCorrectionTexture;
        ColorCorrectionShader shader;
};

}}

#endif
