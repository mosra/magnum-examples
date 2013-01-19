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
