#include "Renderer.h"
#include <cmath>
#include <sstream>
#include <functional>
#define PI 3.14159265f


Renderer::Renderer(int width, int height) : screenWidth(width), screenHeight(height) {
    startTime = std::chrono::steady_clock::now();
    InitWindow(screenWidth, screenHeight, "Dungeon Escape");
    SetTargetFPS(60);
}

void Renderer::UpdateDayProgress() {
    auto currentTime = std::chrono::steady_clock::now();
    float elapsedSeconds = std::chrono::duration<float>(currentTime - startTime).count();
    dayProgress = fmod(elapsedSeconds / 1800.0f, 1.0f);
}

void Renderer::DrawBackground() {
    ClearBackground(SKYBLUE);
    // Sky fade from dawn to dusk
    if (dayProgress > 0.5f) {
        DrawRectangle(0, 0, screenWidth, screenHeight / 2,
            Color{ 255, 165, 0, (unsigned char)(255 * (dayProgress - 0.5f) * 2) });
    }
    // Castle silhouette (bottom)
    DrawRectangle(screenWidth / 2 - 100, screenHeight - 150, 200, 150, GRAY);
    DrawRectangle(screenWidth / 2 - 50, screenHeight - 250, 100, 100, DARKGRAY);
    // Sun arc and color change
    float sunX = screenWidth * (dayProgress * 2.0f);
    if (sunX > screenWidth) sunX = 2 * screenWidth - sunX;
    float sunY = screenHeight * (0.5f - 0.3f * sinf(dayProgress * PI * 2.0f));
    Color sunColor = (dayProgress < 0.3f) ? YELLOW : ((dayProgress > 0.7f) ? ORANGE : GOLD);
    DrawCircle(sunX, sunY, 30, sunColor);
}

void Renderer::DrawTextOverlay(const std::vector<std::string>& displayLines, const char* inputBuffer) {
    DrawRectangle(20, 20, screenWidth - 40, screenHeight - 100, Fade(BLACK, 0.2f));

    // Input prompt first (bottom, safe)
    DrawText("> ", 40, screenHeight - 60, 20, WHITE);
    DrawText(inputBuffer, 80, screenHeight - 60, 20, WHITE);

    // Then wrapped lines (top, with yPos check)
    int yPos = 50;
    const int textAreaWidth = screenWidth - 80;
    for (const auto& line : displayLines) {
        if (yPos < screenHeight - 120) {  // Extra buffer for prompt
            DrawWrappedText(line.c_str(), 40, yPos, textAreaWidth, 16, WHITE, yPos);
        }
    }
}

bool Renderer::WindowShouldClose() {
    return ::WindowShouldClose();
}

void Renderer::DrawWrappedText(const char* text, int x, int y, int maxWidth, int fontSize, Color color, int& currentY) {
    std::string fullText(text);
    std::istringstream iss(fullText);
    std::string word;
    std::string currentLine;
    int lineY = currentY;
    while (iss >> word) {
        std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
        if (MeasureText(testLine.c_str(), fontSize) <= maxWidth) {
            currentLine = testLine;
        }
        else {
            if (!currentLine.empty()) {
                DrawText(currentLine.c_str(), x, lineY, fontSize, color);
                lineY += fontSize + 2;
            }
            currentLine = word;
        }
    }
    if (!currentLine.empty()) {
        DrawText(currentLine.c_str(), x, lineY, fontSize, color);
        lineY += fontSize + 2;
    }
    currentY = lineY + 6;
}

Renderer::~Renderer() {
    CloseWindow();
}