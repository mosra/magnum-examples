#ifndef Magnum_Examples_CubeMap_h
#define Magnum_Examples_CubeMap_h
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

#include <SceneGraph/Drawable.h>

#include "Types.h"

namespace Magnum { namespace Examples {

class CubeMapShader;

class CubeMap: public Object3D, SceneGraph::Drawable3D<> {
    public:
        CubeMap(const std::string& prefix, Object3D* parent, SceneGraph::DrawableGroup3D<>* group);

        void draw(const Matrix4& transformationMatrix, SceneGraph::AbstractCamera3D<>* camera) override;

    private:
        Resource<Buffer> buffer;
        Resource<Mesh> cube;
        Resource<AbstractShaderProgram, CubeMapShader> shader;
        Resource<CubeMapTexture> texture;
};

}}

#endif
