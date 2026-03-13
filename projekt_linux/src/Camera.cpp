/*
 * File:        Camera.cpp
 * Author:      Tomas Goldmann
 * Date:        2025-03-23
 * Description: Camera
 *
 * Copyright (c) 2025, Brno University of Technology. All rights reserved.
 * Licensed under the MIT.
 */


#include "Camera.h"
#include <GL/freeglut.h>
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera() : position(10.0f, 10.0f, 10.0f), target(0.0f, 0.0f, 0.0f), up(0.0f, 1.0f, 0.0f),
                   aspectRatio(1.0f), fov(45.0f), nearPlane(0.1f), farPlane(100.0f),
                   orbitDistance(10.0f), minOrbitDistance(4.0f), maxOrbitDistance(70.0f), targetHeightOffset(0.0f),
                   mode(Mode::BoatOrbit), flyMoveSpeed(16.0f), flyVerticalSpeed(12.0f), walkMoveSpeed(5.5f), maxWalkGradient(0.9f), onFootEyeHeight(1.7f),
                   onFootVelocityXZ(0.0f), onFootVerticalVelocity(0.0f), onFootGrounded(true), jumpKeyWasDown(false),
                   walkAcceleration(24.0f), walkFriction(14.0f), jumpSpeed(7.0f), gravity(18.5f),
                   slideStartGradient(3.5f), slideAcceleration(1.0f), maxSlideSpeed(0.8f) {}

Camera::~Camera() {}

void Camera::init() {
    // Initial camera setup if needed
}

void Camera::update(const Input& input, const glm::vec3& boatPosition, const Terrain& terrain, float deltaTime) {
    handleMouseInput(input, deltaTime);

    if (mode == Mode::OnFoot) {
        const float keyboardLookSpeed = 95.0f * deltaTime;
        if (input.isSpecialKeyDown(GLUT_KEY_LEFT)) {
            yawAngle -= keyboardLookSpeed;
        }
        if (input.isSpecialKeyDown(GLUT_KEY_RIGHT)) {
            yawAngle += keyboardLookSpeed;
        }
        if (input.isSpecialKeyDown(GLUT_KEY_UP)) {
            pitchAngle += keyboardLookSpeed;
        }
        if (input.isSpecialKeyDown(GLUT_KEY_DOWN)) {
            pitchAngle -= keyboardLookSpeed;
        }
        pitchAngle = glm::clamp(pitchAngle, -89.0f, 89.0f);
    }

    glm::vec3 lookDirection;
    lookDirection.x = cos(glm::radians(pitchAngle)) * cos(glm::radians(yawAngle));
    lookDirection.y = sin(glm::radians(pitchAngle));
    lookDirection.z = cos(glm::radians(pitchAngle)) * sin(glm::radians(yawAngle));
    lookDirection = glm::normalize(lookDirection);

    if (mode == Mode::Fly) {
        const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
        const glm::vec3 forward = lookDirection;
        const glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
        const float moveSpeed = flyMoveSpeed * deltaTime;
        const float verticalSpeed = flyVerticalSpeed * deltaTime;

        if (input.isKeyDown('w') || input.isKeyDown('W')) {
            position += forward * moveSpeed;
        }
        if (input.isKeyDown('s') || input.isKeyDown('S')) {
            position -= forward * moveSpeed;
        }
        if (input.isKeyDown('a') || input.isKeyDown('A')) {
            position -= right * moveSpeed;
        }
        if (input.isKeyDown('d') || input.isKeyDown('D')) {
            position += right * moveSpeed;
        }
        if (input.isKeyDown(' ')) {
            position += worldUp * verticalSpeed;
        }
        if (input.isKeyDown('c') || input.isKeyDown('C')) {
            position -= worldUp * verticalSpeed;
        }

        target = position + forward;
        up = glm::normalize(glm::cross(right, forward));
        return;
    }

    if (mode == Mode::OnFoot) {
        const glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
        glm::vec3 flatForward(lookDirection.x, 0.0f, lookDirection.z);
        if (glm::length(flatForward) > 1.0e-5f) {
            flatForward = glm::normalize(flatForward);
        } else {
            flatForward = glm::vec3(0.0f, 0.0f, -1.0f);
        }
        const glm::vec3 right = glm::normalize(glm::cross(flatForward, worldUp));

        glm::vec2 moveDir(0.0f);
        if (input.isKeyDown('w') || input.isKeyDown('W')) {
            moveDir += glm::vec2(flatForward.x, flatForward.z);
        }
        if (input.isKeyDown('s') || input.isKeyDown('S')) {
            moveDir -= glm::vec2(flatForward.x, flatForward.z);
        }
        if (input.isKeyDown('a') || input.isKeyDown('A')) {
            moveDir -= glm::vec2(right.x, right.z);
        }
        if (input.isKeyDown('d') || input.isKeyDown('D')) {
            moveDir += glm::vec2(right.x, right.z);
        }

        const bool hasMoveInput = glm::length(moveDir) > 1.0e-5f;

        if (hasMoveInput) {
            const glm::vec2 desiredVelocity = glm::normalize(moveDir) * walkMoveSpeed;
            glm::vec2 velocityDelta = desiredVelocity - onFootVelocityXZ;
            const float accelScale = onFootGrounded ? 1.0f : 0.35f;
            const float maxVelDelta = walkAcceleration * accelScale * deltaTime;
            const float deltaLen = glm::length(velocityDelta);
            if (deltaLen > maxVelDelta && deltaLen > 1.0e-6f) {
                velocityDelta *= maxVelDelta / deltaLen;
            }
            onFootVelocityXZ += velocityDelta;
        } else {
            // No movement input => never drift automatically.
            onFootVelocityXZ = glm::vec2(0.0f);
        }

        if (hasMoveInput && onFootGrounded) {
            const float speed = glm::length(onFootVelocityXZ);
            if (speed > 1.0e-5f) {
                const float newSpeed = std::max(0.0f, speed - walkFriction * deltaTime);
                onFootVelocityXZ *= newSpeed / speed;
            }
        }

        const float stepX = onFootVelocityXZ.x * deltaTime;
        const float stepZ = onFootVelocityXZ.y * deltaTime;
        if (std::abs(stepX) > 1.0e-6f || std::abs(stepZ) > 1.0e-6f) {
            const glm::vec3 candidate(position.x + stepX, position.y, position.z + stepZ);
            bool canMove = false;
            if (terrain.isInsideBoundsWorld(candidate.x, candidate.z)) {
                const glm::vec2 moveDirection(stepX, stepZ);
                const float directionalGrad = terrain.sampleDirectionalGradientWorld(position.x, position.z, moveDirection);
                if (hasMoveInput) {
                    canMove = std::abs(directionalGrad) <= maxWalkGradient;
                }
            }

            if (canMove) {
                position.x = candidate.x;
                position.z = candidate.z;
            } else {
                if (hasMoveInput) {
                    onFootVelocityXZ *= 0.2f;
                } else {
                    onFootVelocityXZ = glm::vec2(0.0f);
                }
            }
        }

        const float groundY = terrain.sampleHeightWorld(position.x, position.z) + onFootEyeHeight;
        const bool jumpDown = input.isKeyDown(' ');
        if (onFootGrounded) {
            position.y = groundY;
            onFootVerticalVelocity = 0.0f;
            if (jumpDown && !jumpKeyWasDown) {
                onFootVerticalVelocity = jumpSpeed;
                onFootGrounded = false;
            }
        } else {
            onFootVerticalVelocity -= gravity * deltaTime;
            position.y += onFootVerticalVelocity * deltaTime;
            if (position.y <= groundY) {
                position.y = groundY;
                onFootVerticalVelocity = 0.0f;
                onFootGrounded = true;
            }
        }
        jumpKeyWasDown = jumpDown;

        target = position + lookDirection;
        up = worldUp;
        return;
    }

    target = boatPosition;
    position = target - lookDirection * orbitDistance;
    position.y = glm::max(position.y, 2.0f);

    up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 forward = glm::normalize(target - position);
    glm::vec3 right = glm::normalize(glm::cross(forward, up));
    up = glm::normalize(glm::cross(right, forward));

    const int wheel = input.getMouseWheelDelta();
    if (wheel != 0) {
        orbitDistance -= static_cast<float>(wheel) * 1.2f;
        orbitDistance = std::clamp(orbitDistance, minOrbitDistance, maxOrbitDistance);
    }
}

