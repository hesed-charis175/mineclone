#version 330 core

out float FragColor;
in vec2 vUV;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D uNoiseTex;

uniform vec3 uSamples[32];
uniform mat4 uProjection;

const int KERNEL_SIZE = 32;
const float RADIUS = 2.5;
const float BIAS = 0.05;

void main() {
    vec3 fragPos = texture(gPosition, vUV).xyz;
    vec3 normal = normalize(texture(gNormal, vUV).xyz);

    if (fragPos.z >= -0.0001) {
        FragColor = 1.0;
        return;
    }

    vec2 noiseScale = vec2(textureSize(gPosition, 0)) / 4.0;
    vec3 randomVec = normalize(texture(uNoiseTex, vUV * noiseScale).xyz);

    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);

    float occlusion = 0.0;
    for (int i = 0; i < KERNEL_SIZE; ++i) {
        vec3 samplePos = TBN * uSamples[i];
        samplePos = fragPos + samplePos * RADIUS;

        vec4 offset = uProjection * vec4(samplePos, 1.0);
        offset.xyz /= offset.w;
        offset.xyz = offset.xyz * 0.5 + 0.5; // NDC -> [0,1] texture space

        vec3 sampledGPos = texture(gPosition, offset.xy).xyz;
        float sampleDepth = sampledGPos.z;
        if (sampledGPos.z >= -0.0001)
            continue;
        float rangeCheck =
            smoothstep(0.0, 1.0, RADIUS / max(abs(fragPos.z - sampleDepth), 0.01));

        float occ = smoothstep(0.0, BIAS, sampleDepth - samplePos.z);
        occlusion += occ * rangeCheck;
    }

    FragColor = 1.0 - (occlusion / float(KERNEL_SIZE));
}
