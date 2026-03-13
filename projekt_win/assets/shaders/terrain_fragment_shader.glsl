#version 330 core
// Fragment Shader for Terrain Rendering

in vec2 TexCoord;
in vec3 FragPosWorld;
in vec3 NormalWorld;

out vec4 FragColor;

uniform sampler2D terrainTexture; // Sampler for terrain color texture (no normal map for now)
uniform sampler2D heightMapTexture; // **New: Sampler for heightmap texture**

uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPosWorld;
uniform vec3 volcanoCenterWorld;
uniform float volcanoRadius;
uniform float volcanoHeat;

void main() {
    // 1. Sample textures
    vec3 albedoColor = texture(terrainTexture, TexCoord * 8.0).rgb;
    float heightValue = texture(heightMapTexture, TexCoord).r;

    // 2. Lighting calculations (Blinn-Phong - similar to ocean shader)
    vec3 normal = normalize(NormalWorld); // Use geometric normal for terrain
    vec3 lightDirNorm = normalize(lightDir);
    vec3 viewDirNorm = normalize(viewPosWorld - FragPosWorld);
    vec3 reflectDir = reflect(-lightDirNorm, normal);

    // Diffuse component
    float diff = max(dot(normal, lightDirNorm), 0.0);
    vec3 diffuse = diff * lightColor * albedoColor;

    // Specular component
    float spec = pow(max(dot(viewDirNorm, reflectDir), 0.0), 16.0); // Adjust shininess (exponent)
    vec3 specular = spec * lightColor * vec3(0.3); // Example specular color - less shiny than ocean

    // Ambient component
    vec3 ambient = 0.2 * lightColor * albedoColor;

    vec3 lowColor = vec3(0.78, 0.73, 0.64);
    vec3 highColor = vec3(0.42, 0.36, 0.30);
    vec3 heightTint = mix(lowColor, highColor, heightValue);


    // 4. Combine lighting components for final color
    vec3 litTerrain = (ambient + diffuse + specular) * albedoColor * heightTint;

    float distToVolcano = length(FragPosWorld.xz - volcanoCenterWorld.xz);
    float heightFromVolcanoBase = max(FragPosWorld.y - volcanoCenterWorld.y, 0.0);
    float coneHeightMask = smoothstep(1.5, 8.0, heightFromVolcanoBase);
    float lavaMask = 1.0 - smoothstep(volcanoRadius * 0.15, volcanoRadius, distToVolcano);
    lavaMask = pow(clamp(lavaMask, 0.0, 1.0), 2.4);
    lavaMask *= coneHeightMask;
    lavaMask *= clamp(volcanoHeat, 0.0, 1.0);

    vec3 lavaColor = mix(vec3(0.95, 0.18, 0.03), vec3(1.0, 0.75, 0.2), heightValue);
    vec3 finalColor = mix(litTerrain, litTerrain * 0.78 + lavaColor * 0.72, lavaMask);
    FragColor = vec4(finalColor, 1.0);
}
