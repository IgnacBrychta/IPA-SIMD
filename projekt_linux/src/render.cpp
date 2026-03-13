/*
 * File:        render.cpp
 * Author:      Tomas Goldmann
 * Date:        2025-03-23
 * Description: Game renderer
 *
 * Copyright (c) 2025, Brno University of Technology. All rights reserved.
 * Licensed under the MIT.
 */



#include "Renderer.h"
#include <iostream>
#if defined(USE_SOIL2)
#include <SOIL2/SOIL2.h>
#elif defined(USE_SOIL)
#include <SOIL/SOIL.h>
#else
#error "No supported SOIL variant selected by build system."
#endif
#include "utils.h"
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <limits>
#include <sstream>
#include <iomanip>
#include <vector>




namespace {
struct LavaParticleInstance {
    glm::vec3 center;
    glm::vec3 right;
    glm::vec3 up;
    float halfSize;
    glm::vec4 color;
    glm::vec4 shape;
};

static inline float hash01(uint32_t x) {
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return static_cast<float>(x & 0x00FFFFFFu) / 16777215.0f;
}
}

Renderer::Renderer()
    : oceanTextureID(0),
      boatTextureID(0),
      terrainTextureID(0),
      heightMapTextureID(0),
      lavaParticleVAO(0),
      lavaParticleMeshVBO(0),
      lavaParticleInstanceVBO(0) {}

Renderer::~Renderer() {}

bool Renderer::init()
{
    glewInit();
    if (glewIsSupported("GL_VERSION_3_0"))
    {
        std::cout << "OpenGL 3.0 or higher supported: Shaders will work." << std::endl;
    }
    else
    {
        std::cout << "OpenGL 3.0 or higher NOT supported: Shaders might not work." << std::endl;
    }

    glClearColor(0.529f, 0.808f, 0.922f, 1.0f); // Light blue background

    if (!loadTexture("assets/textures/ocean_texture.png", oceanTextureID))
    {
        std::cerr << "Error loading ocean texture." << std::endl;
        return false;
    }
    if (!loadTexture("assets/textures/rock_texture.png", terrainTextureID)) { // Assuming you name it terrain_texture.jpg
        std::cerr << "Error loading terrain texture." << std::endl;
        return false;
    }
    checkGLError("loadTexture - terrainTexture"); // Check after terrain texture loading

    // **Load heightmap texture:**
    if (!loadTexture("assets/textures/volcano.png", heightMapTextureID)) {
        std::cerr << "Error loading terrain heightmap texture." << std::endl;
        return false;
    }
    checkGLError("loadTexture - heightMapTexture"); // Check after heightmap texture loading

    terrainShader.loadShader("assets/shaders/terrain_vertex_shader.glsl", "assets/shaders/terrain_fragment_shader.glsl");
    if (!terrainShader.isLoaded()) {
        std::cerr << "Error loading terrain shader program!" << std::endl;
        return false;
    }
    checkGLError("terrainShader.loadShader"); // Check after shader loading
    // Load and compile ocean shader program:
    oceanShader.loadShader("assets/shaders/ocean_vertex_shader.glsl", "assets/shaders/ocean_fragment_shader.glsl");
    if (!oceanShader.isLoaded()) {
        std::cerr << "Error loading ocean shader program!" << std::endl;
        return false;
    }
    checkGLError("oceanShader.loadShader"); // Check after shader loading
    lavaParticleShader.loadShader("assets/shaders/lava_particle_vertex_shader.glsl", "assets/shaders/lava_particle_fragment_shader.glsl");
    if (!lavaParticleShader.isLoaded()) {
        std::cerr << "Error loading lava particle shader program!" << std::endl;
        return false;
    }
    checkGLError("lavaParticleShader.loadShader");

    glGenVertexArrays(1, &lavaParticleVAO);
    glBindVertexArray(lavaParticleVAO);
    glGenBuffers(1, &lavaParticleMeshVBO);
    glBindBuffer(GL_ARRAY_BUFFER, lavaParticleMeshVBO);
    const float quadCorners[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadCorners), quadCorners, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &lavaParticleInstanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, lavaParticleInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(LavaParticleInstance), nullptr, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(LavaParticleInstance), reinterpret_cast<void*>(offsetof(LavaParticleInstance, center)));
    glEnableVertexAttribArray(1);
    glVertexAttribDivisor(1, 1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(LavaParticleInstance), reinterpret_cast<void*>(offsetof(LavaParticleInstance, right)));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(LavaParticleInstance), reinterpret_cast<void*>(offsetof(LavaParticleInstance, up)));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(LavaParticleInstance), reinterpret_cast<void*>(offsetof(LavaParticleInstance, halfSize)));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(LavaParticleInstance), reinterpret_cast<void*>(offsetof(LavaParticleInstance, color)));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(LavaParticleInstance), reinterpret_cast<void*>(offsetof(LavaParticleInstance, shape)));
    glEnableVertexAttribArray(6);
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    // Boat texture is now loaded in drawBoat, based on Boat class texture path

    setupLighting();
    // shaderProgram.loadShader("assets/shaders/vertex_shader.glsl", "assets/shaders/fragment_shader.glsl"); // Optional shader loading

    return true;
}

void Renderer::cleanup()
{
    glDeleteTextures(1, &oceanTextureID);
    glDeleteTextures(1, &boatTextureID);
    glDeleteTextures(1, &terrainTextureID);
    glDeleteTextures(1, &heightMapTextureID);
    if (lavaParticleInstanceVBO != 0) {
        glDeleteBuffers(1, &lavaParticleInstanceVBO);
        lavaParticleInstanceVBO = 0;
    }
    if (lavaParticleMeshVBO != 0) {
        glDeleteBuffers(1, &lavaParticleMeshVBO);
        lavaParticleMeshVBO = 0;
    }
    if (lavaParticleVAO != 0) {
        glDeleteVertexArrays(1, &lavaParticleVAO);
        lavaParticleVAO = 0;
    }

    // shaderProgram.cleanup(); // Optional shader cleanup
}

void Renderer::renderScene(const Ocean &ocean, const Boat &boat, const Camera &camera, const Terrain &terrain, const std::vector<VolcanoSimulation>& volcanoes)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    camera.lookAt(); // Set up camera view

    setupLighting(); // Ensure lighting is enabled each frame
    drawTerrain(terrain, camera, volcanoes); // **Call drawTerrain here - BEFORE drawOcean**
    for (const VolcanoSimulation& volcano : volcanoes) {
        drawVolcano(volcano, camera);
    }

    drawOcean(ocean, camera);
    drawBoat(boat);
    drawHud(boat, camera);
    checkGLError("drawTerrain"); // Check after drawTerrain
    // Optional: Render skybox, UI, etc.
    // ...
}

