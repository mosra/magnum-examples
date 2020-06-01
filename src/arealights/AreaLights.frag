/*
    This file is part of Magnum.

    Original authors — credit is appreciated but not required:

        2010, 2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018, 2019, 2020 —
            Vladimír Vondruš <mosra@centrum.cz>
        2017 — Jonathan Hale <squareys@googlemail.com>, based on "Real-Time
            Polygonal-Light Shading with Linearly Transformed Cosines", by Eric
            Heitz et al, https://eheitzresearch.wordpress.com/415-2/

    This is free and unencumbered software released into the public domain.

    Anyone is free to copy, modify, publish, use, compile, sell, or distribute
    this software, either in source code form or as a compiled binary, for any
    purpose, commercial or non-commercial, and by any means.

    In jurisdictions that recognize copyright laws, the author or authors of
    this software dedicate any and all copyright interest in the software to
    the public domain. We make this dedication for the benefit of the public
    at large and to the detriment of our heirs and successors. We intend this
    dedication to be an overt act of relinquishment in perpetuity of all
    present and future rights to this software under copyright law.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
    THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

in vec4 v_position;
in vec3 v_normal;

uniform vec3 u_viewPosition;

uniform vec3 u_baseColor;
uniform float u_metalness;
uniform float u_roughness;
uniform float u_f0;

uniform float u_lightIntensity;
uniform float u_twoSided;
uniform vec3 u_quadPoints[4];

layout(binding = 0) uniform sampler2D s_texLTCMat;
layout(binding = 1) uniform sampler2D s_texLTCAmp;

#define M_PI 3.14159265359
#define LUT_SIZE 32.0

out vec4 fragmentColor;

/* Get uv coordinates into LTC lookup texture */
vec2 ltcCoords(float cosTheta, float roughness) {
    const float theta = acos(cosTheta);
    vec2 coords = vec2(roughness, theta/(0.5*M_PI));

    /* Scale and bias coordinates, for correct filtered lookup */
    coords = coords*(LUT_SIZE - 1.0)/LUT_SIZE + 0.5/LUT_SIZE;

    return coords;
}

/** Get inverse matrix from LTC lookup texture */
mat3 ltcMatrix(sampler2D tex, vec2 coord) {
    const vec4 t = texture(tex, coord);
    mat3 Minv = mat3(
        vec3(  1,   0, t.y),
        vec3(  0, t.z,   0),
        vec3(t.w,   0, t.x)
    );

    return Minv;
}

/* Integrate between two edges on a clamped cosine distribution */
float integrateEdge(vec3 v1, vec3 v2) {
    float cosTheta = dot(v1, v2);
    cosTheta = clamp(cosTheta, -0.9999, 0.9999);

    const float theta = acos(cosTheta);
    /* For theta <= 0.001 `theta/sin(theta)` is approximated as 1.0 */
    const float res = cross(v1, v2).z*((theta > 0.001) ? theta/sin(theta) : 1.0);
    return res;
}

int clipQuadToHorizon(inout vec3 L[5]) {
    /* Detect clipping config */
    int config = 0;
    if(L[0].z > 0.0) config += 1;
    if(L[1].z > 0.0) config += 2;
    if(L[2].z > 0.0) config += 4;
    if(L[3].z > 0.0) config += 8;

    int n = 0;

    if(config == 0) {
        // clip all
    } else if(config == 1) { // V1 clip V2 V3 V4
        n = 3;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[3].z * L[0] + L[0].z * L[3];
    } else if(config == 2) { // V2 clip V1 V3 V4
        n = 3;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    } else if(config == 3) { // V1 V2 clip V3 V4
        n = 4;
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
        L[3] = -L[3].z * L[0] + L[0].z * L[3];
    } else if(config == 4) { // V3 clip V1 V2 V4
        n = 3;
        L[0] = -L[3].z * L[2] + L[2].z * L[3];
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
    } else if(config == 5) { // V1 V3 clip V2 V4, impossible
        n = 0;
    } else if(config == 6) { // V2 V3 clip V1 V4
        n = 4;
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    } else if(config == 7) { // V1 V2 V3 clip V4
        n = 5;
        L[4] = -L[3].z * L[0] + L[0].z * L[3];
        L[3] = -L[3].z * L[2] + L[2].z * L[3];
    } else if(config == 8) { // V4 clip V1 V2 V3
        n = 3;
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
        L[1] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] =  L[3];
    } else if(config == 9) { // V1 V4 clip V2 V3
        n = 4;
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
        L[2] = -L[2].z * L[3] + L[3].z * L[2];
    } else if(config == 10) { // V2 V4 clip V1 V3, impossible
        n = 0;
    } else if(config == 11) { // V1 V2 V4 clip V3
        n = 5;
        L[4] = L[3];
        L[3] = -L[2].z * L[3] + L[3].z * L[2];
        L[2] = -L[2].z * L[1] + L[1].z * L[2];
    } else if(config == 12) { // V3 V4 clip V1 V2
        n = 4;
        L[1] = -L[1].z * L[2] + L[2].z * L[1];
        L[0] = -L[0].z * L[3] + L[3].z * L[0];
    } else if(config == 13) { // V1 V3 V4 clip V2
        n = 5;
        L[4] = L[3];
        L[3] = L[2];
        L[2] = -L[1].z * L[2] + L[2].z * L[1];
        L[1] = -L[1].z * L[0] + L[0].z * L[1];
    } else if(config == 14) { // V2 V3 V4 clip V1
        n = 5;
        L[4] = -L[0].z * L[3] + L[3].z * L[0];
        L[0] = -L[0].z * L[1] + L[1].z * L[0];
    } else if(config == 15) { // V1 V2 V3 V4
        n = 4;
    }

    if(n == 3)
        L[3] = L[0];
    if(n == 4)
        L[4] = L[0];

    return n;
}

