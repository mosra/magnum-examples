/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014, 2015
              Vladimír Vondruš <mosra@centrum.cz>

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

#define CORRECTION_TEXTURE_SIZE 1024

uniform sampler2D textureData;
uniform samplerBuffer colorCorrectionTextureData;

in vec2 textureCoords;

layout(location = 0) out vec4 original;
layout(location = 1) out vec4 grayscale;
layout(location = 2) out vec4 corrected;

void main() {
    /* Original color */
    vec4 color = texture(textureData, textureCoords);
    original = color;

    /* Grayscale */
    float gray = dot(color.rgb, vec3(0.3, 0.59, 0.11));
    grayscale = vec4(gray, gray, gray, 1.0);

    /* Color corrected */
    corrected.r = texelFetch(colorCorrectionTextureData, int(color.r*CORRECTION_TEXTURE_SIZE-1)).r;
    corrected.g = texelFetch(colorCorrectionTextureData, int(color.g*CORRECTION_TEXTURE_SIZE-1)).r;
    corrected.b = texelFetch(colorCorrectionTextureData, int(color.b*CORRECTION_TEXTURE_SIZE-1)).r;
}