void Renderer::drawText2D(float x, float y, const std::string& text, void* font) {
    glRasterPos2f(x, y);
    for (const char ch : text) {
        glutBitmapCharacter(font, static_cast<int>(ch));
    }
}

void Renderer::drawHud(const Boat& boat, const Camera& camera) {
    const int width = glutGet(GLUT_WINDOW_WIDTH);
    const int height = glutGet(GLUT_WINDOW_HEIGHT);
    if (width <= 0 || height <= 0) {
        return;
    }

    const GLboolean depthEnabled = glIsEnabled(GL_DEPTH_TEST);
    const GLboolean lightingEnabled = glIsEnabled(GL_LIGHTING);
    const GLboolean textureEnabled = glIsEnabled(GL_TEXTURE_2D);
    const GLboolean blendEnabled = glIsEnabled(GL_BLEND);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0.0, static_cast<double>(width), 0.0, static_cast<double>(height));

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glColor3f(1.0f, 1.0f, 1.0f);

    std::ostringstream speedText;
    speedText << std::fixed << std::setprecision(2) << "Boat speed: " << boat.getSpeed();
    drawText2D(20.0f, static_cast<float>(height - 28), speedText.str());

    const std::string cameraModeText = std::string("Camera mode: ") + camera.getModeName();
    drawText2D(20.0f, static_cast<float>(height - 52), cameraModeText);

    if (camera.isBoatMode() && boat.hasTerrainCollision() && boat.canDisembark()) {
        drawText2D(20.0f, static_cast<float>(height - 76), "Press E to disembark");
    }
    if (camera.isOnFootMode()) {
        const glm::vec3 camPos = camera.getPosition();
        const glm::vec3 boatPos = boat.getPosition();
        const float dx = camPos.x - boatPos.x;
        const float dz = camPos.z - boatPos.z;
        const float dy = std::abs(camPos.y - boatPos.y);
        const float distXZ2 = dx * dx + dz * dz;
        const float dist3D2 = dx * dx + dy * dy + dz * dz;
        constexpr float kBoardRadius = 6.0f;
        constexpr float kMaxHeightDelta = 4.5f;
        constexpr float kBoardDistance3D = 6.5f;
        const bool closeInXZ = distXZ2 <= kBoardRadius * kBoardRadius && dy <= kMaxHeightDelta;
        const bool closeIn3D = dist3D2 <= kBoardDistance3D * kBoardDistance3D;
        if (closeInXZ || closeIn3D) {
            drawText2D(20.0f, static_cast<float>(height - 76), "Press E to board boat");
        }
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    if (blendEnabled) {
        glEnable(GL_BLEND);
    }
    if (textureEnabled) {
        glEnable(GL_TEXTURE_2D);
    }
    if (lightingEnabled) {
        glEnable(GL_LIGHTING);
    }
    if (depthEnabled) {
        glEnable(GL_DEPTH_TEST);
    }
}

