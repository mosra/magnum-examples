#ifndef Magnum_Examples_Types_h
#define Magnum_Examples_Types_h
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

#include <ResourceManager.h>
#include <SceneGraph/MatrixTransformation3D.h>

namespace Magnum {

namespace Trade {
    class AbstractImporter;
}

extern template class ResourceManager<Buffer, Mesh, Trade::AbstractImporter, Texture2D, CubeMapTexture, AbstractShaderProgram>;

namespace Examples {

typedef ResourceManager<Buffer, Mesh, Trade::AbstractImporter, Texture2D, CubeMapTexture, AbstractShaderProgram> CubeMapResourceManager;
typedef SceneGraph::Object<SceneGraph::MatrixTransformation3D<>> Object3D;
typedef SceneGraph::Scene<SceneGraph::MatrixTransformation3D<>> Scene3D;

}}

#endif
