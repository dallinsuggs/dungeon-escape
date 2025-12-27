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
	float scrollOffset = 0.0f; // 0 = window bottom, increases as we scroll up
    Font customFont;

    // Wrap text in window width, draw multiple lines, update currentY position
    void DrawWrappedText(const char* text, int x, int startY, int maxWidth, int fontSize, Color color);

    // TESTING PURPOSES ONLY COMMENT OUT WHEN DONE
    float simulatedElapsed = 0.0f;
    float timeSpeed = 1.0f; // Speed multiplier for time progression
public:
    Renderer(int width, int height);
	int GetWrappedHeight(const char* text, int maxWidth, int fontSize);
    void UpdateDayProgress();
    void DrawBackground();
    void DrawTextOverlay(const std::vector<std::string>& displayLines, const char* inputBuffer);
    bool WindowShouldClose();
    void UpdateScrollInput(); // Call every frame for wheel/arrows
    void SnapToBottom(); // Force window down to newest on output
    ~Renderer();

    // TESTING PURPOSES ONLY COMMENT OUT WHEN DONE
    float GetTimeSpeed() const { return timeSpeed; } // for display
    void SetTimeSpeed(float speed) { timeSpeed = speed; } // to adjust speed
};