void Renderer::drawVolcano(const VolcanoSimulation& volcano, const Camera& camera) {
    const VolcanoSimulation::RenderConfig cfg = volcano.getRenderConfig();
    const glm::vec3 volcanoCenter(cfg.worldX, cfg.worldY, cfg.worldZ);
    const float coneHeight = cfg.coneHeight;

    const float simHeat = std::clamp((volcano.getAverageTemperature() - 300.0f) / 950.0f, 0.0f, 1.0f);
    const float simLava = std::clamp(volcano.getAverageLavaHeight() * 15.0f, 0.0f, 1.0f);
    const float lavaStrength = std::clamp(0.25f + 0.75f * std::max(simHeat, simLava), 0.0f, 1.0f);
    const float timeSec = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.001f;
    const float emitterBaseWorldY = volcanoCenter.y + coneHeight + 0.25f;
    const glm::vec3 ventWorld(volcanoCenter.x, emitterBaseWorldY + 0.35f, volcanoCenter.z);

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    const int sparkCount = 14 + static_cast<int>(18.0f * lavaStrength);
    glBegin(GL_LINES);
    for (int s = 0; s < sparkCount; ++s) {
        const uint32_t seed0 = 0x9e3779b9u * static_cast<uint32_t>(s + 1);
        const uint32_t seed1 = seed0 ^ 0x85ebca6bu;
        const uint32_t seed2 = seed0 ^ 0xc2b2ae35u;

        const float age = std::fmod(timeSec * (0.55f + 0.85f * hash01(seed0)) + 3.0f * hash01(seed1), 1.0f);
        const float alive = 1.0f - age;
        if (alive <= 0.0f) {
            continue;
        }

        const float ang = 6.28318530718f * hash01(seed2);
        const float radial = (0.05f + 0.26f * hash01(seed1 ^ 0x27d4eb2fu)) * (0.35f + 0.65f * alive);
        const float rise = age * (2.1f + 3.7f * lavaStrength);
        const float drift = 0.25f * std::sin(timeSec * 2.1f + static_cast<float>(s) * 0.9f) * age;

        const float x0 = ventWorld.x + std::cos(ang) * radial + drift;
        const float z0 = ventWorld.z + std::sin(ang) * radial + 0.6f * drift;
        const float y0 = ventWorld.y + 0.12f + rise;

        const float x1 = x0 + 0.06f * std::cos(ang);
        const float z1 = z0 + 0.06f * std::sin(ang);
        const float y1 = y0 + 0.15f + 0.35f * alive;

        const float sparkAlpha = std::clamp(0.22f + 0.70f * alive, 0.10f, 0.92f);
        glColor4f(1.0f, 0.75f, 0.24f, sparkAlpha);
        glVertex3f(x0, y0, z0);
        glColor4f(1.0f, 0.24f, 0.08f, 0.02f + 0.24f * alive);
        glVertex3f(x1, y1, z1);
    }
    glEnd();

    const std::size_t particleCount = volcano.getParticleCount();
    const float* px = volcano.getParticlePosX();
    const float* py = volcano.getParticlePosY();
    const float* pz = volcano.getParticlePosZ();
    const float* pt = volcano.getParticleTemperature();

    if (px != nullptr && py != nullptr && pz != nullptr && pt != nullptr && particleCount > 0) {
        const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
        const glm::vec3 cameraPos = camera.getPosition();
        const std::size_t stride = 3;
        std::vector<LavaParticleInstance> instances;
        instances.reserve(particleCount / stride + 1);
        for (std::size_t i = 0; i < particleCount; i += stride) {
            if (!std::isfinite(px[i]) || !std::isfinite(py[i]) || !std::isfinite(pz[i]) || !std::isfinite(pt[i])) {
                continue;
            }

            const glm::vec3 center(
                volcanoCenter.x + px[i] * cfg.particleSpreadScale,
                emitterBaseWorldY + py[i],
                volcanoCenter.z + pz[i] * cfg.particleSpreadScale);
            if (!std::isfinite(center.x) || !std::isfinite(center.y) || !std::isfinite(center.z)) {
                continue;
            }

            glm::vec3 viewDir = cameraPos - center;
            const float viewLen = glm::length(viewDir);
            if (viewLen < 1.0e-5f) {
                continue;
            }
            viewDir /= viewLen;

            glm::vec3 right = glm::cross(worldUp, viewDir);
            if (glm::length(right) < 1.0e-5f) {
                right = glm::vec3(1.0f, 0.0f, 0.0f);
            } else {
                right = glm::normalize(right);
            }
            glm::vec3 up = glm::normalize(glm::cross(viewDir, right));

            const float heat = std::clamp((pt[i] - 320.0f) / 980.0f, 0.0f, 1.0f);
            const float plumeHeight = std::clamp(py[i] / (5.5f + 5.5f * lavaStrength), 0.0f, 1.0f);
            const float smokeMix = std::clamp(0.28f + 0.92f * plumeHeight + 0.85f * (1.0f - heat), 0.0f, 1.0f);
            const float halfSize = 0.12f + 0.16f * smokeMix + 0.09f * plumeHeight;

            const uint32_t h0 = static_cast<uint32_t>(i * 2654435761u + 17u);
            const uint32_t h1 = h0 ^ 0x9e3779b9u;
            const uint32_t h2 = h0 ^ 0x85ebca6bu;
            const float hueJitter = hash01(h0) * 2.0f - 1.0f;
            const float valJitter = hash01(h1) * 2.0f - 1.0f;
            const float alphaJitter = hash01(h2) * 2.0f - 1.0f;

            const glm::vec3 hotColor(0.92f, 0.55f, 0.18f);
            const glm::vec3 sootBase(0.11f, 0.11f, 0.12f);
            const glm::vec3 sootHigh(0.40f, 0.41f, 0.43f);
            const glm::vec3 smokeColor = glm::mix(sootBase, sootHigh, std::pow(plumeHeight, 0.75f));
            const glm::vec3 baseColor = glm::mix(hotColor, smokeColor, smokeMix);
            const float brightness = 1.0f + 0.12f * valJitter;
            const float density = (0.18f + 0.30f * smokeMix) * (0.75f + 0.40f * (1.0f - plumeHeight));
            glm::vec4 col(
                baseColor.r * (1.0f + 0.08f * hueJitter) * brightness,
                baseColor.g * brightness,
                baseColor.b * (1.0f - 0.05f * hueJitter) * brightness,
                density * (1.0f + 0.22f * alphaJitter));
            col.r = std::clamp(col.r, 0.0f, 1.0f);
            col.g = std::clamp(col.g, 0.0f, 1.0f);
            col.b = std::clamp(col.b, 0.0f, 1.0f);
            col.a = std::clamp(col.a, 0.04f, 0.75f);

            const float stretchX = (0.80f + 0.55f * hash01(h0 ^ 0xa24baed4u)) * (0.95f + 0.35f * plumeHeight);
            const float stretchY = (0.78f + 0.50f * hash01(h1 ^ 0x3c6ef372u)) * (0.95f + 0.25f * smokeMix);
            const float rot = 6.28318530718f * hash01(h2 ^ 0xbb67ae85u);
            const float lobe = 0.08f + 0.36f * hash01(h0 ^ 0x510e527fu);
            const glm::vec4 shape(stretchX, stretchY, rot, lobe);

            instances.push_back({center, right, up, halfSize, col, shape});
        }

        if (!instances.empty()) {
            glDisable(GL_LIGHTING);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);

            glm::mat4 projectionMatrix;
            glGetFloatv(GL_PROJECTION_MATRIX, glm::value_ptr(projectionMatrix));

            lavaParticleShader.use();
            lavaParticleShader.setMat4("view", camera.getViewMatrix());
            lavaParticleShader.setMat4("projection", projectionMatrix);
            lavaParticleShader.setFloat("uTime", timeSec);

            glBindVertexArray(lavaParticleVAO);
            glBindBuffer(GL_ARRAY_BUFFER, lavaParticleInstanceVBO);
            glBufferData(GL_ARRAY_BUFFER, instances.size() * sizeof(LavaParticleInstance), instances.data(), GL_DYNAMIC_DRAW);
            glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(instances.size()));
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);
            lavaParticleShader.unuse();
        }
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Renderer::reshape(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, static_cast<float>(width) / height, 0.1f, 100.0f); // Adjust near/far planes
    glMatrixMode(GL_MODELVIEW);
}

bool Renderer::loadTexture(const char *filename, GLuint &textureID)
{
    textureID = SOIL_load_OGL_texture(filename, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y);
    if (textureID == 0)
    {
        std::cerr << "SOIL loading error: '" << SOIL_last_result() << "' (" << filename << ")" << std::endl;
        return false;
    }

    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D); // Generate mipmaps for better quality at distance
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture
    return true;
}

void Renderer::setupLighting()
{
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightAmbient[] = {0.2f, 0.2f, 0.2f, 1.0f};
    GLfloat lightDiffuse[] = {0.8f, 0.8f, 0.8f, 1.0f};
    GLfloat lightSpecular[] = {0.5f, 0.5f, 0.5f, 1.0f};
    GLfloat lightPosition[] = {10.0f, 10.0f, 10.0f, 0.0f}; // Directional light from above and to the right

    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE); // Material colors track glColor
    GLfloat matSpecular[] = {0.5f, 0.5f, 0.5f, 1.0f};
    GLfloat matShininess[] = {50.0f};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShininess);
}

