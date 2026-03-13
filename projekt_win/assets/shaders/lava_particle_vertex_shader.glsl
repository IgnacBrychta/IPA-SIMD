#version 330 core

layout (location = 0) in vec2 aCorner;
layout (location = 1) in vec3 aCenter;
layout (location = 2) in vec3 aRight;
layout (location = 3) in vec3 aUp;
layout (location = 4) in float aHalfSize;
layout (location = 5) in vec4 aColor;
layout (location = 6) in vec4 aShape;

out vec4 vColor;
out vec2 vLocal;
out vec4 vShape;
out float vHeightFactor;

uniform mat4 view;
uniform mat4 projection;
uniform float uTime;

void main() {
    float cornerTop = clamp((aCorner.y + 1.0) * 0.5, 0.0, 1.0);
    float heightFactor = clamp(aCenter.y * 0.055, 0.0, 1.0);

    float swayA = sin(uTime * 0.48 + aCenter.y * 0.16 + aShape.w * 13.0);
    float swayB = cos(uTime * 0.37 + aCenter.x * 0.09 + aShape.w * 19.0);
    float shear = (0.06 + 0.28 * heightFactor) * cornerTop;

    vec3 dynamicOffset = aRight * (swayA * shear * aHalfSize) +
                         aUp * (swayB * 0.08 * shear * aHalfSize);

    vec3 worldPos = aCenter + aRight * (aCorner.x * aHalfSize) + aUp * (aCorner.y * aHalfSize) + dynamicOffset;
    gl_Position = projection * view * vec4(worldPos, 1.0);

    vColor = aColor;
    vLocal = aCorner;
    vShape = aShape;
    vHeightFactor = heightFactor;
}
