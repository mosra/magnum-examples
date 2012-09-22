layout(location = 0) in vec3 position;
layout(location = 1) in vec2 textureCoordinates;

out vec2 varyingTextureCoordinates;

void main() {
    varyingTextureCoordinates = textureCoordinates;

    gl_Position.xywz = vec4(position, 0.0);
}
