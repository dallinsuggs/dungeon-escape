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
    Image windowIcon;

    // Wrap text in window width, draw multiple lines, update currentY position
    void DrawWrappedText(const char* text, int x, int startY, int maxWidth, int fontSize, Color color);

    // Typing animation
	//float typingSpeed = 35.0f; // characters per second SLOW (UGH)
    float typingSpeed = 90.0f; // characters per second FAST (YAY!)
    std::vector<std::string> animLines; // lines being typed
	std::vector<float> animProgress; // Progress per line (0.0 to 1.0)
    float typingTimer = 0.0f;
    bool typingActive = false;

    // TESTING PURPOSES ONLY COMMENT OUT WHEN DONE
    float simulatedElapsed = 0.0f;
    float timeSpeed = 1.0f; // Speed multiplier for time progression
public:
    Renderer(int width, int height);
	int GetWrappedHeight(const char* text, int maxWidth, int fontSize);
    void UpdateDayProgress();
    void DrawBackground();
    void DrawTextOverlay(const std::vector<std::string>& displayLines, const char* inputBuffer);
    void SnapToBottom(int paddingLines = 4);
    bool WindowShouldClose();
    void UpdateScrollInput(); // Call every frame for wheel/arrows
    ~Renderer();

    // Get the current point/time in the day
    float GetDayProgress() const { return dayProgress; }

	// Typing animation controls
    void StartTypingAnimation(const std::vector<std::string>& lines);
    void UpdateTypingAnimation(float deltaTime);
    bool IsTypingActive() const { return typingActive; }
    bool IsTypingDone() const { return !typingActive; }
    const std::vector<std::string>& GetLastTypedLines() const { return animLines; }

    // TESTING PURPOSES ONLY COMMENT OUT WHEN DONE
    float GetTimeSpeed() const { return timeSpeed; } // for display
    void SetTimeSpeed(float speed) { timeSpeed = speed; } // to adjust speed
    void SetDayProgress(float value); // set day progress automtically for testing

    // Game State classes
    enum class GameState {
        MENU,
        HOW_TO_PLAY,
        PLAYING,
        DEAD,
        WIN
    };

    // Menus / UI
    void DrawMenuScreen(int selectedIndex);
    void DrawHowToPlayScreen();
    void DrawEndScreen(bool won);
};