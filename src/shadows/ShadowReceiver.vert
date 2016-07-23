/* magnum-shadows - A Cascading/Parallel-Split Shadow Mapping example
 * Written in 2016 by Bill Robinson airbaggins@gmail.com
 *
 * To the extent possible under law, the author(s) have dedicated all copyright and related and neighboring rights to
 * this software to the public domain worldwide. This software is distributed without any warranty.
 *
 * See <http://creativecommons.org/publicdomain/zero/1.0/>.
 *
 * Credit is appreciated, but not required.
 * */
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
    for (int i = 0; i < shadowmapMatrix.length(); i++) {
        shadowCoords[i] = (shadowmapMatrix[i] * worldPos4).xyz;
    }

    gl_Position = transformationProjectionMatrix * position;
}
