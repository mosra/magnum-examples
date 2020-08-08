/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 —
            Vladimír Vondruš <mosra@centrum.cz>
        2020 — Janos Meny <janos.meny@gmail.com>

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

in vec2 textureCoordinate;

uniform sampler2D positionTexture;
uniform sampler2D normalTexture;
uniform sampler2D albedoTexture;
uniform sampler2D ambientOcclusionTexture;

uniform vec3 lightPosition;
uniform vec3 lightColor;
uniform float shininess;
uniform vec3 specularColor;

out vec4 fragmentColor;

#define BLURR_RADIUS 5

void main()
{
    /* retrieve data from gbuffer */
    vec3 position = texture(positionTexture, textureCoordinate).rgb;

#ifndef DRAW_OCCLUSION_FACTOR
    vec3 normal = texture(normalTexture, textureCoordinate).rgb;
    vec3 albedo = texture(albedoTexture, textureCoordinate).rgb;
#endif

    /* blur occlusion factor */
    vec2 texelSize = 1.0 / vec2(textureSize(ambientOcclusionTexture, 0));
    float blurredOcclusion = 0.0;
    for (int x = -BLURR_RADIUS; x <= BLURR_RADIUS; ++x) {
        for (int y = -BLURR_RADIUS; y <= BLURR_RADIUS; ++y) {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            blurredOcclusion += texture(ambientOcclusionTexture, textureCoordinate + offset).x;
        }
    }
    blurredOcclusion /= float((BLURR_RADIUS*2 + 1)*(BLURR_RADIUS*2 + 1));
    blurredOcclusion = pow(blurredOcclusion, 4); /* To make the effect more visible */

#ifndef DRAW_OCCLUSION_FACTOR
    /* No ambient color */
    fragmentColor = vec4(0,0,0,1);

    /* normalize normal */
    mediump vec3 normalizedTransformedNormal = normalize(normal);

    /* Add diffuse color */
    highp vec3 normalizedLightDirection = normalize(lightPosition - position);
    lowp float intensity = max(0.0, dot(normalizedTransformedNormal, normalizedLightDirection));
    fragmentColor.rgb += albedo*lightColor*intensity*blurredOcclusion;

    /* Add specular color, if needed */
    if(intensity > 0.001) {
        highp vec3 reflection = reflect(-normalizedLightDirection, normalizedTransformedNormal);
        mediump float specularity = clamp(pow(max(0.0, dot(normalize(-position), reflection)), shininess), 0.0, 1.0);
        fragmentColor.rgb += specularColor*specularity; /* white specular color */
    }
#else
    fragmentColor.rgb = vec3(blurredOcclusion);
#endif
}

