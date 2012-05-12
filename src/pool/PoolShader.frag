#version 330

uniform mat4 transformationMatrix;
uniform mat3 normalMatrix;
uniform sampler2D diffuseTexture;
uniform sampler2D waterTexture;
uniform vec2 waterTextureTranslation;
uniform vec3 cameraDirection;
uniform vec3 light[POOL_LIGHT_COUNT];

in vec3 fragPosition;

out vec4 fragColor;

void main() {
    vec3 intersection = vec3(0);
    vec3 normal = vec3(0);
    vec2 texCoord = vec2(0);

    fragColor = vec4(0, 0, 0, 1);

    /* Pool */
    if(fragPosition.x >= -1.0 && fragPosition.x <= 1.0 && fragPosition.z >= -1.0 && fragPosition.z <= 1.0) {

        /* Direction from camera to fragment */
        vec3 direction = cameraDirection+fragPosition;

        direction += normalize(texture(waterTexture, (fragPosition.xz+1.0)/2+waterTextureTranslation).xyz)/5;

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

        texCoord = texCoord*10;

        /* More colored water down there */
        fragColor.rgb = mix(fragColor.rgb, vec3(0.2, 0.67, 0.9), -intersection.y/1.67);

    /* Surface */
    } else {
        intersection = fragPosition;
        texCoord = fragPosition.xz*10;
        normal = vec3(0, 1, 0);
    }

    fragColor += texture(diffuseTexture, texCoord);

    /* Light intensity */
    float intensity = 0.0;
    for(int i = 0; i != POOL_LIGHT_COUNT; ++i)
        intensity += max(0.0, dot(normalMatrix*normal, normalize(light[i] - (transformationMatrix*vec4(intersection, 1.0)).xyz)));

    fragColor.rgb *= intensity;
}
