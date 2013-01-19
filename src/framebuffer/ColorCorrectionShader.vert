uniform mat3 matrix;

layout(location = 0) in vec3 vertex;

out vec2 textureCoords;

void main() {
    textureCoords = vertex.xy/2+vec2(0.5);
    gl_Position.xywz = vec4(matrix*vertex, 0.0);
}
