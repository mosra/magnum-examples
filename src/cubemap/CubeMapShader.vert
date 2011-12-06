#version 330

uniform mat4 modelViewProjectionMatrix;

in vec4 vertex;

out vec3 textureCoords;

void main(void) {
    textureCoords = normalize(vertex.xyz);

    gl_Position = modelViewProjectionMatrix*vertex;
}
