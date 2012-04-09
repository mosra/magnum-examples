#version 330

uniform mat4 matrix;

in vec4 vertex;
in vec3 color;

out vec3 interpolatedColor;

void main() {
    interpolatedColor = color;

    gl_Position = matrix*vertex;
}