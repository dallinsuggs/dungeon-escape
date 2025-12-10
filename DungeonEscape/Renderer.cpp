#include "Renderer.h"
#include <cmath>
#include <sstream>
#include <functional>
#define PI 3.14159265f


// Simple color lerp (linear interpolation) for smooth transitions
Color LerpColor(Color a, Color b, float t) {
    return Color{
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}


Renderer::Renderer(int width = 1024, int height = 768) : screenWidth(width), screenHeight(height) {    
    startTime = std::chrono::steady_clock::now();
    InitWindow(screenWidth, screenHeight, "Dungeon Escape");
    SetTargetFPS(60);
}

void Renderer::UpdateDayProgress() {
    float delta = GetFrameTime();  // Raylib's per-frame time
    simulatedElapsed += delta * timeSpeed;
    dayProgress = fmod(simulatedElapsed / 1800.0f, 1.0f);  // 1800s = 30min cycle
}

void Renderer::DrawBackground() {
	// Sky Gradient: Top (zenith) to bottom (horizon), based on dayProgress
    Color skyTop, skyBottom;
    if (dayProgress < 0.25f) { // Early day: Bright blue
        skyTop = Color{135, 206, 235, 255}; // Sky blue
        skyBottom = Color{173, 216, 230, 255}; // Light blue
    } else if (dayProgress < 0.5f) { // Late day: Warm orange
        float duskT = (dayProgress - 0.25f) / 0.25f;
        skyTop = LerpColor(Color{135, 206, 235, 255}, Color{255, 165, 0, 255}, duskT);
        skyBottom = LerpColor(Color{173, 216, 230, 255}, Color{255, 140, 0, 255}, duskT);
    } else if (dayProgress < 0.75f) { // Early night: Purple dusk
        float nightT = (dayProgress - 0.5f) / 0.25f;
        skyTop = LerpColor(Color{255, 165, 0, 255}, Color{25, 25, 112, 255}, nightT);
		skyBottom = LerpColor(Color{255, 140, 0, 255}, Color{0, 0, 139, 255}, nightT);
    } else { // Deep night: Navy with stars
        skyTop = Color{0, 0, 50, 255}; // Midnight blue
        skyBottom = Color{25, 25, 112, 255}; // Indigo
    }
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight * 0.6f, skyTop, skyBottom);

    // Mountains: layered peaks for depth
    // Back layer (distant, lighter gray)


    // Castle: Central, more detailed (towers, roof, windows)
    DrawRectangle(0, screenHeight * 0.6, screenWidth, screenHeight * 0.4, Color{ 34, 139, 34, 128 }); // Green Hill base, green forest, semi-transparent
    DrawRectangle(screenWidth / 2 - 120, screenHeight - 200, 240, 200, DARKGRAY); // Castle: Gray stone towers with dark outlines
    DrawRectangle(screenWidth / 2 - 60, screenHeight - 300, 120, 100, GRAY); // Tower
    DrawRectangle(screenWidth / 2 - 80, screenHeight - 250, 40, 80, BLACK); // Dark accents


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

    // TESTING PURPOSES ONLY COMMENT OUT WHEN DONE
    // Speed indicator (top-right, subtle)
    std::string speedText = "Time: " + std::to_string((int)timeSpeed) + "x";  // Use 'timeSpeed' directly (member var)
    DrawText(speedText.c_str(), screenWidth - 150, 20, 16, (timeSpeed > 1.0f ? RED : GRAY));
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