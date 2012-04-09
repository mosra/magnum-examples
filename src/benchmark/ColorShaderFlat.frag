#version 330

flat in vec3 flatColor;

out vec4 color;

void main() {
    color.rgb = flatColor;
    color.a = 1.0;
}
