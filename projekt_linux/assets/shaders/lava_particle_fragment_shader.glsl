#version 330 core

in vec4 vColor;
in vec2 vLocal;
in vec4 vShape;
in float vHeightFactor;
out vec4 FragColor;

float hash12(vec2 p) {
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

void main() {
    float c = cos(vShape.z);
    float s = sin(vShape.z);
    mat2 rot = mat2(c, -s, s, c);
    vec2 p = rot * vLocal;
    p /= max(vShape.xy, vec2(0.3));

    float angle = atan(p.y, p.x);
    float radius = length(p);
    float wave = 1.0 + vShape.w * (0.45 * sin(3.0 * angle) + 0.35 * cos(5.0 * angle));
    float envelope = radius / max(wave, 0.35);
    if (envelope > 1.0) {
        discard;
    }

    float shell = smoothstep(1.0, 0.06, envelope);
    float core = smoothstep(0.66, 0.0, envelope);
    float edge = smoothstep(0.98, 0.55, envelope) * (1.0 - core);

    float noise1 = hash12(p * 6.0 + vec2(vShape.w * 13.7, vShape.z));
    float noise2 = hash12(p * 11.0 + vec2(vShape.z * 0.3, vShape.w * 19.1));
    float densityNoise = mix(noise1, noise2, 0.5);
    float density = mix(0.70, 1.18, densityNoise);

    vec3 lightDir = normalize(vec3(-0.35, 0.68, 0.64));
    vec3 normal = normalize(vec3(p * 0.9, sqrt(max(0.0, 1.0 - clamp(envelope * envelope, 0.0, 1.0)))));
    float diff = max(dot(normal, lightDir), 0.0);

    float ashLift = mix(0.92, 1.10, vHeightFactor);
    vec3 smokeLit = vColor.rgb * (0.52 + 0.30 * diff) * ashLift;
    vec3 emberTint = vec3(0.95, 0.40, 0.08) * core * (1.0 - vHeightFactor) * 0.35;
    vec3 rimCool = vec3(0.10, 0.11, 0.12) * edge * 0.30;

    float alpha = vColor.a * shell * density;
    alpha *= mix(1.05, 0.70, vHeightFactor);
    alpha = clamp(alpha, 0.0, 1.0);
    if (alpha < 0.01) {
        discard;
    }

    FragColor = vec4(smokeLit + emberTint - rimCool, alpha);
}
