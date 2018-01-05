/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018 —
            Vladimír Vondruš <mosra@centrum.cz>
        2016 — Bill Robinson <airbaggins@gmail.com>

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

uniform highp mat4 modelMatrix;
uniform highp mat4 transformationProjectionMatrix;
uniform highp mat4 shadowmapMatrix[NUM_SHADOW_MAP_LEVELS];

in highp vec4 position;
in mediump vec3 normal;

out mediump vec3 transformedNormal;

out highp vec3 shadowCoords[NUM_SHADOW_MAP_LEVELS];

void main() {
    transformedNormal = mat3(modelMatrix)*normal;

    vec4 worldPos4 = modelMatrix * position;
    for(int i = 0; i < shadowmapMatrix.length(); i++) {
        shadowCoords[i] = (shadowmapMatrix[i]*worldPos4).xyz;
    }

    gl_Position = transformationProjectionMatrix*position;
}
