#version 330

uniform samplerCube textureData;

in vec3 textureCoords;

out vec4 fragmentColor;

void main(void) {
    fragmentColor = texture(textureData, textureCoords);
}
