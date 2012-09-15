layout(location = 0) in vec4 vertex;
layout(location = 1) in vec4 color;

out vec4 varyingColor;

void main() {
    varyingColor = color;

    gl_Position = vertex;
}
