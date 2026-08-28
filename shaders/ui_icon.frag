#version 330 core

in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform bool uUseSolidColor; 
uniform vec3 uSolidColor;

void main() {
    if (uUseSolidColor) {
        FragColor = vec4(uSolidColor, 1.0);
        return;
    }
    vec4 texColor = texture(uTexture, vUV);
    FragColor = vec4(texColor.rgb, 1.0);
}
