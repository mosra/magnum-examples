/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023, 2024, 2025
             — Vladimír Vondruš <mosra@centrum.cz>
        2019 — Nghia Truong <nghiatruong.vn@gmail.com>

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

uniform highp mat3 viewProjectionMatrix;
uniform int numParticles;
uniform int colorMode;
uniform int screenHeight;
uniform int domainHeight;
uniform float particleRadius;
uniform vec3 uniformColor;


layout(location = 0) in highp vec2 position;
flat out vec3 color;

const vec3 colorRamp[] = vec3[] (
    vec3(1.0, 0.0, 0.0),
    vec3(1.0, 0.5, 0.0),
    vec3(1.0, 1.0, 0.0),
    vec3(1.0, 0.0, 1.0),
    vec3(0.0, 1.0, 0.0),
    vec3(0.0, 1.0, 1.0),
    vec3(0.0, 0.0, 1.0)
);

vec3 generateVertexColor() {
    if(colorMode == 1 ) { /* ramp color by particle id */
        float segmentSize = float(numParticles)/6.0f;
        float segment = floor(float(gl_VertexID)/segmentSize);
        float t = (float(gl_VertexID) - segmentSize*segment)/segmentSize;
        vec3 startVal = colorRamp[int(segment)];
        vec3 endVal = colorRamp[int(segment) + 1];
        return mix(startVal, endVal, t);
    } else { /* uniform diffuse color */
        return uniformColor;
    }
}

void main() {
    color = generateVertexColor();
    gl_PointSize = particleRadius * float(screenHeight) / float(domainHeight);
    gl_Position = mat4(viewProjectionMatrix) * vec4(position, 0, 1.0);
}
