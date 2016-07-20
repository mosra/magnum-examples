
uniform sampler2DArrayShadow shadowmapTexture;
uniform highp vec3 lightDirection;

in mediump vec3 transformedNormal;

in highp vec3 shadowCoord[NUM_SHADOW_MAP_LEVELS];
uniform float shadowDepthSplits[NUM_SHADOW_MAP_LEVELS];

out lowp vec4 color;

void main() {
    vec3 albedo = vec3(0.5,0.5,0.5);

    mediump vec3 normalizedTransformedNormal = normalize(transformedNormal);

	float shadow = 0.0;
	lowp float intensity = dot(normalizedTransformedNormal, lightDirection);
	int shadowLevel = 0;
	if (intensity <= 0) {
		shadow = 0.0f;
		intensity = 0.0f;
	}
	else {
		for (; shadowLevel < NUM_SHADOW_MAP_LEVELS; shadowLevel++) {
			bool inRange = shadowCoord[shadowLevel].x >= 0 && shadowCoord[shadowLevel].y >= 0 && shadowCoord[shadowLevel].x < 1 && shadowCoord[shadowLevel].y < 1;
			if (inRange) {
				float bias = 0.0015;
				shadow = texture(shadowmapTexture, vec4(shadowCoord[shadowLevel].xy, shadowLevel, (1-shadowCoord[shadowLevel].z)-bias));
				break;
			}
		}
	}

    vec3 ambient = vec3(0.5);
    color.rgb = ((ambient + vec3(intensity * shadow)) * albedo);
//    color.rgb = vec3(intensity);
//	color.rgb = shadowCoord[min(NUM_SHADOW_MAP_LEVELS, shadowLevel)] * (1+shadowLevel) / 5.0;
    color.a = 1.0;

#ifdef DEBUG_SHADOWMAP_LEVELS
    if (shadowLevel == 0) {
    	color.rgb *= vec3(1,0,0);
    }
    else if (shadowLevel == 1) {
    	color.rgb *= vec3(1,1,0);
    }
    else if (shadowLevel == 2) {
    	color.rgb *= vec3(0,1,0);
    }
    else if (shadowLevel == 3) {
    	color.rgb *= vec3(0,1,1);
    }
    else if (shadowLevel == 4) {
    	color.rgb *= vec3(0,0,1);
    }
    else {
    	color.rgb *= vec3(1,0,1);
    }
#endif
}

