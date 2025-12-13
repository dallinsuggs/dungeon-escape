#pragma once
#include "raylib.h"
#include <vector>
#include <string>
#include <chrono>
#include <cmath>

class Renderer {
private:
    Texture2D castleTexture;
    int screenWidth, screenHeight;
    std::chrono::steady_clock::time_point startTime;
    float dayProgress;

    // TESTING PURPOSES ONLY COMMENT OUT WHEN DONE
    float simulatedElapsed = 0.0f;
    float timeSpeed = 1.0f; // Speed multiplier for time progression

    // Wrap text in window width, draw multiple lines, update currentY position
    void DrawWrappedText(const char* text, int x, int y, int maxWidth, int fontSize, Color color, int& currentY);

public:
    Renderer(int width, int height);
    void UpdateDayProgress();
    void DrawBackground();
    void DrawTextOverlay(const std::vector<std::string>& displayLines, const char* inputBuffer);
    bool WindowShouldClose();
    ~Renderer();

    // TESTING PURPOSES ONLY COMMENT OUT WHEN DONE
    float GetTimeSpeed() const { return timeSpeed; } // for display
    void SetTimeSpeed(float speed) { timeSpeed = speed; } // to adjust speed
};