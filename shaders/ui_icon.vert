#version 330 core

layout(location = 0) in vec2 aLocalPos;
layout(location = 1) in vec2 aUV;     

uniform vec2 uCenter;    
uniform vec2 uHalfSize; 
uniform vec4 uTileUV;  

out vec2 vUV;

void main() {
    vec2 offset = (aLocalPos * 2.0 - 1.0) * uHalfSize;
    gl_Position = vec4(uCenter + offset, 0.0, 1.0);
    vUV = mix(uTileUV.xy, uTileUV.zw, aUV);
}
