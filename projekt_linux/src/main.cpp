/*
 * File:        main.cpp
 * Author:      Tomas Goldmann
 * Date:        2025-03-23
 * Description: Main
 *
 * Copyright (c) 2025, Brno University of Technology. All rights reserved.
 * Licensed under the MIT.
 */

#include "Game.h"
#include "VolcanoSim.h"
#include <cstdlib>
#include <cstring>

int main(int argc, char** argv) {
    if (argc > 1 && std::strcmp(argv[1], "--volcano-benchmark") == 0) {
        runVolcanoBenchmark();
        return 0;
    }
    if (argc > 1 && std::strcmp(argv[1], "--volcano-reference") == 0) {
        int frames = 10;
        if (argc > 2) {
            const int parsed = std::atoi(argv[2]);
            frames = (parsed > 0) ? parsed : 1;
        }
        runVolcanoReference(frames);
        return 0;
    }

    Game game;
    if (!game.init(argc, argv)) {
        return 1;
    }
    game.run();
    game.cleanup();
    return 0;
}
