uniform vec3 baseColor = vec3(1.0, 1.0, 1.0);
uniform sampler2D textureData;

in vec2 varyingTextureCoordinates;

out vec4 fragmentColor;

void main() {
    fragmentColor.rgb = baseColor*texture(textureData, varyingTextureCoordinates).rgb;
    fragmentColor.a = 1.0;
}
