/*
 * File:        Terran.cpp
 * Author:      Tomas Goldmann
 * Date:        2025-03-23
 * Description: This class defines terrain (surface).
 *
 * Copyright (c) 2025, Brno University of Technology. All rights reserved.
 * Licensed under the MIT.
 */


#include "Terrain.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <iostream>
#include "utils.h" // Include utils.h for checkGLError
#include <GL/glew.h>
#include <limits>
#include <numeric>
#include <cassert>

Terrain::Terrain(int gridSize, float gridSpacing)
    : gridSize(gridSize),
      gridSpacing(gridSpacing),
      vertices(gridSize * gridSize),
      normals(gridSize * gridSize),
      texCoords(gridSize * gridSize),
      vertexBufferID(0),
      normalBufferID(0),
      texCoordBufferID(0),
      indexBufferID(0),
      vaoID(0),
      indexCount(0),
      highestPoint(0.0f, std::numeric_limits<float>::lowest(), 0.0f) {}

Terrain::~Terrain() {
    cleanup();
}

bool Terrain::init(GLuint heightMapTextureID) {
    generateGrid(heightMapTextureID);
    createBuffers();
    return true;
}

void Terrain::cleanup() {
    if (vertexBufferID != 0) {
        glDeleteBuffers(1, &vertexBufferID);
        vertexBufferID = 0;
    }
    if (normalBufferID != 0) {
        glDeleteBuffers(1, &normalBufferID);
        normalBufferID = 0;
    }
    if (texCoordBufferID != 0) {
        glDeleteBuffers(1, &texCoordBufferID);
        texCoordBufferID = 0;
    }
    if (indexBufferID != 0) {
        glDeleteBuffers(1, &indexBufferID);
        indexBufferID = 0;
    }
    if (vaoID != 0) {
        glDeleteVertexArrays(1, &vaoID);
        vaoID = 0;
    }
}



void Terrain::generateGrid(GLuint heightMapTextureID) {

    vertices.resize(gridSize * gridSize);
    normals.resize(gridSize * gridSize);
    texCoords.resize(gridSize * gridSize);

    // **Access Heightmap Texture Data**
    glBindTexture(GL_TEXTURE_2D, heightMapTextureID); // Bind heightmap texture
    GLint textureWidth, textureHeight;
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &textureWidth); // Get texture width
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &textureHeight); // Get texture height

    std::vector<unsigned char> heightmapData(textureWidth * textureHeight); // Assuming 8-bit grayscale heightmap
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

    glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, heightmapData.data()); // Get texture pixel data (Red channel = grayscale height)
    glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture

    float heightScale = 28.0f; // Smaller vertical scale so the volcano is less tall
    const float terrainBaseOffset = -10.0f; // Lower whole terrain a bit relative to sea level.

    highestPoint = glm::vec3(0.0f, std::numeric_limits<float>::lowest(), 0.0f);

    for (int x = 0; x < gridSize; ++x) {
        for (int z = 0; z < gridSize; ++z) {
            float worldX = (x - gridSize / 2.0f) * gridSpacing;
            float worldZ = (z - gridSize / 2.0f) * gridSpacing;

            const float uNorm = static_cast<float>(x) / static_cast<float>(gridSize - 1);
            const float vNorm = static_cast<float>(z) / static_cast<float>(gridSize - 1);
            const int texX = std::clamp(static_cast<int>(std::round(uNorm * static_cast<float>(textureWidth - 1))), 0, textureWidth - 1);
            const int texZ = std::clamp(static_cast<int>(std::round(vNorm * static_cast<float>(textureHeight - 1))), 0, textureHeight - 1);
            const std::size_t texIndex = static_cast<std::size_t>(texZ) * static_cast<std::size_t>(textureWidth) + static_cast<std::size_t>(texX);

            // Height is sampled from volcano heatmap and shifted down, so low values stay near sea level.
            const float height = static_cast<float>(heightmapData[texIndex]) / 255.0f * heightScale + terrainBaseOffset;

            vertices[x * gridSize + z] = glm::vec3(worldX, height, worldZ);
            texCoords[x * gridSize + z] = glm::vec2(uNorm, vNorm);

            if (height > highestPoint.y) {
                highestPoint = vertices[x * gridSize + z];
            }
        }
    }

    for (int x = 0; x < gridSize; ++x) {
        for (int z = 0; z < gridSize; ++z) {
            const int xL = std::max(0, x - 1);
            const int xR = std::min(gridSize - 1, x + 1);
            const int zD = std::max(0, z - 1);
            const int zU = std::min(gridSize - 1, z + 1);

            const float hL = vertices[xL * gridSize + z].y;
            const float hR = vertices[xR * gridSize + z].y;
            const float hD = vertices[x * gridSize + zD].y;
            const float hU = vertices[x * gridSize + zU].y;

            const glm::vec3 normal = glm::normalize(glm::vec3(hL - hR, 2.0f * gridSpacing, hD - hU));
            normals[x * gridSize + z] = normal;
        }
    }
}


