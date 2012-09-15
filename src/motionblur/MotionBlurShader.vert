layout(location = 0) in vec4 vertex;

out vec2 textureCoordinate;

void main() {
    textureCoordinate = (vertex.xy + vec2(1, 1))/2;
    gl_Position = vertex;
}
