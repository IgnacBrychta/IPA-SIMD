/*
 * File:        Boat.cpp
 * Author:      Tomas Goldmann
 * Date:        2025-03-23
 * Description: Class for representng boat
 *
 * Copyright (c) 2025, Brno University of Technology. All rights reserved.
 * Licensed under the MIT.
 */




#include "Boat.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h> // Include tinyobjloader
#include <glm/gtx/norm.hpp>

Boat::Boat() : position(0.0f, 0.5f, 0.0f), rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)), speed(0.0f), steeringSpeed(1.0f), materials(), boatScale(1.0f), speedUpKeyWasDown(false), speedDownKeyWasDown(false), lastCollisionWithTerrain(false), lastCollisionAllowsDisembark(false), lastCollisionPoint(0.0f) {} // Initialize boatScale to 1.0f
Boat::~Boat() {}

bool Boat::init(const char* modelPath, const char* texturePath) {
    materials.clear(); // Explicitly clear materials before loading model
    if (!loadModel(modelPath)) {
        std::cerr << "Error loading boat model: " << modelPath << std::endl;
        return false;
    }
    boatTexturePath = texturePath;
    return true;
}

void Boat::cleanup() {
    // No dynamic memory cleanup in this simple example, but if you used dynamic allocation in loadModel, clean it up here.
}

void Boat::update(const Input& input, const Ocean& ocean, const Terrain& terrain, float deltaTime) {
    handleInput(input, ocean, terrain, deltaTime);
    applyWaveMotion(ocean);
}

void Boat::clearCollisionState() {
    lastCollisionWithTerrain = false;
    lastCollisionAllowsDisembark = false;
}

void Boat::handleInput(const Input& input, const Ocean& ocean, const Terrain& terrain, float deltaTime) {
    clearCollisionState();

    const bool speedUpDown = input.isKeyDown('+') || input.isKeyDown('=');
    if (speedUpDown && !speedUpKeyWasDown) {
        speed += 0.25f;
    }
    speedUpKeyWasDown = speedUpDown;

    const bool speedDownDown = input.isKeyDown('-') || input.isKeyDown('_');
    if (speedDownDown && !speedDownKeyWasDown) {
        speed -= 0.25f;
    }
    speedDownKeyWasDown = speedDownDown;

    if (input.isKeyDown('w')) {
        speed += 0.5f * deltaTime; // Accelerate forward
    }
    if (input.isKeyDown('s')) {
        speed -= 0.5f * deltaTime; // Decelerate/reverse
    }
    if (input.isKeyDown('a')) {
        rotation = glm::rotate(rotation, steeringSpeed * deltaTime, glm::vec3(0.0f, 1.0f, 0.0f)); // Steer left
    }
    if (input.isKeyDown('d')) {
        rotation = glm::rotate(rotation, -steeringSpeed * deltaTime, glm::vec3(0.0f, 1.0f, 0.0f)); // Steer right
    }

    speed = glm::clamp(speed, -0.5f, 8.0f); // Clamp speed
    if (!input.isKeyDown('w') && !input.isKeyDown('s')) {
        speed *= (1.0f - 0.5f * deltaTime); // Natural deceleration
        if (fabs(speed) < 0.01f) speed = 0.0f; // Stop completely
    }

    // Move boat forward/backward based on rotation and speed
    glm::vec3 forwardVector = rotation * glm::vec3(0.0f, 0.0f, -1.0f); // Boat's forward direction
    const glm::vec3 candidatePosition = position + forwardVector * speed * deltaTime;
    glm::vec3 collisionPoint = candidatePosition;
    if (collidesWithTerrain(terrain, ocean, candidatePosition, &collisionPoint)) {
        speed = 0.0f;
        lastCollisionWithTerrain = true;
        lastCollisionPoint = collisionPoint;
        lastCollisionAllowsDisembark = isGradientLowForDisembark(terrain, collisionPoint);
        return;
    }
    position = candidatePosition;
}

float Boat::getCollisionRadius() const {
    const float extentX = std::max(std::fabs(boundingBoxMin.x), std::fabs(boundingBoxMax.x)) * boatScale;
    const float extentZ = std::max(std::fabs(boundingBoxMin.z), std::fabs(boundingBoxMax.z)) * boatScale;
    const float radiusFromModel = std::sqrt(extentX * extentX + extentZ * extentZ) * 0.5f;
    return std::max(0.8f, radiusFromModel);
}

