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
    // Initialize the window + Window Title
    InitWindow(screenWidth, screenHeight, "Dungeon Escape");

	// These lines will remove window decorations and maximize the window
    //SetWindowState(FLAG_WINDOW_UNDECORATED);
    //SetWindowState(FLAG_WINDOW_MAXIMIZED);
    //SetWindowState(FLAG_WINDOW_TOPMOST);

    // Load Window Icon
    windowIcon = LoadImage("window-icon.png");
    SetWindowIcon(windowIcon);


	castleTexture = LoadTexture("foreground.png");
    SetTargetFPS(60);
    scrollOffset = 0.0f;
	dayProgress = 0.0f;
	customFont = LoadFontEx("Jacquard12-Regular.ttf", 96, nullptr, 250);
	SetTextureFilter(customFont.texture, TEXTURE_FILTER_BILINEAR); // smooth when scaled
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
        if (MeasureTextEx(customFont, testLine.c_str(), (float)fontSize, 2.0f).x <= maxWidth) {
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
                DrawTextEx(customFont, currentLine.c_str(), Vector2{ (float)x, (float)lineY }, (float)fontSize, 2.0f, color);  // 2.0f spacing = open feel
                lineY += fontSize + spacing;  // Advance for next line
            }
            currentLine = word;
        }
    }
    if (!currentLine.empty()) {
        DrawTextEx(customFont, currentLine.c_str(), Vector2{ (float)x, (float)lineY }, (float)fontSize, 2.0f, color);  // 2.0f spacing = open feel
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
    // Sky gradient 
    float cycle = dayProgress; // 0.0  = sunrise/start, 0.5 = sunset/midnioght, 1.0 = next sunrise

    Color dayTop = Color{ 100, 180, 220, 255 }; // Morning bright sky
    Color dayBottom = Color{ 150, 200, 230, 255 };
    Color duskTop = Color{ 255, 165, 100, 255 }; // Warm sunset
    Color duskBottom = Color{ 255, 140, 80, 255 };
    Color nightTop = Color{ 10, 10, 50, 255 }; // Deep midnight
    Color nightBottom = Color{ 0, 0, 30, 255 };
    Color dawnTop = Color{ 255, 200, 150, 255 }; // Soft dawn glow
    Color dawnBottom = Color{ 200, 150, 120, 255 };

    // Determine sky colors based on time of day
    Color skyTop, skyBottom;
    if (cycle < 0.25f) {
        // Morning > Day (bright)
        float tint = cycle / 0.25f;
        skyTop = LerpColor(dawnTop, dayTop, tint);
        skyBottom = LerpColor(dawnBottom, dayBottom, tint);
    }
    else if (cycle < 0.5f) {
        // Day > Dusk (warm)
        float tint = (cycle - 0.25f) / 0.25f;
        skyTop = LerpColor(dayTop, duskTop, tint);
        skyBottom = LerpColor(dayBottom, duskBottom, tint);
    }
    else if (cycle < 0.75f) {
        // Dusk > Night (darken)
        float tint = (cycle - 0.5f) / 0.25f;
        skyTop = LerpColor(duskTop, nightTop, tint);
        skyBottom = LerpColor(duskBottom, nightBottom, tint);
    }
    else {
        // Night > Dawn (lighten)
        float tint = (cycle - 0.75f) / 0.25f;
        skyTop = LerpColor(nightTop, dawnTop, tint);
        skyBottom = LerpColor(nightBottom, dawnBottom, tint);
    }
    // Dim during full night
    float nightDim = (cycle >= 0.5f && cycle < 0.75f) ? 0.8f : 1.0f;
    skyTop = Fade(skyTop, nightDim);
    skyBottom = Fade(skyBottom, nightDim);
    DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight(), skyTop, skyBottom);


    //////////////////////////////* SUN *//////////////////////////////
    // Sun arc and color change
    float sunX = GetScreenWidth() * (dayProgress * 2.0f);
    if (sunX > GetScreenWidth()) sunX = 2 * GetScreenWidth() - sunX;
    float sunY = GetScreenHeight() * (0.5f - 0.3f * sinf(dayProgress * PI * 2.0f));
    Color sunColor = (dayProgress < 0.3f) ? YELLOW : ((dayProgress > 0.7f) ? ORANGE : GOLD);
    DrawCircle(sunX, sunY, 30, sunColor);


    //////////////////////////////* MOON *//////////////////////////////
    // Moon arc and color change (opposite sun)
    float moonProgress = fmod(dayProgress + 0.5f, 1.0f); // opposite side of day
    float moonX = GetScreenWidth() * (moonProgress * 2.0f);
    if (moonX > GetScreenWidth()) moonX = 2 * GetScreenWidth() - moonX; // Mirror on right side
    float moonY = GetScreenHeight() * (0.5f + 0.3f * sinf(dayProgress * PI * 2.0f)); // Same arc height as sun, inverted

    // Smooth height transition: 0 at horizon, 1 at peak
    float heightSin = sinf(moonProgress * PI); // -1 to 1 over arc
    float heightFactor = (heightSin + 1.0f) / 2.0f; // 0 (low) to 1 (peak) to 0

    // Moon color: bright white at top, warm orange-red near horizon
    Color horizonMoon = Color{ 255, 200, 140, 255 }; // tweak redder: {255, 160, 100}
    Color peakMoon = WHITE;
    Color moonColor = LerpColor(horizonMoon, peakMoon, heightFactor);

    // Scale radius of moon with window for consistency
    float moonRadius = 20.0f * (GetScreenHeight() / 768.0f); // Base radius 20 at 768 height

    // Draw moon
    float glowRadius = moonRadius * 1.7f;
    DrawCircle((int)moonX, (int)moonY, (int)glowRadius, Fade(moonColor, 0.2f)); // Soft halo
    DrawCircle((int)moonX, (int)moonY, (int)moonRadius, moonColor); // Main moon


    //////////////////////////////* CASTLE *//////////////////////////////
    // Foreground image, sun passes behind all of this but in front of sky
    // Foreground image (your edited one) - cover scaling to fill screen
    if (castleTexture.id > 0) {
        float fgScale = std::max((float)GetScreenWidth() / castleTexture.width,
            (float)GetScreenHeight() / castleTexture.height);
        float fgWidth = castleTexture.width * fgScale;
        float fgHeight = castleTexture.height * fgScale;
        float fgX = (GetScreenWidth() - fgWidth) / 2.0f;
        float fgY = (GetScreenHeight() - fgHeight) / 2.0f;

        Color fgTint = WHITE;
        if (dayProgress > 0.5f) fgTint = LerpColor(WHITE, Color{ 180, 180, 240, 255 }, (dayProgress - 0.5f) * 2);

        DrawTexturePro(castleTexture,
            Rectangle{ 0, 0, (float)castleTexture.width, (float)castleTexture.height },  // source
            Rectangle{ fgX, fgY, fgWidth, fgHeight },  // dest
            Vector2{ 0, 0 },  // origin (top-left pivot)
            0.0f,           // rotation
            fgTint);        // tint
    }

    //////////////////////////////* CASTLE LANTERNS AT NIGHT *//////////////////////////////
    // Lanterns glow during night, tied to foreground position
    if (dayProgress >= 0.35f && dayProgress < 2.0f)
    {
        float nightIntensity = 1.0f - fabsf(dayProgress - 0.625f) * 4.0f; // Fade in/out, peaks at 0.625, midnight
        float pulseBase = 0.9f + 0.2f * sinf(simulatedElapsed * 4.0f); // Pulsing effect

        // Reuse the same scaling/position as foreground (from above)
        float fgScale = std::max((float)GetScreenWidth() / castleTexture.width,
            (float)GetScreenHeight() / castleTexture.height);
        float fgX = (GetScreenWidth() - castleTexture.width * fgScale) / 2.0f;
        float fgY = (GetScreenHeight() - castleTexture.height * fgScale) / 2.0f;

        // Helper: position relative to image (0-1 inside texture), converted to screen
        auto DrawGlowLightRelative = [&](float relX, float relY, float radiusScale = 1.0f) {
            // relX/relY = fraction inside the original image (e.g., 0.5,0.5 = center of castle)
            float x = fgX + relX * (castleTexture.width * fgScale);
            float y = fgY + relY * (castleTexture.height * fgScale);
            float baseRadius = 8.0f * fgScale * radiusScale;  // Scales with image

            static float phases[20] = { 0 };
            static int lightCount = 0;
            float phase = phases[lightCount % 20];
            if (phase == 0.0f) phase = phases[lightCount % 20] = (float)rand() / RAND_MAX * PI * 2;
            lightCount++;


            float flickerSpeed = 2.0f + (lightCount % 4) * 0.5f;  // 2-3.5 Hz per light
            float flicker = 0.85f + 0.15f * sinf(simulatedElapsed * flickerSpeed + phase);
            float intensity = nightIntensity * pulseBase * flicker;

            Color warmCore = Color{ 255, 180, 80, 255 };
            Color warmGlow = Color{ 255, 160, 60, 255 };

            DrawCircle((int)x, (int)y, (int)(baseRadius * 2.5f), Fade(warmGlow, 0.1f * intensity));  // Outer
            DrawCircle((int)x, (int)y, (int)(baseRadius * 1.6f), Fade(warmGlow, 0.2f * intensity));  // Mid
            DrawCircle((int)x, (int)y, (int)baseRadius, Fade(warmCore, intensity));                // Core
            };

            // Example lantern positions on castle (as % of screen)
            // Use a lamda counter for a few lights
            // MAX: 10-15 lights
            // (Run, see where they land, adjust percentX/Y—e.g., 0.3 = 30% from left, 0.4 = 40% from top)
            DrawGlowLightRelative(0.2f, 0.5f, 2.5f); // Far left mid tower outside castle walls
            DrawGlowLightRelative(0.33f, 0.54f, 2.5f); // Far left castle window
            DrawGlowLightRelative(0.425f, 0.47f, 2.0f); // Upper left castle window
            DrawGlowLightRelative(0.465f, 0.675f, 2.0f); // Left castle door tower
            DrawGlowLightRelative(0.557f, 0.675f, 2.0f); // Right castle door tower
            DrawGlowLightRelative(0.565f, 0.52f, 2.0f); // Upper right castle window
            DrawGlowLightRelative(0.64f, 0.23f, 2.3f); // Tall upper right tower inside castle walls
            DrawGlowLightRelative(0.86f, 0.61f, 2.5f); // Far right wide tower on castle walls
    }
}



