/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019,
        2020, 2021, 2022, 2023 — Vladimír Vondruš <mosra@centrum.cz>
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

uniform highp mat4 projectionMatrix;
uniform highp vec3 lightDir;

uniform lowp vec3 ambientColor;
uniform lowp vec3 specularColor;
uniform mediump float shininess;

uniform mediump float particleRadius;

flat in mediump vec3 viewCenter;
flat in lowp vec3 color;
out lowp vec4 fragmentColor;

lowp vec3 shadeLight(mediump vec3 normal, mediump vec3 fragPos, mediump vec3 viewDir) {
    mediump vec3 halfDir  = normalize(lightDir - viewDir);
    mediump vec3 diffuse  = max(dot(normal, lightDir), 0.0)*color;
    mediump vec3 specular = pow(max(dot(halfDir, normal), 0.0), shininess)*specularColor;

    return ambientColor + diffuse * 0.5 + specular; /* scale diffuse to 0.5 */
}

void main() {
    mediump vec3 viewDir = normalize(viewCenter);
    mediump vec3 normal;
    mediump vec3 fragPos;

    normal.xy = gl_PointCoord.xy*vec2(2.0, -2.0) + vec2(-1.0, 1.0);
    mediump float mag = dot(normal.xy, normal.xy);
    if(mag > 1.0) discard; /* outside the sphere */

    normal.z = sqrt(1.0 - mag);
    fragPos = viewCenter + normal*particleRadius; /* correct fragment position */

    mediump mat4 prjMatTransposed = transpose(projectionMatrix);
    mediump float z = dot(vec4(fragPos, 1.0), prjMatTransposed[2]);
    mediump float w = dot(vec4(fragPos, 1.0), prjMatTransposed[3]);
    gl_FragDepth = 0.5*(z/w + 1.0); /* correct fragment depth */

    fragmentColor = vec4(shadeLight(normal, fragPos, viewDir), 1.0);
}