void Renderer::drawOcean(const Ocean& ocean, const Camera& camera)
{
    
    glPushMatrix();

    // Use the ocean shader program:
    oceanShader.use();
    checkGLError("oceanShader.use"); // Check after shader use

    // Set Uniforms for Ocean Shader:
    glm::mat4 modelMatrix;
    glm::mat4 projectionMatrix;

    glGetFloatv(GL_MODELVIEW_MATRIX, glm::value_ptr(modelMatrix));
    checkGLError("glGetFloatv(GL_MODELVIEW_MATRIX)"); // Check after glGetFloatv
    glGetFloatv(GL_PROJECTION_MATRIX, glm::value_ptr(projectionMatrix));
    checkGLError("glGetFloatv(GL_PROJECTION_MATRIX)"); // Check after glGetFloatv


    oceanShader.setMat4("model", modelMatrix);
    oceanShader.setMat4("view", camera.getViewMatrix());
    oceanShader.setMat4("projection", projectionMatrix);
    oceanShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(modelMatrix))));
    checkGLError("shader.setMat4/setMat3 uniforms"); // Check after setting matrix uniforms


    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, oceanTextureID);
    oceanShader.setInt("oceanTexture", 0);
    checkGLError("glBindTexture - oceanTexture"); // Check after glBindTexture

    //glActiveTexture(GL_TEXTURE1);
    //glBindTexture(GL_TEXTURE_2D, normalMapTextureID);
    //oceanShader.setInt("normalMap", 1);
    //checkGLError("glBindTexture - normalMapTexture"); // Check after glBindTexture


    oceanShader.setVec3("lightDir", glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f)));
    oceanShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    oceanShader.setVec3("viewPosWorld", camera.getPosition());
    checkGLError("shader.setVec3 uniforms"); // Check after setting vec3 uniforms


    drawMeshVBO(ocean); // Draw using VBOs and IBO
    checkGLError("drawMeshVBO"); // Check after drawMeshVBO call

    // Unuse shader program after drawing ocean
    oceanShader.unuse();
    checkGLError("oceanShader.unuse"); // Check after shader unuse

    //glPopMatrix();

    glPushMatrix();
    //glTranslatef(0.0f, -1.0f, 0.0f); // Lower the ocean slightly

    // **Disable Texture and Lighting for Normal Visualization**
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.0f, 0.0f); // Red color for normals (you can change this)

    #if SHOW_NORM
    const int gridSize = ocean.getGridSize();
    

    // **Draw Normals as Lines**
    glBegin(GL_LINES);
    for (int x = 0; x < gridSize; ++x)
    {
        for (int z = 0; z < gridSize; ++z)
        {
            glm::vec3 v = ocean.getVertex(x, z);
            glm::vec3 normal = ocean.getWaveNormal(v.x, v.z, ocean.time); // Get normal at vertex

            // Calculate endpoint of normal line - Scale normal for visibility
            float normalLength = 0.5f; // Adjust this value to control normal line length
            glm::vec3 normalEndpoint = v + normal * normalLength;

            // Draw line representing the normal
            glVertex3f(v.x, v.y, v.z);                                        // Start point of normal line (vertex position)
            glVertex3f(normalEndpoint.x, normalEndpoint.y, normalEndpoint.z); // End point of normal line (along normal direction)
        }
    }
    glEnd();

    #endif

    // **Re-enable Texture and Lighting (if you want to switch back to textured rendering later)**
    // glEnable(GL_TEXTURE_2D);
    // glEnable(GL_LIGHTING);

    glPopMatrix();

    glPushMatrix();

    // **Disable Texture and Lighting for Wireframe Rendering**
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glColor3f(0.0f, 0.0f, 0.0f); // Green color for wireframe (you can change this)




    #if SHOW_GRID

    // **Change glBegin mode to GL_LINES for Wireframe**
    glBegin(GL_LINES);
    for (int x = 0; x < gridSize; ++x)
    {
        for (int z = 0; z < gridSize; ++z)
        {
            glm::vec3 v = ocean.getVertex(x, z);

            // Draw horizontal lines (along Z-axis)
            if (x < gridSize - 1)
            {
                glm::vec3 v_next_x = ocean.getVertex(x + 1, z);
                glVertex3f(v.x, v.y, v.z);
                glVertex3f(v_next_x.x, v_next_x.y, v_next_x.z);
            }
            // Draw vertical lines (along X-axis)
            if (z < gridSize - 1)
            {
                glm::vec3 v_next_z = ocean.getVertex(x, z + 1);
                glVertex3f(v.x, v.y, v.z);
                glVertex3f(v_next_z.x, v_next_z.y, v_next_z.z);
            }
        }
    }
    glEnd();

    #endif

    // **Re-enable Texture and Lighting (if you want to switch back to textured rendering later)**
    // glEnable(GL_TEXTURE_2D);
    // glEnable(GL_LIGHTING);
    glPopMatrix();

    glPopMatrix();
}

// ... (rest of Renderer.cpp - Renderer::drawBoat, Renderer::drawMesh, etc.) ...

