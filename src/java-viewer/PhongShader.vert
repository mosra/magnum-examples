#version 330

uniform mat4 transformationMatrix;
uniform mat4 projectionMatrix;
uniform vec3 light[LIGHT_COUNT];

in vec4 vertex;
in vec3 normal;

out vec3 transformedNormal;
out vec3 lightDirection[LIGHT_COUNT];
out vec3 cameraDirection;

void main() {
    /* Transformed vertex position */
    vec4 transformedVertex4 = transformationMatrix*vertex;
    vec3 transformedVertex = transformedVertex4.xyz/transformedVertex4.w;

    /* Transformed normal vector */
    transformedNormal = normalize(mat3x3(transformationMatrix)*normal);

    /* Direction to the light */
    for(int i = 0; i != LIGHT_COUNT; ++i)
        lightDirection[i] = normalize(light[i] - transformedVertex);

    /* Direction to the camera */
    cameraDirection = -transformedVertex;

    /* Transform the vertex */
    gl_Position = projectionMatrix*transformedVertex4;
}
