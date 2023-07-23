#version 450

layout (location = 0) in vec2 TexCoord;
layout (location = 0) out vec4 color;

float sdfSphere(vec3 pos, float radius) {
    return length(pos) - radius;
}

vec4 rayMarch(vec3 ro, vec3 rd) {
    float totalDist = 0.0;
    vec3 pos;

    for (int i = 0; i < 1000; i++) {
        pos = ro + totalDist * rd;
        float dist = sdfSphere(pos, 0.2);

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
