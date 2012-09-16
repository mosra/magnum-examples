layout(location = 0) in vec3 vertex;
layout(location = 1) in vec2 textureCoordinates;

out vec2 varyingTextureCoordinates;

void main() {
    varyingTextureCoordinates = textureCoordinates;

    gl_Position.xywz = vec4(vertex, 0.0);
}
