uniform mat4 matrix;

layout(location = 0) in vec4 vertex;

out vec2 textureCoords;

void main() {
    textureCoords = vertex.xy/2+vec2(0.5);
    gl_Position = matrix*vertex;
}
