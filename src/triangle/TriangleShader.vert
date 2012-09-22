layout(location = 0) in vec3 position;
layout(location = 1) in vec3 color;

out vec3 varyingColor;

void main() {
    varyingColor = color;

    gl_Position.xywz = vec4(position, 0.0);
}
