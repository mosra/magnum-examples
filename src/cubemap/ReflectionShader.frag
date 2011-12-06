#version 330

uniform vec3 diffuseColor;
uniform samplerCube textureData;

in vec3 textureCoords;
in float factor;

out vec4 fragmentColor;

void main(void) {
    /* Combine diffuse color and reflection based on refection factor */
    fragmentColor.rgb = mix(texture(textureData, textureCoords).rgb, diffuseColor, factor);
    fragmentColor.a = 1.0;
}
