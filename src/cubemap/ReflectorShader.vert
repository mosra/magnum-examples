/*
    This file is part of Magnum.

    Copyright © 2010, 2011, 2012, 2013, 2014
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

uniform mat4 transformationMatrix;
uniform mat3 normalMatrix;
uniform mat4 projectionMatrix;
uniform mat3 cameraMatrix;
uniform float reflectivity;

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 textureCoords;

out float factor;
out vec3 cubeMapTextureCoords;
out vec2 tarnishTextureCoords;

void main(void) {
    vec4 transformedVertex = transformationMatrix*position;
    vec3 transformedNormal = normalize(normalMatrix*position.xyz);

    /* Reflection vector */
    vec3 reflection = reflect(normalize(transformedVertex.xyz), transformedNormal);
    cubeMapTextureCoords = cameraMatrix*reflection;

    /* Factor of reflectivity - normals perpendicular to viewer are not reflective */
    factor = pow(1 - max(0.0, dot(transformedNormal,
        cameraMatrix*normalMatrix*vec3(0, 0, 1))),
        reflectivity);

    tarnishTextureCoords = textureCoords;
    gl_Position = projectionMatrix*transformedVertex;
}