void Terrain::createBuffers() {
    glGenVertexArrays(1, &vaoID);
    checkGLError("1"); // Check after drawMeshVBO call

    glBindVertexArray(vaoID);
    checkGLError("2"); // Check after drawMeshVBO call

    glGenBuffers(1, &vertexBufferID);
    checkGLError("3"); // Check after drawMeshVBO call

    glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
    checkGLError("4"); // Check after drawMeshVBO call

    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    checkGLError("5"); // Check after drawMeshVBO call

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    checkGLError("6"); // Check after drawMeshVBO call

    glEnableVertexAttribArray(0);
    checkGLError("7"); // Check after drawMeshVBO call

    glGenBuffers(1, &normalBufferID);
    checkGLError("8"); // Check after drawMeshVBO call

    glBindBuffer(GL_ARRAY_BUFFER, normalBufferID);
    checkGLError("9"); // Check after drawMeshVBO call

    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
    checkGLError("10"); // Check after drawMeshVBO call

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    checkGLError("11"); // Check after drawMeshVBO call

    glEnableVertexAttribArray(1);
    checkGLError("12"); // Check after drawMeshVBO call


    glGenBuffers(1, &texCoordBufferID);
    checkGLError("13"); // Check after drawMeshVBO call

    glBindBuffer(GL_ARRAY_BUFFER, texCoordBufferID);
    checkGLError("14"); // Check after drawMeshVBO call

    glBufferData(GL_ARRAY_BUFFER, texCoords.size() * sizeof(glm::vec2), texCoords.data(), GL_STATIC_DRAW);
    checkGLError("15"); // Check after drawMeshVBO call

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    checkGLError("16"); // Check after drawMeshVBO call

    glEnableVertexAttribArray(2);
    checkGLError("17"); // Check after drawMeshVBO call


    std::vector<unsigned int> indices;
    for (int x = 0; x < gridSize - 1; ++x) {
        for (int z = 0; z < gridSize - 1; ++z) {
            unsigned int v00 = x * gridSize + z;
            unsigned int v10 = (x + 1) * gridSize + z;
            unsigned int v11 = (x + 1) * gridSize + (z + 1);
            unsigned int v01 = x * gridSize + (z + 1);
            indices.insert(indices.end(), {v00, v10, v11, v01});
        }
    }
    glGenBuffers(1, &indexBufferID);
    checkGLError("glGenBuffers - indexBufferID"); // Check after glGenBuffers
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
    checkGLError("18"); // Check after drawMeshVBO call

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    checkGLError("19"); // Check after drawMeshVBO call

    glBindVertexArray(0);
    checkGLError("20"); // Check after drawMeshVBO call

    indexCount = indices.size();
}

void Terrain::updateBuffers() {
    // Not needed for static terrain in this basic example
}


glm::vec3 Terrain::getVertex(int x, int z) const {
    assert(x >= 0 && x < gridSize);
    assert(z >= 0 && z < gridSize);
    return vertices[x * gridSize + z];
}

bool Terrain::isInsideBoundsWorld(float worldX, float worldZ) const {
    if (gridSize <= 1 || gridSpacing <= 0.0f || vertices.empty()) {
        return false;
    }

    const float minX = vertices[0].x;
    const float minZ = vertices[0].z;
    const float maxX = vertices[(gridSize - 1) * gridSize + (gridSize - 1)].x;
    const float maxZ = vertices[(gridSize - 1) * gridSize + (gridSize - 1)].z;
    return worldX >= minX && worldX <= maxX && worldZ >= minZ && worldZ <= maxZ;
}

