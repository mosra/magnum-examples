#define CORRECTION_TEXTURE_SIZE 1024

uniform sampler2D textureData;
uniform samplerBuffer colorCorrectionTextureData;

in vec2 textureCoords;

layout(location = 0) out vec4 original;
layout(location = 1) out vec4 grayscale;
layout(location = 2) out vec4 corrected;

void main() {
    /* Original color */
    vec4 color = texture(textureData, textureCoords);
    original = color;

    /* Grayscale */
    float gray = dot(color.rgb, vec3(0.3, 0.59, 0.11));
    grayscale = vec4(gray, gray, gray, 1.0);

    /* Color corrected */
    corrected.r = texelFetch(colorCorrectionTextureData, int(color.r*CORRECTION_TEXTURE_SIZE-1)).r;
    corrected.g = texelFetch(colorCorrectionTextureData, int(color.g*CORRECTION_TEXTURE_SIZE-1)).r;
    corrected.b = texelFetch(colorCorrectionTextureData, int(color.b*CORRECTION_TEXTURE_SIZE-1)).r;
}
