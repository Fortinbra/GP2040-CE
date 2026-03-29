/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#pragma once

#include "gamepad.h"

class OutputManager {
public:
    static OutputManager& getInstance();
    void init();
    bool process(Gamepad* gamepad);
    void processAux();

private:
    OutputManager() = default;
    bool _btReady = false;
};
