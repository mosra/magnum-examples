#version 330

uniform mat4 transformationMatrix;
uniform mat4 projectionMatrix;

out vec3 fragPosition;

const vec2 data[4] = vec2[](
    vec2(-2.5,  2.5),
    vec2(-2.5, -2.5),
    vec2( 2.5,  2.5),
    vec2( 2.5, -2.5)
);

void main() {
    vec4 vec = vec4(data[gl_VertexID], 0.0, 1.0).xzyw;

    /* Position */
    fragPosition = vec.xyz;
    gl_Position = projectionMatrix*transformationMatrix*vec;
}
