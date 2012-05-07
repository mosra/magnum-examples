#version 330

uniform mat4 transformationMatrix;
uniform mat4 projectionMatrix;

out vec2 texCoord;

const vec2 data[4] = vec2[](
    vec2(-1.0,  1.0),
    vec2(-1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2( 1.0, -1.0)
);

void main() {
    /* Texture coordinate */
    texCoord = (data[gl_VertexID]+vec2(1.0))*10;

    /* Position */
    gl_Position = projectionMatrix*transformationMatrix*vec4(data[gl_VertexID], 0.0, 1.0).xzyw;
}
