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
    vec3 transformedFragPosition = (transformationMatrix*vec4(fragPosition, 1.0)).xyz;
    vec3 transformedCameraDirection = normalize(-transformedFragPosition);

    /* Pool */
    if(fragPosition.x >= -1.0 && fragPosition.x <= 1.0 && fragPosition.z >= -1.0 && fragPosition.z <= 1.0) {

        /* Direction from camera to fragment */
        vec3 direction = cameraDirection+fragPosition;
        vec3 waterNormal = normalize(texture(waterTexture, (fragPosition.xz+1.0)/2+waterTextureTranslation).xyz);

        direction += waterNormal/5;

        /* Near */
        vec3 intersection = vec3(0.0);
        vec3 intersectionNormal = vec3(0.0);
        vec2 texCoord = vec2(0.0);
        if(direction.z > 0 && (intersection = fragPosition + (direction/direction.z)*(1-fragPosition.z)).z > abs(intersection.x)) {
            texCoord = intersection.xy;
            intersectionNormal = vec3(0.0, 0.0, -1.0);

        /* Far */
        } else if(direction.z <= 0 && -(intersection = fragPosition + (direction/-direction.z)*(1+fragPosition.z)).z >= abs(intersection.x)) {
            texCoord = intersection.xy;
            intersectionNormal = vec3(0.0, 0.0, 1.0);

        /* Right */
        } else if(direction.x > 0 && (intersection = fragPosition + (direction/direction.x)*(1-fragPosition.x)).x > abs(intersection.z)) {
            texCoord = intersection.zy;
            intersectionNormal = vec3(-1.0, 0.0, 0.0);

        /* Left */
        } else if(direction.x <= 0 && -(intersection = fragPosition + (direction/-direction.x)*(1+fragPosition.x)).x >= abs(intersection.z)) {
            texCoord = intersection.zy;
            intersectionNormal = vec3(1.0, 0.0, 0.0);
        }

        /* More colored water down there */
        fragColor.rgb = mix(vec3(0.0), vec3(0.2, 0.67, 0.9), -intersection.y/1.67);
        fragColor.w = 0.0;

        fragColor += texture(diffuseTexture, texCoord*10);

        float totalIntensity = 0.0;
        vec3 totalHighlight = vec3(0.0);
        vec3 transformedIntersection = (transformationMatrix*vec4(intersection, 1.0)).xyz;
        vec3 transformedIntersectionNormal = normalMatrix*intersectionNormal;
        vec3 transformedWaterNormal = normalMatrix*normalize(vec3(0.0, 1.0, 0.0)+waterNormal/5);
        for(int i = 0; i != POOL_LIGHT_COUNT; ++i) {
            vec3 lightDirection = normalize(light[i] - transformedFragPosition);
            vec3 intersectionLightDirection = normalize(light[i] - transformedIntersection);

            /* Light intensity at wall intersection */
            float intensity = max(0.0, dot(transformedIntersectionNormal, intersectionLightDirection));
            totalIntensity += intensity;

            /* Specular highlight at water surface */
            vec3 reflection = reflect(-lightDirection, transformedWaterNormal);
            float specularity = pow(max(0.0, dot(transformedCameraDirection, reflection)), 300.0);
            totalHighlight += vec3(specularity);
        }

        fragColor.rgb *= totalIntensity;
        fragColor.rgb += totalHighlight;

    /* Surface */
    } else {
        fragColor = texture(diffuseTexture, fragPosition.xz*10);

        float totalIntensity = 0.0;
        vec3 totalHighlight = vec3(0.0);
        vec3 transformedNormal = normalMatrix*vec3(0.0, 1.0, 0.0);
        for(int i = 0; i != POOL_LIGHT_COUNT; ++i) {
            vec3 lightDirection = normalize(light[i] - transformedFragPosition);

            /* Light intensity */
            float intensity = max(0.0, dot(transformedNormal, lightDirection));

            if(intensity > 0.0) {
                totalIntensity += intensity;

                /* Specular highlight */
                vec3 reflection = reflect(-lightDirection, transformedNormal);
                float specularity = pow(max(0.0, dot(transformedCameraDirection, reflection)), 80.0);
                totalHighlight += vec3(specularity);
            }
        }

        fragColor.rgb *= totalIntensity;
        fragColor.rgb += totalHighlight;
    }
}
