/*
 * File:        Game.cpp
 * Author:      Tomas Goldmann
 * Date:        2025-03-23
 * Description: This class integrates all items into the game world.
 *
 * Copyright (c) 2025, Brno University of Technology. All rights reserved.
 * Licensed under the MIT.
 */

#include "Game.h"
#include <algorithm>
#include <cmath>

Game* Game::instance = nullptr;

Game::Game() : renderer(), volcanoes(), input(), ocean(200), boat(), camera(), terrain(150, 1.0f), flyToggleKeyWasDown(false), interactKeyWasDown(false), boardBoatCooldown(0.0f)  {
    instance = this;
}

Game::~Game() {
    instance = nullptr;
}

bool Game::init(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1200, 800);
    glutCreateWindow("3D Boat Simulation");

    // **Initialize GLEW AFTER creating the window:**
    if (glewInit() != GLEW_OK) {
        //std::cerr << "GLEW initialization failed!" << std::endl;
        return false;
    }



    glutDisplayFunc(displayCallback);
    glutReshapeFunc(reshapeCallback);
    glutKeyboardFunc(keyboardCallback);
    glutKeyboardUpFunc(keyboardUpCallback);
    glutSpecialFunc(specialCallback);
    glutSpecialUpFunc(specialUpCallback);
    glutMouseFunc(mouseCallback);
    glutMotionFunc(motionCallback);
    glutPassiveMotionFunc(passiveMotionCallback);
    glutTimerFunc(16, timerCallback, 0); // ~60 FPS

    glEnable(GL_DEPTH_TEST);
    renderer.init();
    //int oceanGridSize = 1000; // Default gridSize (you can change this)
    //ocean = Ocean(oceanGridSize); // Pass gridSize to constructor
    ocean.init();

    if (!boat.init("assets/models/boat.obj", "assets/models/boat.jpg")) {
        std::cerr << "Boat initialization failed!" << std::endl;
        return false;
    }
    boat.setScale(0.01f); // Example: Make the boat half its original size




    std::cerr << "Terrain init" << std::endl;

    // Initialize Terrain
    if (!terrain.init(renderer.heightMapTextureID)) {
        std::cerr << "Terrain initialization failed!" << std::endl;
        return false;
    }
    std::cerr << "Camera init" << std::endl;

    camera.init();
    std::cerr << "Input init" << std::endl;

    input.init();
    std::cerr << "Controls: F toggle fly mode | E board/disembark when prompted | +/- set boat speed | on-foot: WASD + Space(jump) | fly: WASD + Space/C | hold RMB to look around" << std::endl;

    const int volcanoCount = 5;
    const std::vector<glm::vec3> peaks = terrain.getHighestPeaks(volcanoCount, 12.0f);
    if (peaks.empty()) {
        std::cerr << "No terrain peaks found for volcano placement!" << std::endl;
        return false;
    }

    volcanoes.clear();
    volcanoes.resize(peaks.size());
    for (std::size_t i = 0; i < peaks.size(); ++i) {
        if (!volcanoes[i].init(8192, 128, 128, 2026u + static_cast<unsigned int>(i * 97u))) {
            std::cerr << "Volcano simulation initialization failed at index " << i << std::endl;
            return false;
        }

        VolcanoSimulation::RenderConfig cfg = volcanoes[i].getRenderConfig();
        cfg.worldX = peaks[i].x;
        cfg.worldZ = peaks[i].z;
        cfg.worldY = peaks[i].y - cfg.coneHeight;
        volcanoes[i].setRenderConfig(cfg);
        volcanoes[i].setCollisionTerrain(terrain, ocean.getSeaLevelOffset());

        std::cerr << "Volcano[" << i << "] anchored at peak: x=" << peaks[i].x
                  << " y=" << peaks[i].y
                  << " z=" << peaks[i].z << std::endl;
    }

    return true;
}

void Game::run() {
    glutMainLoop();
}

void Game::cleanup() {
    renderer.cleanup();
    ocean.cleanup();
    boat.cleanup();
    terrain.cleanup(); // Cleanup terrain

}

void Game::displayCallback() {
    instance->renderer.renderScene(instance->ocean, instance->boat, instance->camera, instance->terrain, instance->volcanoes); // **Pass instance->terrain**
        glutSwapBuffers();
}

void Game::reshapeCallback(int width, int height) {
    instance->renderer.reshape(width, height);
    instance->camera.setAspectRatio(static_cast<float>(width) / height);
}

void Game::keyboardCallback(unsigned char key, int x, int y) {
    instance->input.handleKeyPress(key);
}

void Game::keyboardUpCallback(unsigned char key, int x, int y) {
    instance->input.handleKeyRelease(key);
}

void Game::specialCallback(int key, int x, int y) {
    instance->input.handleSpecialKeyPress(key);
}

void Game::specialUpCallback(int key, int x, int y) {
    instance->input.handleSpecialKeyRelease(key);
}

void Game::mouseCallback(int button, int state, int x, int y) {
    instance->input.handleMouseClick(button, state, x, y);
}

void Game::motionCallback(int x, int y) {
    instance->input.handleMouseMove(x, y);
}

void Game::passiveMotionCallback(int x, int y) {
    instance->input.handleMouseMove(x, y);
}

void Game::updateGame() {
    float deltaTime = 1.0f / 60.0f; // Fixed timestep for simplicity
    instance->input.update();
    instance->boardBoatCooldown = std::max(0.0f, instance->boardBoatCooldown - deltaTime);

    const bool flyToggleDown = instance->input.isKeyDown('f') || instance->input.isKeyDown('F');
    if (flyToggleDown && !instance->flyToggleKeyWasDown) {
        instance->camera.toggleFlyMode();
        std::cerr << "Camera mode: " << instance->camera.getModeName() << std::endl;
    }
    instance->flyToggleKeyWasDown = flyToggleDown;

    const bool interactDown = instance->input.isKeyDown('e') || instance->input.isKeyDown('E');
    const bool interactPressedThisFrame = interactDown && !instance->interactKeyWasDown;
    bool boardedBoatThisFrame = false;
    if (instance->camera.isOnFootMode() && instance->boardBoatCooldown <= 0.0f && interactPressedThisFrame) {
        const glm::vec3 camPos = instance->camera.getPosition();
        const glm::vec3 boatPos = instance->boat.getPosition();
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
            instance->camera.setBoatMode();
            boardedBoatThisFrame = true;
            std::cerr << "Boarded boat: switched to BOAT mode" << std::endl;
        }
    }
    if (instance->camera.isBoatMode() && !boardedBoatThisFrame) {
        instance->boat.update(instance->input, instance->ocean, instance->terrain, deltaTime);
        if (instance->boat.hasTerrainCollision() && instance->boat.canDisembark() && interactPressedThisFrame) {
            instance->camera.startOnFoot(instance->boat.getDisembarkPosition(), instance->terrain);
            instance->boardBoatCooldown = 0.8f;
            std::cerr << "Boat touched shore with low gradient: switched to ON_FOOT mode" << std::endl;
        }
    }
    instance->interactKeyWasDown = interactDown;
    instance->camera.update(instance->input, instance->boat.getPosition(), instance->terrain, deltaTime);
    instance->ocean.update(deltaTime);
    for (VolcanoSimulation& volcano : instance->volcanoes) {
        volcano.stepSIMD(deltaTime);
    }
}

void Game::timerCallback(int value) {
    instance->updateGame();
    glutTimerFunc(16, timerCallback, 0); // Re-register timer
    glutPostRedisplay(); // Request redraw
}
