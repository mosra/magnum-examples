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
