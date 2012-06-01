#version 330

layout(location = 0) in vec4 vertex;
layout(location = 1) in vec2 textureCoordinates;

out vec2 varyingTextureCoordinates;

void main() {
    varyingTextureCoordinates = textureCoordinates;

    gl_Position = vertex;
}
