uniform mat4 transformationProjectionMatrix;

layout(location = 0) in vec4 position;

out vec3 textureCoords;

void main(void) {
    textureCoords = position.xyz;

    gl_Position = transformationProjectionMatrix*position;
}
