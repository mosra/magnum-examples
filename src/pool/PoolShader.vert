uniform mat4 transformationMatrix;
uniform mat4 projectionMatrix;

out vec3 fragPosition;

const vec2 data[4] = vec2[](
    vec2( 2.5,  2.5),
    vec2( 2.5, -2.5),
    vec2(-2.5,  2.5),
    vec2(-2.5, -2.5)
);

void main() {
    vec4 vec = vec4(0.0);
    switch(gl_VertexID) {
        case 0: vec = vec4(data[0], 0.0, 1.0).xzyw; break;
        case 1: vec = vec4(data[1], 0.0, 1.0).xzyw; break;
        case 2: vec = vec4(data[2], 0.0, 1.0).xzyw; break;
        case 3: vec = vec4(data[3], 0.0, 1.0).xzyw; break;
    }

    /* Position */
    fragPosition = vec.xyz;
    gl_Position = projectionMatrix*transformationMatrix*vec;
}
