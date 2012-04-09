#version 330

in vec3 interpolatedColor;

out vec4 color;

void main() {
    color.rgb = interpolatedColor;
    color.a = 1.0;
}
