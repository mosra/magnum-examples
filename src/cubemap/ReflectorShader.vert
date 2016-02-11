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
        cameraMatrix*normalMatrix*vec3(0.0, 0.0, 1.0))),
        reflectivity);

    tarnishTextureCoords = textureCoords;
    gl_Position = projectionMatrix*transformedVertex;
}
