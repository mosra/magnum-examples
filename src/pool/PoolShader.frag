#version 330

uniform sampler2D diffuseTexture;
uniform vec3 cameraDirection;
uniform vec3 light[POOL_LIGHT_COUNT];

in vec3 fragPosition;

out vec4 fragColor;

void main() {
    /* Direction from camera to fragment */
    vec3 direction = cameraDirection+fragPosition;

    vec3 intersection = vec3(0);
    vec3 normal = vec3(0);
    vec2 texCoord = vec2(0);

    /* Near */
    if(direction.z > 0 && (intersection = fragPosition + (direction/direction.z)*(1-fragPosition.z)).z > abs(intersection.x)) {
        texCoord = intersection.xy;
        normal = vec3(0, 0, -1);

    /* Far */
    } else if(direction.z <= 0 && -(intersection = fragPosition + (direction/-direction.z)*(1+fragPosition.z)).z >= abs(intersection.x)) {
        texCoord = intersection.xy;
        normal = vec3(0, 0, 1);

    /* Right */
    } else if(direction.x > 0 && (intersection = fragPosition + (direction/direction.x)*(1-fragPosition.x)).x > abs(intersection.z)) {
        texCoord = intersection.zy;
        normal = vec3(-1, 0, 0);

    /* Left */
    } else if(direction.x <= 0 && -(intersection = fragPosition + (direction/-direction.x)*(1+fragPosition.x)).x >= abs(intersection.z)) {
        texCoord = intersection.zy;
        normal = vec3(1, 0, 0);
    }

    texCoord = (texCoord+vec2(1.0))*10;
    fragColor = texture(diffuseTexture, texCoord);

    /* Light intensity */
    float intensity = 0.0;
    for(int i = 0; i != POOL_LIGHT_COUNT; ++i)
        intensity += max(0.0, dot(normal, normalize(light[i] - intersection)));

    fragColor.rgb *= intensity;
}