void Renderer::DrawTextOverlay(const std::vector<std::string>& displayLines, const char* inputBuffer) {
    // Semi-transparent background for text area
    // DrawRectangle(20, 20, GetScreenWidth() - 40, GetScreenHeight() - 100, Fade(BLACK, 0.2f));
    //////////////////////////////* TEXT AREA *//////////////////////////////
    Color panelColor = Fade(BLACK, 0.65f); // Semi-transparent black
    if (dayProgress > 0.5f) {
        panelColor = Fade(Color{ 15, 10, 35, 255 }, 0.7f); // Indigo at night
    }
    DrawRectangle(20, 20, GetScreenWidth() - 40, GetScreenHeight() - 100, panelColor);
    DrawRectangleLinesEx(Rectangle{ 18, 18, (float)GetScreenWidth() - 36, (float)GetScreenHeight() - 96 }, 2.0f, Fade(WHITE, 0.15f));
    // Input prompt with bg overdraw for clean clears/backspace
    int inputY = GetScreenHeight() - 60;
    int inputHeight = 24; // Font 20 + padding
    Color inputBg = Fade(BLACK, 0.7f); // solid for input area
    if (dayProgress > 0.5f) inputBg = Fade(Color{ 20, 15, 40, 255 }, 0.75f); // indigo at night
    DrawRectangle(30, inputY - 2, GetScreenWidth() - 60, inputHeight, inputBg); // Covers prompt + buffer space
    // DRAW INPUT PROMPT
    float promptSize = 36.0f; // Big and charming ♡
    DrawTextEx(customFont, "> ", Vector2{ 40.0f, (float)inputY }, promptSize, 2.0f, WHITE);
    DrawTextEx(customFont, inputBuffer ? inputBuffer : "", Vector2{ 80.0f, (float)inputY }, promptSize, 2.0f, WHITE);
    // Text area setup
    int textAreaTop = 50;
    int textAreaBottom = GetScreenHeight() - 120; // Buffer for input
    int availableHeight = textAreaBottom - textAreaTop;
    int textAreaWidth = GetScreenWidth() - 80;
    int fontSize = 32;
    int lineSpacing = fontSize + 4;
    // Precompute heights for permanent history lines
    std::vector<int> lineHeights;
    int totalHeight = 0;
    for (const auto& line : displayLines) {
        int h = GetWrappedHeight(line.c_str(), textAreaWidth, fontSize);
        lineHeights.push_back(h);
        totalHeight += h + 10;  // We now use +10 consistently (your better spacing!)
    }
    if (!displayLines.empty()) totalHeight -= 10; // Avoid extra gap at end

    // Scrolling setup
    float maxScroll = std::max(0.0f, (float)totalHeight - (float)availableHeight);
    if (scrollOffset > maxScroll) scrollOffset = maxScroll;

    // Viewport offset: 0 = newest at bottom
    int viewportTopContent = (int)((float)totalHeight - (float)availableHeight - scrollOffset);
    viewportTopContent = std::max(0, viewportTopContent);

    int currentContentY = 0;

    // FIRST: Draw permanent history (all finished text)
    for (size_t i = 0; i < displayLines.size(); ++i) {
        int h = lineHeights[i];
        if (currentContentY + h > viewportTopContent && currentContentY < viewportTopContent + availableHeight) {
            int drawStartY = textAreaTop + (currentContentY - viewportTopContent);
            DrawWrappedText(displayLines[i].c_str(), 40, drawStartY, textAreaWidth, fontSize, WHITE);
        }
        currentContentY += h + 10;  // Consistent spacing
    }

    // SECOND: If typing is active, draw the typing animation ON TOP of the history
    if (typingActive) {
        for (size_t i = 0; i < animLines.size(); ++i) {
            int visibleChars = (int)(animLines[i].length() * animProgress[i]);
            std::string partial = animLines[i].substr(0, visibleChars);

            // Blinking cursor (faster and cuter ♡)
            if (animProgress[i] < 1.0f && fmod(typingTimer * 8.0f, 1.0f) > 0.5f) {
                partial += "_";
            }

            int h = GetWrappedHeight(partial.c_str(), textAreaWidth, fontSize);
            if (currentContentY + h > viewportTopContent && currentContentY < viewportTopContent + availableHeight) {
                int drawStartY = textAreaTop + (currentContentY - viewportTopContent);
                DrawWrappedText(partial.c_str(), 40, drawStartY, textAreaWidth, fontSize, WHITE);
            }
            // Use GetWrappedHeight to compute actual height in pixels
            int wrappedHeight = GetWrappedHeight(partial.c_str(), textAreaWidth, fontSize);
            currentContentY += wrappedHeight + 10;  // 10 = spacing between entries
        }
    }

    // TESTING PURPOSES ONLY COMMENT OUT WHEN DONE
    std::string speedText = "Time: " + std::to_string((int)timeSpeed) + "x";
    DrawText(speedText.c_str(), GetScreenWidth() - 150, 20, 16, (timeSpeed > 1.0f ? RED : GRAY));
    // Debug
    // DrawText(TextFormat("Offset: %.0f / Max: %.0f", scrollOffset, maxScroll), GetScreenWidth() - 300, 40, 16, YELLOW);
}

