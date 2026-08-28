#version 330 core


layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in float aMaterial;
layout(location = 4) in float aSkyLight;
layout(location = 5) in vec3 aBlockLight;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vViewPos;
out vec2 vUV;
out vec3 vViewNormal;
out float vMaterial;
out float vSkyLight;
out vec3 vBlockLight;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vec4 viewPos = uView * worldPos;
    gl_Position = uProjection * viewPos;

    vViewPos = viewPos.xyz;
    vUV = aUV;
    vViewNormal = mat3(uView) * aNormal;
    vMaterial = aMaterial;
    vSkyLight = aSkyLight;
    vBlockLight = aBlockLight;
}
