#version 450

layout (push_constant) uniform PushConstants {
    float iTime;
} pc;
layout (location = 0) in vec2 TexCoord;
layout (location = 0) out vec4 color;

float sdfSphere(vec3 pos, float radius) {
    return length(pos) - radius;
}

float map (vec3 pos) {
    vec3 q = vec3(pc.iTime * 0.01, 0.0, 0.0);
    float d = sdfSphere(pos + q, 0.2);
    return d;
}

vec4 rayMarch(vec3 ro, vec3 rd) {
    float totalDist = 0.0;
    vec3 pos;

    for (int i = 0; i < 100; i++) {
        pos = ro + totalDist * rd;
        float dist = map(pos);

        if (dist < 0.01) {
            vec3 sphereCol = vec3(0.8, 0.6, 0.7);
            return vec4(sphereCol - totalDist, 1.0); // sphere colour
        }
        if (dist > 20.0) {
            break;
        }
        totalDist += dist;
    }
    return vec4(0.3); // background colour
}


void main()
{
    // normalize to [-1, 1]
    vec2 uv = TexCoord * 2.0 - 1.0;
    uv.x *= 1.4;

    // Camera settings
    vec3 cameraPos = vec3(0.0, -0.0, 0.5);

    // Create ray direction from screen coordinates
    vec3 rayDir = normalize(vec3(uv, -0.8));
    // vec3 rayDir = vec3(uv, 0.7);

    color = rayMarch(cameraPos, rayDir);
    // color = vec4(uv, 0.0, 1.0);
}