bool Boat::collidesWithTerrain(const Terrain& terrain, const Ocean& ocean, const glm::vec3& candidatePosition, glm::vec3* collisionPoint) const {
    const float radius = getCollisionRadius();
    const float boatDraft = 0.25f;
    const glm::vec3 forward = glm::normalize(rotation * glm::vec3(0.0f, 0.0f, -1.0f));
    glm::vec3 right = glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f));
    if (glm::length(right) > 1.0e-5f) {
        right = glm::normalize(right);
    } else {
        right = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    const glm::vec3 samples[5] = {
        candidatePosition,
        candidatePosition + forward * radius,
        candidatePosition - forward * radius,
        candidatePosition + right * radius,
        candidatePosition - right * radius
    };

    for (const glm::vec3& samplePos : samples) {
        if (!terrain.isInsideBoundsWorld(samplePos.x, samplePos.z)) {
            continue;
        }

        const float terrainY = terrain.sampleHeightWorld(samplePos.x, samplePos.z);
        const float waterY = ocean.getWaveHeight(samplePos.x, samplePos.z, ocean.time);
        if (terrainY + boatDraft >= waterY) {
            if (collisionPoint != nullptr) {
                *collisionPoint = samplePos;
            }
            return true;
        }
    }
    return false;
}

bool Boat::isGradientLowForDisembark(const Terrain& terrain, const glm::vec3& collisionPoint) const {
    if (!terrain.isInsideBoundsWorld(collisionPoint.x, collisionPoint.z)) {
        return false;
    }

    constexpr float kMaxAllowedGradient = 0.85f;
    constexpr float kCheckRadius = 1.5f;
    const glm::vec2 samples[] = {
        glm::vec2(0.0f, 0.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec2(-1.0f, 0.0f),
        glm::vec2(0.0f, 1.0f),
        glm::vec2(0.0f, -1.0f),
        glm::vec2(0.7071f, 0.7071f),
        glm::vec2(-0.7071f, 0.7071f),
        glm::vec2(0.7071f, -0.7071f),
        glm::vec2(-0.7071f, -0.7071f)
    };

    for (const glm::vec2& sample : samples) {
        const float x = collisionPoint.x + sample.x * kCheckRadius;
        const float z = collisionPoint.z + sample.y * kCheckRadius;
        if (!terrain.isInsideBoundsWorld(x, z)) {
            return false;
        }

        const float gradient = terrain.sampleGradientMagnitudeWorld(x, z);
        if (gradient > kMaxAllowedGradient) {
            return false;
        }
    }

    return true;
}

void Boat::applyWaveMotion(const Ocean& ocean) {
    // Sample wave height at boat's position
    position.y = ocean.getWaveHeight(position.x, position.z, ocean.time);

    // Get wave normal
    const glm::vec3 waveNormal = ocean.getWaveNormal(position.x, position.z, ocean.time);
    //std::cout << "Wave Normal: (" << waveNormal.x << ", " << waveNormal.y << ", " << waveNormal.z << ")" << std::endl;

    // Get current boat forward direction
    const glm::vec3 currentBoatForward = rotation * glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 horizontalForward(currentBoatForward.x, 0.0f, currentBoatForward.z);
    if (glm::length2(horizontalForward) > 1.0e-8f) {
        horizontalForward = glm::normalize(horizontalForward);
    } else {
        horizontalForward = glm::vec3(0.0f, 0.0f, -1.0f); // Default if boat is pointing straight up/down
    }

    const glm::vec3 targetUp = waveNormal;
    glm::vec3 targetForward = glm::cross(glm::cross(targetUp, horizontalForward), targetUp); // Project horizontalForward onto plane perpendicular to targetUp
    if (glm::length2(targetForward) > 1.0e-8f) {
        targetForward = glm::normalize(targetForward);
    } else {
        targetForward = horizontalForward; // Fallback if projection fails (waveNormal is vertical or horizontalForward is parallel to waveNormal somehow)
    }

    glm::vec3 targetRight = glm::cross(targetForward, targetUp);
    if (glm::length2(targetRight) > 1.0e-8f) {
        targetRight = glm::normalize(targetRight);
    } else {
        targetRight = glm::vec3(1.0f, 0.0f, 0.0f);
    }

    // Create rotation matrix from basis vectors
    glm::mat3 targetRotationMatrix(targetRight, targetUp, -targetForward); // Boat forward is -Z
    glm::quat targetRotation = glm::quat_cast(targetRotationMatrix);

    rotation = glm::slerp(rotation, targetRotation, 0.1f);
}


bool Boat::loadModel(const char* path) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::string warn, err;

    // Extract the directory path from the OBJ file path
    std::string inputfile_dir = "./";
    std::string path_str = path;
    size_t last_slash_pos = path_str.find_last_of("/\\"); // Handle both / and \ path separators
    if (last_slash_pos != std::string::npos) {
        inputfile_dir = path_str.substr(0, last_slash_pos + 1); // Include the last slash
    }

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path, inputfile_dir.c_str()); // Pass mtl_basepath

    if (!warn.empty()) {
        std::cout << "tinyobjloader warning: " << warn << std::endl;
    }

    if (!err.empty()) {
        std::cerr << "tinyobjloader error: " << err << std::endl;
    }

    if (!ret) {
        return false;
    }

    vertices.clear();
    normals.clear();
    texCoords.clear();
    materialIndices.clear(); // Clear material indices
    // Loop through each shape in the OBJ file
    for (const auto& shape : shapes) {
        // Loop over faces(polygon)
        for (size_t f = 0; f < shape.mesh.indices.size() / 3; f++) {
            int material_index = shape.mesh.material_ids[f]; // Get material index for this face

            // Loop over vertices in the face (triangle)
            for (size_t v = 0; v < 3; v++) {
                tinyobj::index_t idx = shape.mesh.indices[3 * f + v];

                tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
                vertices.push_back(glm::vec3(vx, vy, vz));

                if (!attrib.normals.empty()) {
                    tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                    tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                    tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
                    normals.push_back(glm::vec3(nx, ny, nz));
                } else {
                    normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f)); // Default normal if no normals in OBJ
                }

                if (!attrib.texcoords.empty()) {
                    tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                    tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                    texCoords.push_back(glm::vec2(tx, 1.0f - ty)); // Flip V texture coord (OpenGL convention)
                } else {
                    texCoords.push_back(glm::vec2(0.0f, 0.0f)); // Default texcoord if no texcoords in OBJ
                }
                materialIndices.push_back(material_index); // Store material index for this vertex
            }
        }
    }

    if (ret) {

        glm::quat modelRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // Identity quaternion (no rotation initially)

        // **Experiment with these rotations to find the correct orientation!**
        // Example 1: Rotate 180 degrees around Y-axis (to flip the boat horizontally if it's backwards)
        modelRotation = glm::rotate(modelRotation, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate 180 degrees yaw
        modelRotation = glm::rotate(modelRotation, glm::radians(180.f), glm::vec3(1.0f, 0.0f, 0.0f)); // Yaw rotation (example: 0 degrees)
        modelRotation = glm::rotate(modelRotation, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch rotation (example: 0 degrees)
        // Example 2: Rotate around X-axis (Pitch) - Adjust angle as needed
        // modelRotation = glm::rotate(modelRotation, glm::radians(0.0f), glm::vec3(1.0f, 0.0f, 0.0f)); // No pitch rotation in this example

        // Example 3: Rotate around Z-axis (Roll) - Adjust angle as needed
        // modelRotation = glm::rotate(modelRotation, glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // No roll rotation in this example


        // **Apply rotation to vertices AFTER model loading**
        for (glm::vec3& vertex : vertices) {
            vertex = modelRotation * vertex; // Apply rotation to each vertex in model space
        }





        // **Calculate Bounding Box AFTER model loading**
        if (!vertices.empty()) {
            boundingBoxMin = vertices[0]; // Initialize with first vertex
            boundingBoxMax = vertices[0];
            // **Bounding Box Calculation Loop - This is where boundingBoxMin and boundingBoxMax are calculated and set**
            for (const auto& vertex : vertices) {
                boundingBoxMin.x = glm::min(boundingBoxMin.x, vertex.x);
                boundingBoxMin.y = glm::min(boundingBoxMin.y, vertex.y);
                boundingBoxMin.z = glm::min(boundingBoxMin.z, vertex.z);

                boundingBoxMax.x = glm::max(boundingBoxMax.x, vertex.x);
                boundingBoxMax.y = glm::max(boundingBoxMax.y, vertex.y);
                boundingBoxMax.z = glm::max(boundingBoxMax.z, vertex.z);
            }
        }
    }

    return ret;
}
