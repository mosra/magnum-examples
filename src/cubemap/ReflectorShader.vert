/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024 — Vladimír Vondruš <mosra@centrum.cz>

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

uniform mediump mat4 transformationMatrix;
uniform mediump mat3 normalMatrix;
uniform mediump mat4 projectionMatrix;
uniform mediump mat3 cameraMatrix;
uniform lowp float reflectivity;

layout(location = 0) in mediump vec4 position;
layout(location = 1) in mediump vec2 textureCoords;

out lowp float factor;
out mediump vec3 cubeMapTextureCoords;
out mediump vec2 tarnishTextureCoords;

void main(void) {
    mediump vec4 transformedVertex = transformationMatrix*position;
    mediump vec3 transformedNormal = normalize(normalMatrix*position.xyz);

    /* Reflection vector */
    mediump vec3 reflection = reflect(normalize(transformedVertex.xyz), transformedNormal);
    cubeMapTextureCoords = cameraMatrix*reflection;

    /* Factor of reflectivity - normals perpendicular to viewer are not reflective */
    factor = pow(1.0 - max(0.0, dot(transformedNormal,
        normalize(cameraMatrix*normalMatrix*vec3(0, 0, 1)))),
        reflectivity);

    tarnishTextureCoords = textureCoords;
    gl_Position = projectionMatrix*transformedVertex;
}
