#include "InputHandler.h"

InputHandler::InputHandler() : letterCount(0) {
    inputBuffer[0] = '\0';
}

bool InputHandler::UpdateInput(std::string& userInput) {
    bool enterPressed = false;

    // Printable chars
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 125 && letterCount < 255) {
            inputBuffer[letterCount++] = (char)key;
            inputBuffer[letterCount] = '\0';
        }
        key = GetCharPressed();
    }

    // Special keys
    if (IsKeyPressed(KEY_ENTER) && letterCount > 0) {
        userInput = std::string(inputBuffer);
        letterCount = 0;
        inputBuffer[0] = '\0';
        enterPressed = true;
    }
    else if (IsKeyPressed(KEY_BACKSPACE) && letterCount > 0) {
        letterCount--;
        inputBuffer[letterCount] = '\0';
    }

    return enterPressed;
}