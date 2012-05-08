#version 330

uniform sampler2D diffuseTexture;
uniform vec3 cameraDirection;

in vec3 fragPosition;

out vec4 fragColor;

void main() {
    /* Direction from camera to fragment */
    vec3 direction = cameraDirection+fragPosition;

    vec3 intersection = vec3(0.0);
    vec2 texCoord = vec2(0.0);

    /* Near */
    if(direction.z > 0 && (intersection = fragPosition + (direction/direction.z)*(1-fragPosition.z)).z > abs(intersection.x))
        texCoord = intersection.xy;

    /* Far */
    else if(direction.z <= 0 && -(intersection = fragPosition + (direction/-direction.z)*(1+fragPosition.z)).z >= abs(intersection.x))
        texCoord = intersection.xy;

    /* Right */
    else if(direction.x > 0 && (intersection = fragPosition + (direction/direction.x)*(1-fragPosition.x)).x > abs(intersection.z))
        texCoord = intersection.zy;

    /* Left */
    else if(direction.x <= 0 && -(intersection = fragPosition + (direction/-direction.x)*(1+fragPosition.x)).x >= abs(intersection.z))
        texCoord = intersection.zy;

    texCoord = (texCoord+vec2(1.0))*10;

    fragColor = texture(diffuseTexture, texCoord);
}
