layout(location = 0) in vec4 position;
layout(location = 1) in vec3 color;

out vec3 varyingColor;

void main() {
    varyingColor = color;

    gl_Position = position;
}
