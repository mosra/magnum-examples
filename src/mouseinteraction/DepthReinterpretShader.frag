/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 —
            Vladimír Vondruš <mosra@centrum.cz>

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

uniform highp sampler2D depthTexture;

in mediump vec2 textureCoordinates;

out highp vec4 reinterpretDepth;

void main() {
    /* Rescale so 1.0 is mapped to 0xffffffff instead of 0x100000000 */
    highp float depth = texture(depthTexture, textureCoordinates).r*((256.0*256.0*256.0*256.0 - 1.0)/(256.0*256.0*256.0*256.0));

    /* Store continued fractions in the resulting 8bit vector. We don't want
       rounding to happen because we store remaining fraction in the
       immediately next component, so ensure the stored values are clearly
       mappable to 0-255 */
    reinterpretDepth = floor(255.0*fract(depth*vec4(
        1.0,
        256.0,
        256.0*256.0,
        256.0*256.0*256.0)))/255.0;
}