void Renderer::drawBoat(const Boat &boat)
{
    glPushMatrix();
    glm::vec3 boatPos = boat.getPosition();
    glm::quat boatRotation = boat.getRotation();
    float boatScale = boat.getScale(); // Get the boat's scale factor

    glTranslatef(boatPos.x, boatPos.y, boatPos.z);
    glm::mat4 rotationMatrix = glm::mat4_cast(boatRotation);
    glMultMatrixf(glm::value_ptr(rotationMatrix));

    glScalef(boatScale, boatScale, boatScale); // Uniform scaling - scale equally in all directions

    // Load boat texture here, based on the texture path from the Boat class
    if (!boat.getTexturePath().empty())
    {
        if (boatTextureID == 0 || boat.getTexturePath() != lastBoatTexturePath)
        { // Check if texture needs to be loaded or reloaded
            if (boatTextureID != 0)
            { // If there's a previous texture, delete it
                glDeleteTextures(1, &boatTextureID);
                boatTextureID = 0;
            }
            if (!loadTexture(boat.getTexturePath().c_str(), boatTextureID))
            {
                std::cerr << "Error loading boat texture: " << boat.getTexturePath() << std::endl;
            }
            else
            {
                lastBoatTexturePath = boat.getTexturePath(); // Store the path of the loaded texture
            }
        }
    }

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, boatTextureID);
    glColor3f(1.0f, 1.0f, 1.0f); // Texture color modulation

    // Draw the loaded boat mesh
    drawMesh(boat.getVertices(), boat.getNormals(), boat.getTexCoords(), boat.getMaterialIndices(), boat.getMaterials()); // Pass material data

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glPopMatrix();

    glPushMatrix();
    glm::vec3 minPoint = boat.getBoundingBoxMin(); // Get model-space min point
    glm::vec3 maxPoint = boat.getBoundingBoxMax(); // Get model-space max point

    glTranslatef(boatPos.x, boatPos.y, boatPos.z);
    glMultMatrixf(glm::value_ptr(rotationMatrix));
    glScalef(boatScale, boatScale, boatScale); // Uniform scaling - scale equally in all directions
                                               // **Draw Wireframe Bounding Box:**
    glDisable(GL_LIGHTING);                    // Disable lighting for bounding box (solid color wireframe)
    glColor3f(1.0f, 1.0f, 0.0f);               // Yellow color for bounding box
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // Set polygon mode to line (wireframe)


    #if SHOW_BOUDING_BOX
    glBegin(GL_QUADS); // Draw a cube using quads (wireframe)

    // Front face
    glVertex3f(minPoint.x, minPoint.y, maxPoint.z);
    glVertex3f(maxPoint.x, minPoint.y, maxPoint.z);
    glVertex3f(maxPoint.x, maxPoint.y, maxPoint.z);
    glVertex3f(minPoint.x, maxPoint.y, maxPoint.z);

    // Back face
    glVertex3f(minPoint.x, minPoint.y, minPoint.z);
    glVertex3f(maxPoint.x, minPoint.y, minPoint.z);
    glVertex3f(maxPoint.x, maxPoint.y, minPoint.z);
    glVertex3f(minPoint.x, maxPoint.y, minPoint.z);

    // Top face
    glVertex3f(minPoint.x, maxPoint.y, maxPoint.z);
    glVertex3f(maxPoint.x, maxPoint.y, maxPoint.z);
    glVertex3f(maxPoint.x, maxPoint.y, minPoint.z);
    glVertex3f(minPoint.x, maxPoint.y, minPoint.z);

    // Bottom face
    glVertex3f(minPoint.x, minPoint.y, maxPoint.z);
    glVertex3f(maxPoint.x, minPoint.y, maxPoint.z);
    glVertex3f(maxPoint.x, minPoint.y, minPoint.z);
    glVertex3f(minPoint.x, minPoint.y, minPoint.z);

    // Right face
    glVertex3f(maxPoint.x, minPoint.y, maxPoint.z);
    glVertex3f(maxPoint.x, maxPoint.y, maxPoint.z);
    glVertex3f(maxPoint.x, maxPoint.y, minPoint.z);
    glVertex3f(maxPoint.x, minPoint.y, minPoint.z);

    // Left face
    glVertex3f(minPoint.x, minPoint.y, maxPoint.z);
    glVertex3f(minPoint.x, maxPoint.y, maxPoint.z);
    glVertex3f(minPoint.x, maxPoint.y, minPoint.z);
    glVertex3f(minPoint.x, minPoint.y, minPoint.z);

    glEnd();

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Restore polygon mode to fill
    #endif

    glEnable(GL_LIGHTING);    
                     // Re-enable lighting for boat model
    glPopMatrix();
}


void Renderer::drawMeshVBO(const Ocean& ocean) {
    // 1. Bind Vertex Array Object (VAO) - if you are using VAOs. If not, bind VBOs directly.
    glBindVertexArray(ocean.getVAO()); // Assuming Ocean class has getVAO() that returns VAO ID

    // If NOT using VAOs, you would bind VBOs individually like this:
    /*
    glBindBuffer(GL_ARRAY_BUFFER, ocean.getVertexBufferID()); // Bind vertex VBO
    glBindBuffer(GL_ARRAY_BUFFER, ocean.getNormalBufferID());   // Bind normal VBO
    glBindBuffer(GL_ARRAY_BUFFER, ocean.getTexCoordBufferID()); // Bind texCoord VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ocean.getIndexBufferID()); // Bind IBO
    */


    // 2. Draw using glDrawElements (assuming you are using indexed rendering with IBO)
    glDrawElements(GL_QUADS, ocean.getIndexCount(), GL_UNSIGNED_INT, 0); // Draw using indices from IBO

    // If drawing as triangles instead of quads, use:
    // glDrawElements(GL_TRIANGLES, ocean.getIndexCount(), GL_UNSIGNED_INT, 0);


    // 3. Unbind VAO (or VBOs if not using VAOs) - optional, but good practice
    glBindVertexArray(0);

    // If NOT using VAOs, unbind VBOs individually (optional):
    /*
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    */
}

void Renderer::drawMeshTerainVBO(const Terrain& terrain) {
    // 1. Bind Vertex Array Object (VAO) - if you are using VAOs. If not, bind VBOs directly.
    glBindVertexArray(terrain.getVAO()); // Assuming Ocean class has getVAO() that returns VAO ID

    // If NOT using VAOs, you would bind VBOs individually like this:
    /*
    glBindBuffer(GL_ARRAY_BUFFER, ocean.getVertexBufferID()); // Bind vertex VBO
    glBindBuffer(GL_ARRAY_BUFFER, ocean.getNormalBufferID());   // Bind normal VBO
    glBindBuffer(GL_ARRAY_BUFFER, ocean.getTexCoordBufferID()); // Bind texCoord VBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ocean.getIndexBufferID()); // Bind IBO
    */


    // 2. Draw using glDrawElements (assuming you are using indexed rendering with IBO)
    glDrawElements(GL_QUADS, terrain.getIndexCount(), GL_UNSIGNED_INT, 0); // Draw using indices from IBO

    // If drawing as triangles instead of quads, use:
    // glDrawElements(GL_TRIANGLES, ocean.getIndexCount(), GL_UNSIGNED_INT, 0);


    // 3. Unbind VAO (or VBOs if not using VAOs) - optional, but good practice
    glBindVertexArray(0);

    // If NOT using VAOs, unbind VBOs individually (optional):
    /*
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    */
}

// Static variable to store the last loaded boat texture path

void Renderer::drawMesh(const std::vector<glm::vec3> &vertices, const std::vector<glm::vec3> &normals, const std::vector<glm::vec2> &texCoords,
                        const std::vector<int> &materialIndices, const std::vector<tinyobj::material_t> &materials)
{
    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < vertices.size(); ++i)
    {
        int material_index = materialIndices[i];
        if (material_index != -1 && static_cast<std::vector<tinyobj::material_t>::size_type>(material_index) < materials.size())
        { // Check if material index is valid
            const tinyobj::material_t &material = materials[material_index];
            glColor3f(material.diffuse[0], material.diffuse[1], material.diffuse[2]); // Set diffuse color
        }
        else
        {
            glColor3f(1.0f, 1.0f, 1.0f); // Default white color if no valid material
        }

        glNormal3f(normals[i].x, normals[i].y, normals[i].z);
        glTexCoord2f(texCoords[i].x, texCoords[i].y);
        glVertex3f(vertices[i].x, vertices[i].y, vertices[i].z);
    }
    glEnd();
}

