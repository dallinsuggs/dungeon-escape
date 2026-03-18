#include "CommandParser.hpp"
#include "Player.hpp"
#include "Item.hpp"
#include "Room.hpp"
#include "FileManager.hpp"
#include <unordered_map>
#include <iostream>
#include "Renderer.h"
#include <chrono>
#include <sstream>
#include <vector>
#include <functional>
#include <cmath>
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/error/en.h"
#include <fstream>
#include "InputHandler.h"
#include "raylib.h"

#ifdef _WIN32
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

std::vector<std::string> captureOutput(std::function<void()> func) {
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    func();
    std::cout.rdbuf(old);
    std::istringstream iss(oss.str());
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(iss, line)) {
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

//////////////////////* MAIN */////////////////////
int main() {
    SetTraceLogLevel(LOG_ALL);
    SetTraceLogCallback([](int logType, const char* text, va_list args) {
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), text, args);
        printf("%s\n", buffer);
        });

    Renderer renderer(1024, 768);
    Renderer::GameState state = Renderer::GameState::MENU;

    // Menu state
    int menuIndex = 0;
    const int MENU_ITEMS = 3;

    // Outer loop — keeps window alive across all screens
    while (!renderer.WindowShouldClose()) {
        renderer.UpdateDayProgress();

        // ── MENU ─────────────────────────────────────────────────────────────
        if (state == Renderer::GameState::MENU) {
            // Keyboard navigation
            if (IsKeyPressed(KEY_UP))    menuIndex = (menuIndex - 1 + MENU_ITEMS) % MENU_ITEMS;
            if (IsKeyPressed(KEY_DOWN))  menuIndex = (menuIndex + 1) % MENU_ITEMS;

            // Mouse hover navigation
            // (DrawMenuScreen handles highlight, we just need click detection here)
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsKeyPressed(KEY_ENTER)) {
                if (menuIndex == 0) state = Renderer::GameState::PLAYING;
                if (menuIndex == 1) state = Renderer::GameState::HOW_TO_PLAY;
                if (menuIndex == 2) break; // Quit
            }

            // Also allow mouse click to select hovered item
            // We replicate the hit areas from DrawMenuScreen
            int panelW = 500, panelH = 400;
            int panelX = GetScreenWidth() / 2 - panelW / 2;
            int panelY = GetScreenHeight() / 2 - panelH / 2;
            int optionStartY = panelY + 130;
            int optionSpacing = 70;
            int optionSize = 36;
            const char* options[] = { "New Game", "How to Play", "Quit" };
            for (int i = 0; i < MENU_ITEMS; i++) {
                float optW = MeasureText(options[i], optionSize);
                float optX = (float)(GetScreenWidth() / 2) - optW / 2.0f;
                float optY = (float)(optionStartY + i * optionSpacing);
                Rectangle optRect = { optX, optY, optW, (float)optionSize };
                if (CheckCollisionPointRec(GetMousePosition(), optRect)) {
                    menuIndex = i;
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        if (i == 0) state = Renderer::GameState::PLAYING;
                        if (i == 1) state = Renderer::GameState::HOW_TO_PLAY;
                        if (i == 2) break;
                    }
                }
            }

            BeginDrawing();
            renderer.DrawBackground();
            renderer.DrawMenuScreen(menuIndex);
            EndDrawing();
            continue;
        }

        // ── HOW TO PLAY ───────────────────────────────────────────────────────
        if (state == Renderer::GameState::HOW_TO_PLAY) {
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_BACKSPACE)) {
                state = Renderer::GameState::MENU;
            }
            BeginDrawing();
            renderer.DrawBackground();
            renderer.DrawHowToPlayScreen();
            EndDrawing();
            continue;
        }

        // ── END SCREENS ───────────────────────────────────────────────────────
        if (state == Renderer::GameState::DEAD || state == Renderer::GameState::WIN) {
            bool won = (state == Renderer::GameState::WIN);
            if (IsKeyPressed(KEY_ENTER)) {
                menuIndex = 0;
                state = Renderer::GameState::PLAYING;
            }
            if (IsKeyPressed(KEY_ESCAPE)) break; // Quit

            BeginDrawing();
            renderer.DrawBackground();
            renderer.DrawEndScreen(won);
            EndDrawing();
            continue;
        }

        // ── PLAYING ───────────────────────────────────────────────────────────
        if (state == Renderer::GameState::PLAYING) {

            // Fresh game setup each time we enter PLAYING
            FileManager fm;
            std::unordered_map<std::string, Item> allItems = fm.loadItems("items.json");
            std::unordered_map<std::string, Room> allRooms = fm.loadRooms("rooms.json", allItems);

            if (allRooms.empty()) {
                std::cout << "ERROR: No rooms loaded.\n";
                break;
            }
            auto startIt = allRooms.find("cell_1");
            if (startIt == allRooms.end()) {
                std::cout << "ERROR: Starting room not found.\n";
                break;
            }

            Player player("Ferengate");
            player.setCurrentRoom(&startIt->second);
            bool running = true;
            std::string userInput;
            std::vector<std::string> displayLines;
            CommandParser parser(&player, running, &fm, &renderer, &allRooms);

            renderer.SetDayProgress(0.0f);

            std::vector<std::string> startingLines = captureOutput([&]() {
                parser.writeMessage(allRooms.at("cell_1").describeSelf());
                });
            startingLines.erase(std::remove_if(startingLines.begin(), startingLines.end(),
                [](const std::string& s) { return s.empty(); }), startingLines.end());
            for (const auto& line : startingLines) {
                if (!line.empty()) displayLines.push_back(line);
            }
            renderer.SnapToBottom();

            InputHandler inputHandler;
            bool wasTyping = false;

            // ── Inner game loop ───────────────────────────────────────────────
            while (!renderer.WindowShouldClose()) {
                renderer.UpdateDayProgress();
                renderer.UpdateScrollInput();
                renderer.UpdateTypingAnimation(GetFrameTime());

                // Always runs — transfers completed animation to permanent history
                bool currentlyTyping = renderer.IsTypingActive();
                if (wasTyping && !currentlyTyping) {
                    const auto& completedLines = renderer.GetLastTypedLines();
                    for (const auto& line : completedLines) {
                        if (!line.empty()) displayLines.push_back(line);
                    }
                    renderer.SnapToBottom();
                }
                wasTyping = currentlyTyping;

                // Check if game ended this frame
                if (parser.gameOver) {
                    // Wait for animation to finish before showing end screen
                    if (!renderer.IsTypingActive()) {
                        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) ||
                            IsKeyPressed(KEY_SPACE)) {
                            state = parser.gameWon
                                ? Renderer::GameState::WIN
                                : Renderer::GameState::DEAD;
                            break;
                        }
                    }
                }
                else if (running) {
                    // Debug keys — remove when done
                    //if (IsKeyPressed(KEY_N)) renderer.SetDayProgress(0.65f);
                    //if (IsKeyPressed(KEY_LEFT_CONTROL)) {
                    //    renderer.SetTimeSpeed(renderer.GetTimeSpeed() > 1.0f ? 1.0f : 60.0f);
                    //}
					// Full Screen toggle
                    if (IsKeyPressed(KEY_F11)) ToggleFullscreen();

                    if (inputHandler.UpdateInput(userInput)) {
                        std::string inputLine = "> " + userInput;
                        displayLines.push_back(inputLine);

                        std::vector<std::string> newOutputLines;

                        if (parser.pendingChoice.active) {
                            int choice = -1;
                            try { choice = std::stoi(userInput) - 1; }
                            catch (...) {}

                            if (choice >= 0 && choice < (int)parser.pendingChoice.choices.size()) {
                                newOutputLines = captureOutput([&]() {
                                    parser.pendingChoice.choices[choice].action();
                                    });
                                parser.pendingChoice.active = false;
                            }
                            else {
                                newOutputLines.push_back("Invalid choice, enter a number.");
                            }
                        }
                        else {
                            newOutputLines = captureOutput([&]() {
                                parser.parse(userInput);
                                });
                        }

                        if (!newOutputLines.empty()) {
                            renderer.StartTypingAnimation(newOutputLines);
                            renderer.SnapToBottom();
                        }

                        userInput.clear();
                    }
                }
                else {
                    // handleQuit was called — go back to menu
                    state = Renderer::GameState::MENU;
                    break;
                }

                // Draw prompt based on state
                const char* prompt = parser.gameOver
                    ? "Press ENTER to continue..."
                    : inputHandler.GetBuffer();

                BeginDrawing();
                renderer.DrawBackground();
                renderer.DrawTextOverlay(displayLines, prompt);
                EndDrawing();
            }
        }
    }

    return 0;
}