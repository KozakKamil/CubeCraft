#version 330 core

in vec2 vUV;
in vec3 vNormal;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform vec3 uLightDir;   // kierunek w ktorym swiatlo pada (znormalizowany)
uniform float uAmbient;   // bazowe swiatlo (np. 0.3)

void main() {
    vec4 texColor = texture(uTexture, vUV);

    // Lambert diffuse: im bardziej sciana patrzy w strone -lightDir, tym jasniejsza
    float diff = max(dot(normalize(vNormal), -uLightDir), 0.0);

    float lightIntensity = uAmbient + (1.0 - uAmbient) * diff;

    FragColor = vec4(texColor.rgb * lightIntensity, texColor.a);
}