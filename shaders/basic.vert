#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in float aEmissive;
layout(location = 4) in float aSkyLight;
layout(location = 5) in vec3 aBlockLight;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uLightSpaceMatrix;

out vec2 vUV;
out vec3 vNormal;
out vec4 vFragPosLightSpace;
out float vEmissive;
out float vSkyLight;
out vec3 vBlockLight;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    gl_Position = uProjection * uView * worldPos;

    vUV = aUV;
    vNormal = mat3(uModel) * aNormal;
    vFragPosLightSpace = uLightSpaceMatrix * worldPos;
    vEmissive = aEmissive;
    vSkyLight = aSkyLight;
    vBlockLight = aBlockLight;
}
