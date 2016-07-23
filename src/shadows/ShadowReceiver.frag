uniform float shadowBias;
uniform sampler2DArrayShadow shadowmapTexture;
uniform highp vec3 lightDirection;

in mediump vec3 transformedNormal;
in highp vec3 shadowCoords[NUM_SHADOW_MAP_LEVELS];
uniform float shadowDepthSplits[NUM_SHADOW_MAP_LEVELS];

out lowp vec4 color;


void main() {
	//You might want to source this from a texture or a vertex colour
    vec3 albedo = vec3(0.5,0.5,0.5);
	//You might want to source this from a uniform
    vec3 ambient = vec3(0.5,0.5,0.5);

    mediump vec3 normalizedTransformedNormal = normalize(transformedNormal);

	float inverseShadow = 1.0;
	//Is the normal of this face pointing towards the light?
	lowp float intensity = dot(normalizedTransformedNormal, lightDirection);
	if (intensity <= 0) {
		//Pointing away from the light anyway, we know it's in the shade, don't bother shadow map lookup
		inverseShadow = 0.0f;
		intensity = 0.0f;
	}
	else {
		int shadowLevel = 0;
		bool inRange;
		//Starting with highest resolution shadow map, find one we're in range of
		for (; shadowLevel < NUM_SHADOW_MAP_LEVELS; shadowLevel++) {
			vec3 shadowCoord = shadowCoords[shadowLevel];
			inRange = shadowCoord.x >= 0 && shadowCoord.y >= 0 && shadowCoord.x < 1 && shadowCoord.y < 1 && shadowCoord.z >= 0 && shadowCoord.z < 1;
			if (inRange) {
				inverseShadow = texture(shadowmapTexture, vec4(shadowCoord.xy, shadowLevel, shadowCoord.z-shadowBias));
				break;
			}
		}
#ifdef DEBUG_SHADOWMAP_LEVELS
        switch (shadowLevel) {
            case 0: albedo *= vec3(1,0,0); break;
            case 1: albedo *= vec3(1,1,0); break;
            case 2: albedo *= vec3(0,1,0); break;
            case 3: albedo *= vec3(0,1,1); break;
            default: albedo *= vec3(1,0,1); break;
        }
#else
		if (!inRange) {
			// If your shadow maps don't cover your entire view, you might want to remove this
			albedo *= vec3(1,0,1); //Something has gone wrong - didn't find a shadow map
		}
#endif
	}

    color.rgb = ((ambient + vec3(intensity * inverseShadow)) * albedo);
    color.a = 1.0;
}

