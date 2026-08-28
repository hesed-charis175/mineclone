#version 330 core

out vec4 FragColor;
in vec2 vUV;

uniform sampler2D gPosition;       
uniform sampler2D gNormal;        
uniform sampler2D gAlbedoEmissive;
uniform sampler2D gBlockLight;   
uniform sampler2D uSSAO;
uniform sampler2D uShadowMap;

uniform mat4 uInvView;  
uniform mat4 uLightSpaceMatrix;
uniform vec3 uLightDir;
uniform vec3 uLightDirView; 
                           
                          
uniform vec3 uLightColor;
uniform float uSkyLightFactor;

#define MAX_POINT_LIGHTS 4
uniform vec3 uPointLightPosView[MAX_POINT_LIGHTS];
uniform int uNumPointLights;
const float POINT_LIGHT_RANGE = 8.0;
const vec3 POINT_LIGHT_TINT = vec3(1.0, 0.93, 0.78);

const float MIN_AMBIENT = 0.07;

const float NIGHT_SKY_FLOOR = 0.15;
const float DAY_SKY_MAX = 0.60;
const float DIRECT_STRENGTH = 0.60;

float computeShadow(vec4 fragPosLightSpace, vec3 worldNormal) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;
    if (projCoords.z > 1.0) return 0.0;

    float currentDepth = projCoords.z;
    float bias = max(0.003 * (1.0 - dot(worldNormal, -uLightDir)), 0.0008);

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
    vec4 posSample = texture(gPosition, vUV);
    vec3 viewPos = posSample.xyz;

    if (viewPos.z >= -0.0001)
        discard;

    float skyLight = posSample.a;
    vec4 normalSample = texture(gNormal, vUV);
    vec3 viewNormal = normalize(normalSample.xyz);
    vec3 blockLight = texture(gBlockLight, vUV).rgb;

    vec4 albedoMaterial = texture(gAlbedoEmissive, vUV);
    vec3 albedo = albedoMaterial.rgb;
    float material = albedoMaterial.a;
    float ao = texture(uSSAO, vUV).r;

    vec3 lightingColor;
    vec3 specular = vec3(0.0);
    if (material > 0.9) {
        lightingColor = vec3(1.15);
    } else {
        vec3 worldPos = (uInvView * vec4(viewPos, 1.0)).xyz;
        vec3 worldNormal = normalize(mat3(uInvView) * viewNormal);

        vec4 fragPosLightSpace = uLightSpaceMatrix * vec4(worldPos, 1.0);
        float diffuse = max(dot(worldNormal, -uLightDir), 0.0);
        float shadow = computeShadow(fragPosLightSpace, worldNormal);

        float effectiveSky = skyLight * mix(NIGHT_SKY_FLOOR, DAY_SKY_MAX, uSkyLightFactor);
        vec3 ambient = max(max(vec3(effectiveSky), blockLight) * ao, vec3(MIN_AMBIENT));
        vec3 direct = uLightColor * diffuse * (1.0 - shadow) * DIRECT_STRENGTH;
        lightingColor = ambient + direct;

        if (material > 0.001) {
            vec3 viewDir = normalize(-viewPos);

            vec3 halfwayDir = normalize(-uLightDirView + viewDir);
            float specAngle = max(dot(viewNormal, halfwayDir), 0.0);
            float shininess = mix(20.0, 130.0, material);
            float specFactor = pow(specAngle, shininess);
            specular = uLightColor * specFactor * material * (1.0 - shadow) * 2.2;

            for (int i = 0; i < uNumPointLights; ++i) {
                vec3 toLight = uPointLightPosView[i] - viewPos;
                float dist = length(toLight);
                if (dist >= POINT_LIGHT_RANGE) continue;
                vec3 pointLightDir = toLight / max(dist, 0.001);
                vec3 pointHalfway = normalize(pointLightDir + viewDir);
                float pointSpecAngle = max(dot(viewNormal, pointHalfway), 0.0);
                float pointSpecFactor = pow(pointSpecAngle, shininess);
                float attenuation = 1.0 - dist / POINT_LIGHT_RANGE;
                attenuation *= attenuation;
                specular += POINT_LIGHT_TINT * pointSpecFactor * material * attenuation * 3.0;
            }
        }
    }

    FragColor = vec4(albedo * lightingColor + specular, 1.0);
}