float Terrain::sampleHeightWorld(float worldX, float worldZ) const {
    if (gridSize <= 1 || gridSpacing <= 0.0f || vertices.empty()) {
        return 0.0f;
    }

    const glm::vec3 origin = vertices[0];
    const float fx = (worldX - origin.x) / gridSpacing;
    const float fz = (worldZ - origin.z) / gridSpacing;

    if (!std::isfinite(fx) || !std::isfinite(fz)) {
        return origin.y;
    }

    const float maxIdx = static_cast<float>(gridSize - 1);
    const float clampedX = std::clamp(fx, 0.0f, maxIdx);
    const float clampedZ = std::clamp(fz, 0.0f, maxIdx);

    const int x0 = static_cast<int>(std::floor(clampedX));
    const int z0 = static_cast<int>(std::floor(clampedZ));
    const int x1 = std::min(x0 + 1, gridSize - 1);
    const int z1 = std::min(z0 + 1, gridSize - 1);

    const float tx = clampedX - static_cast<float>(x0);
    const float tz = clampedZ - static_cast<float>(z0);

    const auto h = [&](int x, int z) {
        return vertices[x * gridSize + z].y;
    };

    const float h00 = h(x0, z0);
    const float h10 = h(x1, z0);
    const float h01 = h(x0, z1);
    const float h11 = h(x1, z1);

    const float hx0 = h00 + tx * (h10 - h00);
    const float hx1 = h01 + tx * (h11 - h01);
    return hx0 + tz * (hx1 - hx0);
}

float Terrain::sampleGradientMagnitudeWorld(float worldX, float worldZ) const {
    if (gridSize <= 1 || gridSpacing <= 0.0f || vertices.empty()) {
        return 0.0f;
    }

    const float step = std::max(0.25f, 0.5f * gridSpacing);

    const float hL = sampleHeightWorld(worldX - step, worldZ);
    const float hR = sampleHeightWorld(worldX + step, worldZ);
    const float hD = sampleHeightWorld(worldX, worldZ - step);
    const float hU = sampleHeightWorld(worldX, worldZ + step);

    const float dHdx = (hR - hL) / (2.0f * step);
    const float dHdz = (hU - hD) / (2.0f * step);
    return std::sqrt(dHdx * dHdx + dHdz * dHdz);
}

float Terrain::sampleDirectionalGradientWorld(float worldX, float worldZ, const glm::vec2& directionWorld) const {
    if (gridSize <= 1 || gridSpacing <= 0.0f || vertices.empty()) {
        return 0.0f;
    }

    const float dirLen = glm::length(directionWorld);
    if (dirLen <= 1.0e-5f) {
        return 0.0f;
    }

    const glm::vec2 dir = directionWorld / dirLen;
    const float step = std::max(0.25f, gridSpacing);

    const float h0 = sampleHeightWorld(worldX, worldZ);
    const float h1 = sampleHeightWorld(worldX + dir.x * step, worldZ + dir.y * step);
    return (h1 - h0) / step;
}


glm::vec3 Terrain::getNormal(int x, int z) const
{
    assert(x >= 0 && x < gridSize);
    assert(z >= 0 && z < gridSize);
    return normals[x * gridSize + z];

}

std::vector<glm::vec3> Terrain::getHighestPeaks(int count, float minDistanceWorld) const
{
    std::vector<glm::vec3> peaks;
    if (count <= 0 || vertices.empty()) {
        return peaks;
    }

    std::vector<std::size_t> order(vertices.size());
    std::iota(order.begin(), order.end(), static_cast<std::size_t>(0));
    std::sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        return vertices[a].y > vertices[b].y;
    });

    const float minDist2 = std::max(0.0f, minDistanceWorld * minDistanceWorld);
    for (std::size_t id : order) {
        const glm::vec3& candidate = vertices[id];
        bool tooClose = false;
        for (const glm::vec3& p : peaks) {
            const float dx = candidate.x - p.x;
            const float dz = candidate.z - p.z;
            if (dx * dx + dz * dz < minDist2) {
                tooClose = true;
                break;
            }
        }
        if (tooClose) {
            continue;
        }

        peaks.push_back(candidate);
        if (static_cast<int>(peaks.size()) >= count) {
            break;
        }
    }

    return peaks;
}
