#pragma once
#include "raylib.h"
#include <string>

class InputHandler {
private:
    char inputBuffer[256];
    int letterCount;

public:
    InputHandler();
    bool UpdateInput(std::string& userInput);  // Returns true if Enter pressed
    const char* GetBuffer() const { return inputBuffer; }
};
