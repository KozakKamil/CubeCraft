#version 330 core

in vec2 vUV;
in vec3 vNormal;
in vec3 vWorldPos;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec3 uLightDir;
uniform float uAmbient;
uniform vec3 uCameraPos;
uniform vec3 uFogColor;
uniform float uFogStart;
uniform float uFogEnd;

void main() {
    vec4 texColor = texture(uTexture, vUV);

    // Oswietlenie
    float diff = max(dot(normalize(vNormal), -uLightDir), 0.0);
    float lightIntensity = uAmbient + (1.0 - uAmbient) * diff;
    vec3 litColor = texColor.rgb * lightIntensity;

    // Mgla
    float dist = length(vWorldPos - uCameraPos);
    float fogFactor = clamp((dist - uFogStart) / (uFogEnd - uFogStart), 0.0, 1.0);
    vec3 finalColor = mix(litColor, uFogColor, fogFactor);

    FragColor = vec4(finalColor, texColor.a);
}