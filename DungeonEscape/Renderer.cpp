#include "Renderer.h"
#include <cmath>
#include <sstream>
#include <functional>
#include <algorithm>

// CONSTANTS
const float WHEEL_SPEED = 80.0f; // Pixels per wheel notch
const float ARROW_SPEED = 300.0f; // Pixels per second for arrow keys



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
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(screenWidth, screenHeight, "Dungeon Escape");
	castleTexture = LoadTexture("8bit_castle.jpg");
    SetTargetFPS(60);
    scrollOffset = 0.0f;
}

// Computes height for one wrapped line
int Renderer::GetWrappedHeight(const char* text, int maxWidth, int fontSize) {
    std::string fullText(text);
    std::istringstream iss(fullText);
    std::string word;
    std::string currentLine;
    int numVisualLines = 0;
    int spacing = 2;  // Consistent with draw logic

    while (iss >> word) {
        std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
        if (MeasureText(testLine.c_str(), fontSize) <= maxWidth) {
            currentLine = testLine;
        }
        else {
            if (!currentLine.empty()) {
                numVisualLines++;  // Count the line that wrapped
            }
            currentLine = word;
        }
    }
    if (!currentLine.empty()) {
        numVisualLines++;  // Count the final line (was missing!)
    }

    // Height: lines * fontSize + (lines-1) * spacing (no extra after last)
    return numVisualLines * fontSize + std::max(0, numVisualLines - 1) * spacing;
}

// Draw wrapped text and update currentY position
void Renderer::DrawWrappedText(const char* text, int x, int startY, int maxWidth, int fontSize, Color color) {
    std::string fullText(text);
    std::istringstream iss(fullText);
    std::string word;
    std::string currentLine;
    int lineY = startY;
    int spacing = 2;

    while (iss >> word) {
        std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
        if (MeasureText(testLine.c_str(), fontSize) <= maxWidth) {
            currentLine = testLine;
        }
        else {
            if (!currentLine.empty()) {
                DrawText(currentLine.c_str(), x, lineY, fontSize, color);
                lineY += fontSize + spacing;  // Advance for next line
            }
            currentLine = word;
        }
    }
    if (!currentLine.empty()) {
        DrawText(currentLine.c_str(), x, lineY, fontSize, color);
    }
}

// Update dayProgress based on simulated time progression
void Renderer::UpdateDayProgress() {
    float delta = GetFrameTime();  // Raylib's per-frame time
    simulatedElapsed += delta * timeSpeed;
    dayProgress = fmod(simulatedElapsed / 1800.0f, 1.0f);  // 1800s = 30min cycle
}

// Draw background with Raylib
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
    DrawRectangleGradientV(0, 0, GetScreenWidth(), (int)(GetScreenHeight() * 0.6f), skyTop, skyBottom);



    //////////////////////////////* CASTLE *//////////////////////////////
    // Castle texture (centered, scaled)
	float scale = 0.2f; // %60 of original size
	int castleWidth = (int)(castleTexture.width * scale);
	int castleHeight = (int)(castleTexture.height * scale);
    int castleX = (GetScreenWidth() - castleWidth) / 2;
    int castleY = (int)(GetScreenHeight() * 0.2f); // Adjust Y to sit above lake
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
    float sunX = GetScreenWidth() * (dayProgress * 2.0f);
    if (sunX > GetScreenWidth()) sunX = 2 * GetScreenWidth() - sunX;
    float sunY = GetScreenHeight() * (0.5f - 0.3f * sinf(dayProgress * PI * 2.0f));
    Color sunColor = (dayProgress < 0.3f) ? YELLOW : ((dayProgress > 0.7f) ? ORANGE : GOLD);
    DrawCircle(sunX, sunY, 30, sunColor);
}


