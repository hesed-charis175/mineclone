#version 330 core

in vec3 vViewPos;
in vec2 vUV;
in vec3 vViewNormal;
in float vMaterial;
in float vSkyLight;
in vec3 vBlockLight;

layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedoEmissive;
layout(location = 3) out vec4 gBlockLight;

uniform sampler2D uTexture;

void main() {
    vec4 texColor = texture(uTexture, vUV);
     if (texColor.a < 0.5)
        discard;

    gPosition = vec4(vViewPos, vSkyLight);
    gNormal = vec4(normalize(vViewNormal), 1.0);
    gAlbedoEmissive = vec4(texColor.rgb, vMaterial);
    gBlockLight = vec4(vBlockLight, 1.0);
}
