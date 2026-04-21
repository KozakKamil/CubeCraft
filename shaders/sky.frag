#version 330 core

in vec3 vDir;
out vec4 FragColor;

uniform vec3 uSunDir;

void main() {
    vec3 dir = normalize(vDir);

    float t = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 horizonColor = vec3(0.75, 0.85, 0.95);
    vec3 zenithColor  = vec3(0.35, 0.55, 0.85);
    vec3 sky = mix(horizonColor, zenithColor, t);

    float sunDot = max(dot(dir, uSunDir), 0.0);
    float sunSpot = pow(sunDot, 128.0);
    float sunHalo = pow(sunDot, 8.0);
    sky = sky + vec3(1.0, 0.9, 0.7) * sunSpot * 0.6;
    sky = sky + vec3(1.0, 0.8, 0.5) * sunHalo * 0.15;

    FragColor = vec4(sky, 1.0);
}