void Renderer::DrawTextOverlay(const std::vector<std::string>& displayLines, const char* inputBuffer) {
    // Semi-transparent background for text area
    DrawRectangle(20, 20, GetScreenWidth() - 40, GetScreenHeight() - 100, Fade(BLACK, 0.2f));

    // Input prompt with bg overdraw for clean clears/backspace
    int inputY = GetScreenHeight() - 60;
    int inputHeight = 24; // Font 20 + padding
    Color inputBg = Fade(SKYBLUE, 0.3f); // Blend with bottom of sky
    DrawRectangle(30, inputY - 2, GetScreenWidth() - 60, inputHeight, inputBg); // Covers prompt + buffer space
    DrawText("> ", 40, inputY, 20, WHITE);
    DrawText(inputBuffer ? inputBuffer : "", 80, inputY, 20, WHITE); // Always draw input

    // Text area setup
    int textAreaTop = 50;
    int textAreaBottom = GetScreenHeight() - 120; // Buffer for input
    int availableHeight = textAreaBottom - textAreaTop;
    int textAreaWidth = GetScreenWidth() - 80;
    int fontSize = 16;
    int lineSpacing = fontSize + 4;

    // Precompute heights (your code unchanged up to here)
    std::vector<int> lineHeights;
    int totalHeight = 0;
    for (const auto& line : displayLines) {
        int h = GetWrappedHeight(line.c_str(), textAreaWidth, fontSize);
        lineHeights.push_back(h);
        totalHeight += h + 2;
    }
    if (!displayLines.empty()) totalHeight -= 2;  // Avoid double-gap

    // Scrolling setup (replaces old auto-scroll)
    float maxScroll = std::max(0.0f, (float)totalHeight - (float)availableHeight);

    // Manual clamp scrollOffset
    if (scrollOffset < 0.0f) scrollOffset = 0.0f;
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;

    // Viewport offset: 0 = newest at bottom
    int viewportTopContent = (int)((float)totalHeight - (float)availableHeight - scrollOffset);
    viewportTopContent = std::max(0, viewportTopContent);  // Prevent over-scroll top

    // Then draw visible lines
    int currentContentY = 0;
    for (size_t i = 0; i < displayLines.size(); ++i) {
        int h = lineHeights[i];
        // Draw if this line overlaps the viewport
        if (currentContentY + h > viewportTopContent && currentContentY < viewportTopContent + availableHeight) {
            // This line is at least partially visible
            int drawStartY = textAreaTop + (currentContentY - viewportTopContent);
            DrawWrappedText(displayLines[i].c_str(), 40, drawStartY, textAreaWidth, fontSize, WHITE);
        }
        currentContentY += h + 2; // GAP BETWEEN MESSAGES
    }

    // TESTING PURPOSES ONLY COMMENT OUT WHEN DONE
    // Speed indicator (top-right, subtle)
    std::string speedText = "Time: " + std::to_string((int)timeSpeed) + "x";  // Use 'timeSpeed' directly (member var)
    DrawText(speedText.c_str(), GetScreenWidth() - 150, 20, 16, (timeSpeed > 1.0f ? RED : GRAY));

    // Debug
    //DrawText(TextFormat("Offset: %.0f / Max: %.0f", scrollOffset, maxScroll), GetScreenWidth() - 300, 40, 16, YELLOW);
}

// Update scrollOffset based on mouse wheel and arrow keys
void Renderer::UpdateScrollInput() {
	// Mouse wheel / touchpad (positive = up, negative = down)
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        scrollOffset += wheel * WHEEL_SPEED;
    };

	// Arrow keys (hold to scroll continuously)
    if (IsKeyDown(KEY_UP)) {
        scrollOffset += ARROW_SPEED * GetFrameTime(); // UP
    }
    if (IsKeyDown(KEY_DOWN)) {
        scrollOffset -= ARROW_SPEED * GetFrameTime(); // DOWN
    }
}

// Snap scroll to bottom (newest output)
void Renderer::SnapToBottom() {
    scrollOffset = 0.0f;
}


bool Renderer::WindowShouldClose() {
    return ::WindowShouldClose();
}

Renderer::~Renderer() {
    UnloadTexture(castleTexture);
    CloseWindow();
}