#ifndef Magnum_Examples_TexturedTriangleExample_h
#define Magnum_Examples_TexturedTriangleExample_h
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

#include <Mesh.h>
#include <Texture.h>
#include <Contexts/GlutContext.h>

#include "TexturedTriangleShader.h"

namespace Magnum { namespace Examples {

class TexturedTriangleExample: public Magnum::Contexts::GlutContext {
    public:
        TexturedTriangleExample(int& argc, char** argv);

    protected:
        void viewportEvent(const Magnum::Math::Vector2<GLsizei>& size);
        void drawEvent();

    private:
        Magnum::Mesh mesh;
        TexturedTriangleShader shader;
        Magnum::Texture2D texture;
};

}}

#endif
