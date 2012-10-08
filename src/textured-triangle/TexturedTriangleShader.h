#ifndef Magnum_Examples_TexturedTriangleShader_h
#define Magnum_Examples_TexturedTriangleShader_h
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

#include <AbstractShaderProgram.h>
#include <Color.h>

namespace Magnum { namespace Examples {

class TexturedTriangleShader: public Magnum::AbstractShaderProgram {
    public:
        typedef Magnum::AbstractShaderProgram::Attribute<0, Magnum::Point2D> Position;
        typedef Magnum::AbstractShaderProgram::Attribute<1, Magnum::Vector2> TextureCoordinates;

        static const GLint TextureLayer = 0;

        TexturedTriangleShader();

        inline TexturedTriangleShader* setBaseColor(const Color3<GLfloat>& color) {
            setUniform(baseColorUniform, color);
            return this;
        }

    private:
        GLint baseColorUniform;
};

}}

#endif
