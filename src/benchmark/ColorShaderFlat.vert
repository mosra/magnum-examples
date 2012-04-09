#version 330

uniform mat4 matrix;

in vec4 vertex;
in vec3 color;

flat out vec3 flatColor;

void main() {
    flatColor = color;

    gl_Position = matrix*vertex;
}