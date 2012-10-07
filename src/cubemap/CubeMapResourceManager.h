#ifndef Magnum_Examples_CubeMapResourceManager_h
#define Magnum_Examples_CubeMapResourceManager_h
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

namespace Magnum {

class IndexedMesh;
template<std::uint8_t> class Texture;
typedef Texture<2> Texture2D;
class CubeMapTexture;
class AbstractShaderProgram;

namespace Trade {
    class AbstractImporter;
}

extern template class ResourceManager<IndexedMesh, Trade::AbstractImporter, Texture2D, CubeMapTexture, AbstractShaderProgram>;

namespace Examples {

typedef ResourceManager<IndexedMesh, Trade::AbstractImporter, Texture2D, CubeMapTexture, AbstractShaderProgram> CubeMapResourceManager;

}}

#endif
