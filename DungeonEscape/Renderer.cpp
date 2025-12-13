#include "Renderer.h"
#include <cmath>
#include <sstream>
#include <functional>


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
	castleTexture = LoadTexture("8bit_castle.jpg");
    SetTargetFPS(60);
}

void Renderer::UpdateDayProgress() {
    float delta = GetFrameTime();  // Raylib's per-frame time
    simulatedElapsed += delta * timeSpeed;
    dayProgress = fmod(simulatedElapsed / 1800.0f, 1.0f);  // 1800s = 30min cycle
}

void Renderer::DrawBackground() {
    // Sky gradient (tweaked for misty teal; insert your full phases here if expanded)
    Color skyTop, skyBottom;
    float tint = (dayProgress < 0.5f) ? 1.0f : 0.7f;
    if (dayProgress < 0.25f) {
        skyTop = LerpColor(Color{ 100, 180, 220, 255 }, Color{ 135, 206, 235, 255 }, tint);
        skyBottom = LerpColor(Color{ 150, 200, 230, 255 }, Color{ 173, 216, 230, 255 }, tint);
    }
    else if (dayProgress < 0.5f) {
        float duskT = (dayProgress - 0.25f) / 0.25f;
        skyTop = LerpColor(Color{ 135, 206, 235, 255 }, Color{ 255, 165, 0, 255 }, duskT * tint);
        skyBottom = LerpColor(Color{ 173, 216, 230, 255 }, Color{ 255, 140, 0, 255 }, duskT * tint);
    }
    else if (dayProgress < 0.75f) {
        float nightT = (dayProgress - 0.5f) / 0.25f;
        skyTop = LerpColor(Color{ 255, 165, 0, 255 }, Color{ 25, 25, 112, 255 }, nightT * tint);
        skyBottom = LerpColor(Color{ 255, 140, 0, 255 }, Color{ 0, 0, 139, 255 }, nightT * tint);
    }
    else {
        skyTop = LerpColor(Color{ 0, 0, 50, 255 }, Color{ 25, 25, 112, 255 }, tint);
        skyBottom = LerpColor(Color{ 25, 25, 112, 255 }, Color{ 0, 0, 139, 255 }, tint);
    }
    DrawRectangleGradientV(0, 0, (int)screenWidth, (int)(screenHeight * 0.6f), skyTop, skyBottom);



    //////////////////////////////* CASTLE *//////////////////////////////
    // Castle texture (centered, scaled)
	float scale = 0.2f; // %60 of original size
	int castleWidth = (int)(castleTexture.width * scale);
	int castleHeight = (int)(castleTexture.height * scale);
    int castleX = (screenWidth - castleWidth) / 2;
    int castleY = (int)(screenHeight * 0.2f); // Adjust Y to sit above lake
    Color tintColor;
    if (dayProgress < 0.5f) {
        tintColor = LerpColor(WHITE, GOLD, dayProgress * 0.5f); // Warm day glow
    }
    else {
        tintColor = LerpColor(GOLD, Color{ 200, 200, 255, 255 }, (dayProgress - 0.5f)); // Cool night blue
    }
    DrawTexturePro(castleTexture,
        Rectangle{ 0, 0, (float)castleTexture.width, (float)castleTexture.height },  // Source (full image)
        Rectangle{ (float)castleX, (float)castleY, (float)castleWidth, (float)castleHeight },  // Dest (scaled/pos)
        Vector2{ 0, 0 }, 0.0f, tintColor);



    //////////////////////////////* SUN *//////////////////////////////
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
    UnloadTexture(castleTexture);
    CloseWindow();
}