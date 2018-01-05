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

uniform float shadowBias;
uniform sampler2DArrayShadow shadowmapTexture;
uniform highp vec3 lightDirection;

in mediump vec3 transformedNormal;
in highp vec3 shadowCoords[NUM_SHADOW_MAP_LEVELS];
uniform float shadowDepthSplits[NUM_SHADOW_MAP_LEVELS];

out lowp vec4 color;

void main() {
    /* You might want to source this from a texture or a vertex color */
    vec3 albedo = vec3(0.5,0.5,0.5);

    /* You might want to source this from a uniform */
    vec3 ambient = vec3(0.5,0.5,0.5);

    mediump vec3 normalizedTransformedNormal = normalize(transformedNormal);

    float inverseShadow = 1.0;

    /* Is the normal of this face pointing towards the light? */
    lowp float intensity = dot(normalizedTransformedNormal, lightDirection);

    /* Pointing away from the light anyway, we know it's in the shade, don't
       bother shadow map lookup */
    if(intensity <= 0) {
        inverseShadow = 0.0f;
        intensity = 0.0f;

    } else {
        int shadowLevel = 0;
        bool inRange = false;

        /* Starting with highest resolution shadow map, find one we're in range
           of */
        for(; shadowLevel < NUM_SHADOW_MAP_LEVELS; ++shadowLevel) {
            vec3 shadowCoord = shadowCoords[shadowLevel];
            inRange = shadowCoord.x >= 0 &&
                      shadowCoord.y >= 0 &&
                      shadowCoord.x <  1 &&
                      shadowCoord.y <  1 &&
                      shadowCoord.z >= 0 &&
                      shadowCoord.z <  1;
            if(inRange) {
                inverseShadow = texture(shadowmapTexture, vec4(shadowCoord.xy, shadowLevel, shadowCoord.z-shadowBias));
                break;
            }
        }

        #ifdef DEBUG_SHADOWMAP_LEVELS
        switch(shadowLevel) {
            case 0: albedo *= vec3(1,0,0); break;
            case 1: albedo *= vec3(1,1,0); break;
            case 2: albedo *= vec3(0,1,0); break;
            case 3: albedo *= vec3(0,1,1); break;
            default: albedo *= vec3(1,0,1); break;
        }
        #else
        if(!inRange) {
            // If your shadow maps don't cover your entire view, you might want to remove this
            albedo *= vec3(1,0,1); //Something has gone wrong - didn't find a shadow map
        }
        #endif
    }

    color.rgb = ((ambient + vec3(intensity*inverseShadow))*albedo);
    color.a = 1.0;
}
