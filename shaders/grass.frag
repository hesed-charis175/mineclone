#version 330 core

in vec2 vUV;
in vec3 vTint;
out vec4 FragColor;

uniform sampler2D uTexture;

uniform vec3 uAmbientColor;

void main() {
    vec4 texColor = texture(uTexture, vUV);
    if (texColor.a < 0.5)
        discard;

    FragColor = vec4(texColor.rgb * vTint * uAmbientColor, 1.0);
}
