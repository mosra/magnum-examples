uniform float shadowBias;
uniform sampler2DArrayShadow shadowmapTexture;

in mediump vec3 transformedNormal;
in highp vec3 interpolatedLightDirection;
in highp vec3 shadowCoords[NUM_SHADOW_MAP_LEVELS];
uniform float shadowDepthSplits[NUM_SHADOW_MAP_LEVELS];

out lowp vec4 color;


void main() {
	//You might want to source this from a texture or a vertex colour
    vec3 albedo = vec3(0.5,0.5,0.5);

    mediump vec3 normalizedTransformedNormal = normalize(transformedNormal);

	float inverseShadow = 1.0;
	//Is the normal of this face pointing towards the light?
	lowp float intensity = dot(normalizedTransformedNormal, normalize(interpolatedLightDirection));
	if (intensity <= 0) {
		//Pointing away from the light
		inverseShadow = 0.0f;
		intensity = 0.0f;
	}
	else {
		int shadowLevel = 0;
		for (; shadowLevel < NUM_SHADOW_MAP_LEVELS; shadowLevel++) {
			vec3 shadowCoord = shadowCoords[shadowLevel];
			bool inRange = shadowCoord.x >= 0 && shadowCoord.y >= 0 && shadowCoord.x < 1 && shadowCoord.y < 1 && shadowCoord.z >= 0 && shadowCoord.z < 1;
			if (inRange) {
				inverseShadow = texture(shadowmapTexture, vec4(shadowCoord.xy, shadowLevel, shadowCoord.z-shadowBias));
				break;
			}
		}
#ifdef DEBUG_SHADOWMAP_LEVELS
		if (shadowLevel == 0) {
			albedo *= vec3(1,0,0);
		}
		else if (shadowLevel == 1) {
			albedo *= vec3(1,1,0);
		}
		else if (shadowLevel == 2) {
			albedo *= vec3(0,1,0);
		}
		else if (shadowLevel == 3) {
			albedo *= vec3(0,1,1);
		}
		else if (shadowLevel == 4) {
			albedo *= vec3(0,0,1);
		}
		else {
			albedo *= vec3(1,0,1);
		}
#else
		if (shadowLevel == NUM_SHADOW_MAP_LEVELS) {
			albedo = vec3(0.5,0.0,0.5); //Something has gone wrong - didn't find a shadow map
		}
#endif
	}

    vec3 ambient = vec3(0.5);
    color.rgb = ((ambient + vec3(intensity * inverseShadow)) * albedo);
    color.a = 1.0;
}