// Update scrollOffset based on mouse wheel and arrow keys
void Renderer::UpdateScrollInput() {
	// Mouse wheel / touchpad (positive = up, negative = down)
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        scrollOffset += wheel * WHEEL_SPEED;
    };

    //CHANGE THIS TO GET HISTORY OF INPUTS NOT SCROLL UP/DOWN
	// Arrow keys (hold to scroll continuously) 
 //   if (IsKeyDown(KEY_UP)) {
 //       scrollOffset += ARROW_SPEED * GetFrameTime(); // UP
 //   }
 //   if (IsKeyDown(KEY_DOWN)) {
 //       scrollOffset -= ARROW_SPEED * GetFrameTime(); // DOWN
 //   }
}


// Typing animation update per frame
void Renderer::StartTypingAnimation(const std::vector<std::string>& lines) {
    animLines = lines;
    animProgress.clear();
    for (auto& line : animLines) animProgress.push_back(0.0f);
    typingTimer = 0.0f;
    typingActive = !lines.empty();
}

// Update typing animation progress
void Renderer::UpdateTypingAnimation(float deltaTime) {
    if (!typingActive || animLines.empty()) {
        typingActive = false;
        return;
    }

    float charsPerSecond = typingSpeed;
    float charsThisFrame = charsPerSecond * deltaTime;

    bool stillTyping = false;

    for (size_t i = 0; i < animLines.size(); ++i) {
        if (animLines[i].empty()) {
            animProgress[i] = 1.0f;
            continue;
        }
        if (animProgress[i] >= 1.0f) continue;

        animProgress[i] += charsThisFrame / (float)animLines[i].length();

        if (animProgress[i] < 1.0f) {
            stillTyping = true;
            break;                    // type one line at a time
        }
        else {
            animProgress[i] = 1.0f;   // finish this line
        }
    }

    if (!stillTyping) {
        typingActive = false;
    }
}


// Snap scroll to bottom (newest output)
void Renderer::SnapToBottom(int paddingLines) {
    // Compute total height of the padding in pixels
    int fontSize = 32;           // match text font size
    int lineSpacing = fontSize + 4; // spacing between lines
    int paddingPixels = paddingLines * lineSpacing;

    // dynamic padding for currently typing lines
    if (typingActive && !animLines.empty())
    {
        int typingHeight = 0;
        for (const auto& line : animLines) {
            typingHeight += GetWrappedHeight(line.c_str(), GetScreenWidth() - 80, fontSize);
        }
        paddingPixels += typingHeight = 30; // 30px buffer for cursor to stay visible
    }
    // Leave paddingPixels above the bottom
    scrollOffset = -paddingPixels;
}


bool Renderer::WindowShouldClose() {
    return ::WindowShouldClose();
}

Renderer::~Renderer() {
    UnloadTexture(castleTexture);
    UnloadFont(customFont);
    UnloadImage(windowIcon);
    CloseWindow();
}