/*
 * Get intensity of light from the arealight given by `points` at the point `P`
 * with normal `N` when viewed from direction `P`.
 * @param N Normal
 * @param V View Direction
 * @param P Vertex Position
 * @param Minv Matrix to transform from BRDF distribution to clamped cosine distribution
 * @param points Light quad vertices
 * @param twoSided Whether the light is two sided
 */
float ltcEvaluate(vec3 N, vec3 V, vec3 P, mat3 Minv, vec3 points[4], bool twoSided) {
    /* Construct orthonormal basis around N */
    const vec3 T1 = normalize(V - N*dot(V, N));
    const vec3 T2 = cross(N, T1);

    /* Rotate area light in (T1, T2, R) basis */
    Minv = Minv*transpose(mat3(T1, T2, N));

    /* Allocate 5 vertices for polygon (one additional which may result from
     * clipping) */
    vec3 L[5];
    L[0] = Minv*(points[0] - P);
    L[1] = Minv*(points[1] - P);
    L[2] = Minv*(points[2] - P);
    L[3] = Minv*(points[3] - P);

    /* Clip light quad so that the part behind the surface does not affect the
     * lighting of the point */
    int n = clipQuadToHorizon(L);
    if(n == 0)
        return 0.0;

    // project onto sphere
    L[0] = normalize(L[0]);
    L[1] = normalize(L[1]);
    L[2] = normalize(L[2]);
    L[3] = normalize(L[3]);
    L[4] = normalize(L[4]);

    /* Integrate over the clamped cosine distribution in the domain of the
     * transformed light polygon */
    float sum = integrateEdge(L[0], L[1])
              + integrateEdge(L[1], L[2])
              + integrateEdge(L[2], L[3]);
    if(n >= 4)
        sum += integrateEdge(L[3], L[4]);
    if(n == 5)
        sum += integrateEdge(L[4], L[0]);

    /* Negated due to winding order */
    sum = twoSided ? abs(sum) : max(0.0, -sum);

    return sum;
}

void main() {
    const vec3 pos = v_position.xyz;
    const vec3 viewDir = normalize(u_viewPosition - pos);

    const vec3 diffColor = u_baseColor*(1.0 - u_metalness);
    const vec3 specColor = mix(vec3(u_f0, u_f0, u_f0), u_baseColor, u_metalness);

    /* Create coords into LTC LUT, get inverse matrix from texture and compute radiance of light */
    const vec2 coords = ltcCoords(dot(viewDir, v_normal), u_roughness);

    /* Get matrix to transform light polygon into a clamped cosine distribution space */
    const mat3 invMat = ltcMatrix(s_texLTCMat, coords);
    float Lo_i = ltcEvaluate(v_normal, viewDir, pos, invMat, u_quadPoints, u_twoSided > 0.0);

    /* Apply BRDF scale terms (BRDF magnitude and Schlick Fresnel) */
    const vec2 schlick = texture(s_texLTCAmp, coords).xy;
    vec3 color = (u_lightIntensity*Lo_i)*(specColor*schlick.x + (1.0 - specColor)*schlick.y);

    /* Normalize */
    color /= 2.0f * M_PI;

    fragmentColor = vec4(color, 1.0);
}
