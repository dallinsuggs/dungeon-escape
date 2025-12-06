#pragma once
#include "raylib.h"
#include <vector>
#include <string>
#include <chrono>
#include <cmath>

class Renderer {
private:
    int screenWidth, screenHeight;
    std::chrono::steady_clock::time_point startTime;
    float dayProgress;

    // Wrap text in window width, draw multiple lines, update currentY position
    void DrawWrappedText(const char* text, int x, int y, int maxWidth, int fontSize, Color color, int& currentY);

public:
    Renderer(int width, int height);
    void UpdateDayProgress();
    void DrawBackground();
    void DrawTextOverlay(const std::vector<std::string>& displayLines, const char* inputBuffer);
    bool WindowShouldClose();
    ~Renderer();
};