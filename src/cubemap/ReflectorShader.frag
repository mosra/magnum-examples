uniform vec3 diffuseColor;
uniform samplerCube textureData;
uniform sampler2D tarnishTextureData;

in vec3 cubeMapTextureCoords;
in vec2 tarnishTextureCoords;
in float factor;

out vec4 fragmentColor;

void main(void) {
    /* Combine diffuse color and reflection based on refection factor,
       multiply with tarnish texture */
    fragmentColor.rgb = mix(texture(textureData, cubeMapTextureCoords).rgb, diffuseColor, factor)
        * texture(tarnishTextureData, tarnishTextureCoords).rgb;
    fragmentColor.a = 1.0;
}
