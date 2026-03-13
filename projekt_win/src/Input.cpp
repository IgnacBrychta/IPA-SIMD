/*
 * File:        Input.cpp
 * Author:      Tomas Goldmann
 * Date:        2025-03-23
 * Description: This class defines the game inputs.
 *
 * Copyright (c) 2025, Brno University of Technology. All rights reserved.
 * Licensed under the MIT.
 */


#include "Input.h"
#include <GL/freeglut.h>
#include <cctype>
#include <cstring>
Input::Input() {
    memset(mouseButtonsDown, false, sizeof(mouseButtonsDown)); // Initialize mouse button states to false
    mouseX = mouseY = lastMouseX = lastMouseY = 0;
    mouseDeltaX = mouseDeltaY = 0;
    mouseWheelDelta = 0;
    mouseWheelDeltaPending = 0;
}
Input::~Input() {}

void Input::init() {}
void Input::update() {
    // Calculate mouse delta
    mouseDeltaX = mouseX - lastMouseX;
    mouseDeltaY = mouseY - lastMouseY;

    // Update last mouse positions for next frame
    lastMouseX = mouseX;
    lastMouseY = mouseY;

    mouseWheelDelta = mouseWheelDeltaPending;
    mouseWheelDeltaPending = 0;
}
void Input::handleKeyPress(unsigned char key) {
    if (std::isalpha(static_cast<unsigned char>(key))) {
        key = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(key)));
    }
    keysDown.insert(key);
}

void Input::handleKeyRelease(unsigned char key) {
    if (std::isalpha(static_cast<unsigned char>(key))) {
        key = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(key)));
    }
    keysDown.erase(key);
}

void Input::handleSpecialKeyPress(int key) {
    specialKeysDown.insert(key);
}

void Input::handleSpecialKeyRelease(int key) {
    specialKeysDown.erase(key);
}

void Input::handleMouseClick(int button, int state, int x, int y) {
    mouseX = x;
    mouseY = y;

    // Prevent large first-frame jump when mouse drag starts.
    if (state == GLUT_DOWN) {
        lastMouseX = x;
        lastMouseY = y;
    }

    if (state == GLUT_DOWN && button == 3) {
        mouseWheelDeltaPending += 1;
        return;
    }
    if (state == GLUT_DOWN && button == 4) {
        mouseWheelDeltaPending -= 1;
        return;
    }

    if (button >= 0 && button < 5) { // Check button index is within range
        mouseButtonsDown[button] = (state == GLUT_DOWN);
    }
}

void Input::handleMouseMove(int x, int y) {
    mouseX = x;
    mouseY = y;
}

bool Input::isKeyDown(unsigned char key) const {
    if (std::isalpha(static_cast<unsigned char>(key))) {
        key = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(key)));
    }
    return keysDown.count(key);
}

bool Input::isSpecialKeyDown(int key) const {
    return specialKeysDown.count(key);
}