void Renderer::drawLavaFlowsOnTerrain(const Terrain& terrain, const VolcanoSimulation& volcano) {
    const int terrainGridSize = terrain.getGridSize();
    const int lavaGridWidth = volcano.getGridWidth();
    const int lavaGridHeight = volcano.getGridHeight();
    if (terrainGridSize < 3 || lavaGridWidth < 2 || lavaGridHeight < 2) {
        return;
    }

    const VolcanoSimulation::RenderConfig cfg = volcano.getRenderConfig();
    const float* lavaGrid = volcano.getLavaHeightGrid();
    const float* tempGrid = volcano.getTemperatureGrid();
    const float maxRadius = std::max(cfg.coneRadius * 1.18f, 1.0f);
    const float hardCutRadius = std::max(cfg.coneRadius * 1.28f, 1.0f);
    const float timeSec = static_cast<float>(glutGet(GLUT_ELAPSED_TIME)) * 0.001f;
    const auto gridIndex = [lavaGridWidth](int x, int z) {
        return static_cast<std::size_t>(z) * static_cast<std::size_t>(lavaGridWidth) + static_cast<std::size_t>(x);
    };
    const auto coneMaskAt = [&](float worldX, float worldZ, float terrainY) {
        const float dx = worldX - cfg.worldX;
        const float dz = worldZ - cfg.worldZ;
        const float dist = std::sqrt(dx * dx + dz * dz);
        if (dist > hardCutRadius) {
            return 0.0f;
        }

        const float radial = std::clamp(1.0f - dist / hardCutRadius, 0.0f, 1.0f);
        const float heightNorm = std::clamp((terrainY - cfg.worldY) / std::max(cfg.coneHeight, 1.0f), 0.0f, 1.0f);
        return radial * (0.35f + 0.65f * heightNorm);
    };

    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glDepthMask(GL_FALSE);

    glBegin(GL_QUADS);
    for (int gx = 0; gx < lavaGridWidth - 1; ++gx) {
        for (int gz = 0; gz < lavaGridHeight - 1; ++gz) {
            const std::size_t i00 = gridIndex(gx, gz);
            const std::size_t i10 = gridIndex(gx + 1, gz);
            const std::size_t i01 = gridIndex(gx, gz + 1);
            const std::size_t i11 = gridIndex(gx + 1, gz + 1);

            const float lava00 = std::max(0.0f, lavaGrid[i00]);
            const float lava10 = std::max(0.0f, lavaGrid[i10]);
            const float lava01 = std::max(0.0f, lavaGrid[i01]);
            const float lava11 = std::max(0.0f, lavaGrid[i11]);
            const float maxLava = std::max(std::max(lava00, lava10), std::max(lava01, lava11));
            if (maxLava < 0.002f) {
                continue;
            }

            const float nx0 = (2.0f * static_cast<float>(gx) / static_cast<float>(lavaGridWidth - 1)) - 1.0f;
            const float nx1 = (2.0f * static_cast<float>(gx + 1) / static_cast<float>(lavaGridWidth - 1)) - 1.0f;
            const float nz0 = (2.0f * static_cast<float>(gz) / static_cast<float>(lavaGridHeight - 1)) - 1.0f;
            const float nz1 = (2.0f * static_cast<float>(gz + 1) / static_cast<float>(lavaGridHeight - 1)) - 1.0f;

            const float mx = 0.5f * (nx0 + nx1);
            const float mz = 0.5f * (nz0 + nz1);
            const float radialMask = 1.0f - std::sqrt(mx * mx + mz * mz);
            if (radialMask <= -0.08f) {
                continue;
            }

            const auto emitCorner = [&](float nx, float nz, float lava, float temp) {
                const float worldX = cfg.worldX + nx * maxRadius;
                const float worldZ = cfg.worldZ + nz * maxRadius;
                if (!terrain.isInsideBoundsWorld(worldX, worldZ)) {
                    glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
                    glVertex3f(worldX, 0.0f, worldZ);
                    return;
                }

                const float terrainY = terrain.sampleHeightWorld(worldX, worldZ);
                const float coneMask = coneMaskAt(worldX, worldZ, terrainY);
                if (coneMask <= 0.02f) {
                    glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
                    glVertex3f(worldX, terrainY, worldZ);
                    return;
                }
                const float lavaNorm = std::clamp(lava * 3.10f, 0.0f, 1.0f);
                const float heatNorm = std::clamp((temp - 350.0f) / 1050.0f, 0.0f, 1.0f);
                const float pulse = 0.84f + 0.16f * std::sin(timeSec * 5.0f + worldX * 0.09f + worldZ * 0.07f);
                const float glow = std::clamp((0.55f * lavaNorm + 0.45f * heatNorm) * pulse * coneMask, 0.0f, 1.0f);
                const float alpha = std::clamp((0.24f + 0.92f * lavaNorm + 0.26f * heatNorm) * coneMask, 0.0f, 1.0f);
                const float lift = 0.09f + 0.20f * lavaNorm + 0.03f * heatNorm;

                glColor4f(1.0f, 0.30f + 0.68f * glow, 0.02f + 0.08f * (1.0f - glow), alpha);
                glVertex3f(worldX, terrainY + lift, worldZ);
            };

            emitCorner(nx0, nz0, lava00, tempGrid[i00]);
            emitCorner(nx1, nz0, lava10, tempGrid[i10]);
            emitCorner(nx1, nz1, lava11, tempGrid[i11]);
            emitCorner(nx0, nz1, lava01, tempGrid[i01]);
        }
    }
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glBegin(GL_TRIANGLES);
    for (int gx = 1; gx < lavaGridWidth - 1; ++gx) {
        for (int gz = 1; gz < lavaGridHeight - 1; ++gz) {
            const std::size_t centerIdx = gridIndex(gx, gz);
            const float lava = std::max(0.0f, lavaGrid[centerIdx]);
            const float temp = tempGrid[centerIdx];
            const float lavaNorm = std::clamp(lava * 2.6f, 0.0f, 1.0f);
            const float heatNorm = std::clamp((temp - 340.0f) / 1100.0f, 0.0f, 1.0f);
            const float intensity = std::max(lavaNorm, 0.75f * heatNorm);
            if (intensity < 0.05f) {
                continue;
            }

            const float nx = (2.0f * static_cast<float>(gx) / static_cast<float>(lavaGridWidth - 1)) - 1.0f;
            const float nz = (2.0f * static_cast<float>(gz) / static_cast<float>(lavaGridHeight - 1)) - 1.0f;
            const float worldX = cfg.worldX + nx * maxRadius;
            const float worldZ = cfg.worldZ + nz * maxRadius;
            if (!terrain.isInsideBoundsWorld(worldX, worldZ)) {
                continue;
            }
            const float terrainY = terrain.sampleHeightWorld(worldX, worldZ);
            const float coneMask = coneMaskAt(worldX, worldZ, terrainY);
            if (coneMask <= 0.02f) {
                continue;
            }

            const float step = std::max(0.35f, terrain.getGridSpacing());
            const float hL = terrain.sampleHeightWorld(worldX - step, worldZ);
            const float hR = terrain.sampleHeightWorld(worldX + step, worldZ);
            const float hD = terrain.sampleHeightWorld(worldX, worldZ - step);
            const float hU = terrain.sampleHeightWorld(worldX, worldZ + step);
            glm::vec2 downhill(hL - hR, hD - hU);
            float downLen = glm::length(downhill);
            if (downLen < 1.0e-4f) {
                downhill = glm::vec2(worldX - cfg.worldX, worldZ - cfg.worldZ);
                downLen = glm::length(downhill);
            }
            if (downLen < 1.0e-4f) {
                continue;
            }
            downhill /= downLen;

            glm::vec2 side(-downhill.y, downhill.x);
            const float slope = std::clamp(std::sqrt((hR - hL) * (hR - hL) + (hU - hD) * (hU - hD)) / (2.0f * step), 0.0f, 1.8f);
            const float runLen = (0.70f + 1.65f * intensity) * (0.85f + 0.90f * slope);
            const float runHalfWidth = 0.15f + 0.20f * intensity;
            const float flicker = 0.80f + 0.20f * std::sin(timeSec * 6.0f + worldX * 0.11f + worldZ * 0.08f);

            const glm::vec2 p0 = glm::vec2(worldX, worldZ) - side * runHalfWidth;
            const glm::vec2 p1 = glm::vec2(worldX, worldZ) + side * runHalfWidth;
            const glm::vec2 tip = glm::vec2(worldX, worldZ) + downhill * runLen;

            const float y0 = terrain.sampleHeightWorld(p0.x, p0.y) + 0.11f + 0.08f * intensity;
            const float y1 = terrain.sampleHeightWorld(p1.x, p1.y) + 0.11f + 0.08f * intensity;
            const float y2 = terrain.sampleHeightWorld(tip.x, tip.y) + 0.07f + 0.05f * intensity;

            glColor4f(1.0f, 0.94f, 0.46f, std::clamp((0.28f + 0.58f * intensity * flicker) * coneMask, 0.0f, 0.92f));
            glVertex3f(p0.x, y0, p0.y);
            glColor4f(1.0f, 0.72f, 0.16f, std::clamp((0.30f + 0.64f * intensity * flicker) * coneMask, 0.0f, 0.95f));
            glVertex3f(p1.x, y1, p1.y);
            glColor4f(1.0f, 0.18f, 0.04f, 0.02f);
            glVertex3f(tip.x, y2, tip.y);
        }
    }
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBegin(GL_QUADS);
    for (int gx = 0; gx < lavaGridWidth - 1; ++gx) {
        for (int gz = 0; gz < lavaGridHeight - 1; ++gz) {
            const std::size_t i00 = gridIndex(gx, gz);
            const std::size_t i10 = gridIndex(gx + 1, gz);
            const std::size_t i01 = gridIndex(gx, gz + 1);
            const std::size_t i11 = gridIndex(gx + 1, gz + 1);
            const float lavaMean = 0.25f * (lavaGrid[i00] + lavaGrid[i10] + lavaGrid[i01] + lavaGrid[i11]);
            if (lavaMean < 0.008f) {
                continue;
            }

            const float nx0 = (2.0f * static_cast<float>(gx) / static_cast<float>(lavaGridWidth - 1)) - 1.0f;
            const float nx1 = (2.0f * static_cast<float>(gx + 1) / static_cast<float>(lavaGridWidth - 1)) - 1.0f;
            const float nz0 = (2.0f * static_cast<float>(gz) / static_cast<float>(lavaGridHeight - 1)) - 1.0f;
            const float nz1 = (2.0f * static_cast<float>(gz + 1) / static_cast<float>(lavaGridHeight - 1)) - 1.0f;

            const auto emitCrustCorner = [&](float nx, float nz, float lava, float temp) {
                const float worldX = cfg.worldX + nx * maxRadius;
                const float worldZ = cfg.worldZ + nz * maxRadius;
                const float terrainY = terrain.sampleHeightWorld(worldX, worldZ);
                const float coneMask = coneMaskAt(worldX, worldZ, terrainY);
                if (coneMask <= 0.02f) {
                    glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
                    glVertex3f(worldX, terrainY, worldZ);
                    return;
                }
                const float lavaNorm = std::clamp(lava * 1.8f, 0.0f, 1.0f);
                const float heatNorm = std::clamp((temp - 300.0f) / 900.0f, 0.0f, 1.0f);
                const float crust = std::clamp(1.0f - heatNorm, 0.0f, 1.0f);
                glColor4f(0.18f + 0.24f * crust, 0.04f + 0.08f * crust, 0.01f, (0.14f + 0.34f * lavaNorm) * coneMask);
                glVertex3f(worldX, terrainY + 0.05f + 0.06f * lavaNorm, worldZ);
            };

            emitCrustCorner(nx0, nz0, lavaGrid[i00], tempGrid[i00]);
            emitCrustCorner(nx1, nz0, lavaGrid[i10], tempGrid[i10]);
            emitCrustCorner(nx1, nz1, lavaGrid[i11], tempGrid[i11]);
            emitCrustCorner(nx0, nz1, lavaGrid[i01], tempGrid[i01]);
        }
    }
    glEnd();

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

// Renderer.cpp - Add drawTerrain function
void Renderer::drawTerrain(const Terrain& terrain, const Camera& camera, const std::vector<VolcanoSimulation>& volcanoes) {
    
    //printf("Drawing terrain\n");
    glPushMatrix();
    // No translation needed for terrain in this basic example
    glTranslatef(0.0f, 0.0f, 0.0f); // Lower the ocean slightly

    // Use the terrain shader program:
    terrainShader.use();
    checkGLError("terrainShader.use");

    glm::mat4 modelMatrix;
    glm::mat4 projectionMatrix;

    glGetFloatv(GL_MODELVIEW_MATRIX, glm::value_ptr(modelMatrix));
    checkGLError("glGetFloatv(GL_MODELVIEW_MATRIX)"); // Check after glGetFloatv
    glGetFloatv(GL_PROJECTION_MATRIX, glm::value_ptr(projectionMatrix));
    checkGLError("glGetFloatv(GL_PROJECTION_MATRIX)"); // Check after glGetFloatv

    terrainShader.setMat4("model", modelMatrix);
    terrainShader.setMat4("view", camera.getViewMatrix());
    terrainShader.setMat4("projection", projectionMatrix);
    terrainShader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(modelMatrix))));
    checkGLError("terrainShader setMat4/setMat3 uniforms");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, terrainTextureID);
    terrainShader.setInt("terrainTexture", 0);
    checkGLError("glBindTexture - terrainTexture");

    glActiveTexture(GL_TEXTURE1); // Activate texture unit 1 for heightmap
    glBindTexture(GL_TEXTURE_2D, heightMapTextureID); // Bind heightmap texture
    terrainShader.setInt("heightMapTexture", 1); // Set uniform sampler2D heightMapTexture to texture unit 1
    checkGLError("glBindTexture - heightMapTexture"); // Check after binding heightmap texture

    // Lighting Uniforms (reuse same light parameters as ocean for simplicity)
    terrainShader.setVec3("lightDir", glm::normalize(glm::vec3(1.0f, -1.0f, 1.0f))); // Example light direction
    terrainShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f)); // White light
    terrainShader.setVec3("viewPosWorld", camera.getPosition());
    if (!volcanoes.empty()) {
        const VolcanoSimulation* dominant = &volcanoes.front();
        float dominantHeat = -1.0f;
        for (const VolcanoSimulation& v : volcanoes) {
            const float heatNorm = std::clamp((v.getAverageTemperature() - 300.0f) / 900.0f, 0.0f, 1.0f);
            const float lavaNorm = std::clamp(v.getAverageLavaHeight() * 20.0f, 0.0f, 1.0f);
            const float heat = std::max(heatNorm, lavaNorm);
            if (heat > dominantHeat) {
                dominantHeat = heat;
                dominant = &v;
            }
        }
        const VolcanoSimulation::RenderConfig dominantCfg = dominant->getRenderConfig();
        terrainShader.setVec3("volcanoCenterWorld", glm::vec3(dominantCfg.worldX, dominantCfg.worldY, dominantCfg.worldZ));
        terrainShader.setFloat("volcanoRadius", dominantCfg.coneRadius * 1.18f);
        terrainShader.setFloat("volcanoHeat", std::clamp(dominantHeat * 0.55f, 0.0f, 0.75f));
    } else {
        terrainShader.setVec3("volcanoCenterWorld", glm::vec3(0.0f, 0.0f, 0.0f));
        terrainShader.setFloat("volcanoRadius", 1.0f);
        terrainShader.setFloat("volcanoHeat", 0.0f);
    }
    checkGLError("terrainShader setVec3 uniforms");


    drawMeshTerainVBO(terrain); // Render terrain mesh using VBOs and IBO
    checkGLError("drawMeshVBO - terrain");
    glBindTexture(GL_TEXTURE_2D, 0); // Bind heightmap texture

    terrainShader.unuse();
    checkGLError("terrainShader.unuse");

    for (const VolcanoSimulation& volcano : volcanoes) {
        drawLavaFlowsOnTerrain(terrain, volcano);
    }

    glPopMatrix();

    

    glPushMatrix();
    glTranslatef(0.0f, 0.0f, 0.0f); // Lower the ocean slightly

    // **Disable Texture and Lighting for Normal Visualization**
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.0f, 0.0f); // Red color for normals (you can change this)


    #if SHOW_NORM

    int gridSize = terrain.getGridSize();
    float gridSpacing = terrain.getGridSpacing();

    // **Draw Normals as Lines**
    glBegin(GL_LINES);
    for (int x = 0; x < gridSize; ++x)
    {
        for (int z = 0; z < gridSize; ++z)
        {
            glm::vec3 v = terrain.getVertex(x, z);
            glm::vec3 normal = terrain.getNormal(x, z); // Get normal at vertex

            // Calculate endpoint of normal line - Scale normal for visibility
            float normalLength = 0.5f; // Adjust this value to control normal line length
            glm::vec3 normalEndpoint = v + normal * normalLength;

            // Draw line representing the normal
            glVertex3f(v.x, v.y, v.z);                                        // Start point of normal line (vertex position)
            glVertex3f(normalEndpoint.x, normalEndpoint.y, normalEndpoint.z); // End point of normal line (along normal direction)
        }
    }
    glEnd();

    #endif

    // **Re-enable Texture and Lighting (if you want to switch back to textured rendering later)**
    // glEnable(GL_TEXTURE_2D);
    // glEnable(GL_LIGHTING);

    glPopMatrix();



    glPushMatrix();
    glTranslatef(0.0f, 0.2f, 0.0f); // Lower the ocean slightly

    // **Disable Texture and Lighting for Wireframe Rendering**
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_LIGHTING);
    glColor3f(0.0f, 0.0f, 0.0f); // Green color for wireframe (you can change this)

    #if SHOW_GRID
    // **Change glBegin mode to GL_LINES for Wireframe**
    glBegin(GL_LINES);
    for (int x = 0; x < gridSize; ++x)
    {
        for (int z = 0; z < gridSize; ++z)
        {
            glm::vec3 v = terrain.getVertex(x, z);

            // Draw horizontal lines (along Z-axis)
            if (x < gridSize - 1)
            {
                glm::vec3 v_next_x = terrain.getVertex(x + 1, z);
                glVertex3f(v.x, v.y, v.z);
                glVertex3f(v_next_x.x, v_next_x.y, v_next_x.z);
            }
            // Draw vertical lines (along X-axis)
            if (z < gridSize - 1)
            {
                glm::vec3 v_next_z = terrain.getVertex(x, z + 1);
                glVertex3f(v.x, v.y, v.z);
                glVertex3f(v_next_z.x, v_next_z.y, v_next_z.z);
            }
        }
    }
    glEnd();

    #endif
    // **Re-enable Texture and Lighting (if you want to switch back to textured rendering later)**
    // glEnable(GL_TEXTURE_2D);
    // glEnable(GL_LIGHTING);

    glPopMatrix();
}
