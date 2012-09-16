layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 color;

out vec3 varyingColor;

void main() {
    varyingColor = color;

    gl_Position.xywz = vec4(vertex, 0.0);
}
