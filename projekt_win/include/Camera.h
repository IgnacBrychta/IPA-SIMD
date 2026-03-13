// Camera.h
#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include "Input.h" // Optional Shader class
#include "Terrain.h"

class Camera {
public:
    enum class Mode {
        BoatOrbit,
        Fly,
        OnFoot
    };

    Camera();
    ~Camera();

    void init();
    void update(const Input& input, const glm::vec3& boatPosition, const Terrain& terrain, float deltaTime);
    void lookAt() const;
    void toggleFlyMode();
    void startOnFoot(const glm::vec3& worldPosition, const Terrain& terrain);
    void setBoatMode();
    bool isFlyMode() const { return mode == Mode::Fly; }
    bool isBoatMode() const { return mode == Mode::BoatOrbit; }
    bool isOnFootMode() const { return mode == Mode::OnFoot; }
    const char* getModeName() const;

    void setAspectRatio(float ratio) { aspectRatio = ratio; }
    glm::vec3 getPosition() const { return position; } // Public getter for position
    glm::mat4 getViewMatrix() const; // **Declare getViewMatrix() method**

        // New: Camera Rotation Control
        void handleMouseInput(const Input& input, float deltaTime);
        void rotateYaw(float angle);
        void rotatePitch(float angle);
private:
    glm::vec3 position;
    glm::vec3 target; // Point to look at
    glm::vec3 up;
    float aspectRatio;
    float fov;
    float nearPlane;
    float farPlane;


    // New: Camera Rotation State
    float yawAngle=0.0f;
    float pitchAngle=0.0f;
    float rotationSpeed;
    float orbitDistance;
    float minOrbitDistance;
    float maxOrbitDistance;
    float targetHeightOffset;
    Mode mode;
    float flyMoveSpeed;
    float flyVerticalSpeed;
    float walkMoveSpeed;
    float maxWalkGradient;
    float onFootEyeHeight;
    glm::vec2 onFootVelocityXZ;
    float onFootVerticalVelocity;
    bool onFootGrounded;
    bool jumpKeyWasDown;
    float walkAcceleration;
    float walkFriction;
    float jumpSpeed;
    float gravity;
    float slideStartGradient;
    float slideAcceleration;
    float maxSlideSpeed;
};

#endif // CAMERA_H
