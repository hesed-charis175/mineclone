#version 330 core

in vec2 vUV;
in vec3 vNormal;
in vec4 vFragPosLightSpace;
in float vEmissive;
in float vSkyLight;
in vec3 vBlockLight;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform sampler2D uShadowMap;
uniform vec3 uLightDir;   
uniform vec3 uLightColor; 
uniform float uSkyLightFactor; 

const float MIN_AMBIENT = 0.08; 
const float NIGHT_SKY_FLOOR = 0.15;
const float DAY_SKY_MAX = 0.35;
const float DIRECT_STRENGTH = 0.45;

float computeShadow(vec4 fragPosLightSpace, vec3 normal) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;

    float currentDepth = projCoords.z;

       float bias = max(0.003 * (1.0 - dot(normal, -uLightDir)), 0.0008);

    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(uShadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(uShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias > pcfDepth) ? 1.0 : 0.0;
        }
    }
    return shadow / 9.0;
}

void main() {
    vec4 texColor = texture(uTexture, vUV);
    vec3 normal = normalize(vNormal);

    vec3 lighting;
    if (vEmissive > 0.5) {
            lighting = vec3(1.15);
    } else {
        float diffuse = max(dot(normal, -uLightDir), 0.0);
        float shadow = computeShadow(vFragPosLightSpace, normal);
         float effectiveSky = vSkyLight * mix(NIGHT_SKY_FLOOR, DAY_SKY_MAX, uSkyLightFactor);
              vec3 ambient = max(max(vec3(effectiveSky), vBlockLight), vec3(MIN_AMBIENT));
        lighting = ambient + uLightColor * diffuse * (1.0 - shadow) * DIRECT_STRENGTH;
    }

    FragColor = vec4(texColor.rgb * lighting, texColor.a);
}