void Camera::lookAt() const {
    gluLookAt(position.x, position.y, position.z,
              target.x, target.y, target.z,
              up.x, up.y, up.z);
}

glm::mat4 Camera::getViewMatrix() const { // Keep `const` method
    // Recalculate camera position and up vector based on yawAngle and pitchAngle


    // **Calculate view matrix based on CURRENT position, target, up - NO MODIFICATION of members in getViewMatrix**
    return glm::lookAt(position, target, up); // Use current position, target, up
}


void Camera::handleMouseInput(const Input& input, float deltaTime) {
    (void)deltaTime;
    if (input.isMouseButtonDown(GLUT_RIGHT_BUTTON)) { // Rotate only when right mouse button is pressed
        float mouseSensitivity = 0.1f; // Adjust sensitivity
        yawAngle   += input.getMouseDeltaX() * mouseSensitivity;
        pitchAngle -= input.getMouseDeltaY() * mouseSensitivity; // Invert vertical mouse motion

        // Clamp pitch angle to prevent camera flipping
        pitchAngle = glm::clamp(pitchAngle, -89.0f, 89.0f);
    }
}

void Camera::rotateYaw(float angle) {
    yawAngle += angle;
}

void Camera::rotatePitch(float angle) {
    pitchAngle += angle;
}

void Camera::toggleFlyMode() {
    if (mode == Mode::Fly) {
        mode = Mode::BoatOrbit;
    } else {
        mode = Mode::Fly;
    }
}

void Camera::startOnFoot(const glm::vec3& worldPosition, const Terrain& terrain) {
    mode = Mode::OnFoot;
    position.x = worldPosition.x;
    position.z = worldPosition.z;
    position.y = terrain.sampleHeightWorld(worldPosition.x, worldPosition.z) + onFootEyeHeight;
    onFootVelocityXZ = glm::vec2(0.0f);
    onFootVerticalVelocity = 0.0f;
    onFootGrounded = true;
    jumpKeyWasDown = false;

    glm::vec3 lookDirection;
    lookDirection.x = cos(glm::radians(pitchAngle)) * cos(glm::radians(yawAngle));
    lookDirection.y = sin(glm::radians(pitchAngle));
    lookDirection.z = cos(glm::radians(pitchAngle)) * sin(glm::radians(yawAngle));
    lookDirection = glm::normalize(lookDirection);
    target = position + lookDirection;
    up = glm::vec3(0.0f, 1.0f, 0.0f);
}

void Camera::setBoatMode() {
    mode = Mode::BoatOrbit;
    onFootVelocityXZ = glm::vec2(0.0f);
    onFootVerticalVelocity = 0.0f;
    onFootGrounded = true;
    jumpKeyWasDown = false;
}

const char* Camera::getModeName() const {
    switch (mode) {
        case Mode::BoatOrbit:
            return "BOAT";
        case Mode::Fly:
            return "FLY";
        case Mode::OnFoot:
            return "ON_FOOT";
        default:
            return "UNKNOWN";
    }
}
