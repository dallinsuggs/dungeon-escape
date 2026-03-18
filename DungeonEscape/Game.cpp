#include "CommandParser.hpp"
#include "Player.hpp"
#include "Item.hpp"
#include "Room.hpp"
#include "FileManager.hpp"
#include <unordered_map>
#include <iostream>
// For the window
#include "Renderer.h"
#include <chrono> // for time tracking
#include <sstream> // for capture
#include <vector> // for lines
#include <functional> // for lambda
#include <cmath> // for sinf
// For json parsing
#include "rapidjson/document.h" // Core DOM parser
#include "rapidjson/filereadstream.h" // For reading JSON from a FILE*
#include "rapidjson/error/en.h" // Optional: human-readable parse errors
#include <fstream> // for file reading
// For Input Handling
#include "InputHandler.h"
#include "raylib.h"
#include "FileManager.hpp"

// so it doesn't open a console window on Windows
#ifdef _WIN32
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

// Capture cout to lines for Raylib display
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
    // For logging purposes
    SetTraceLogLevel(LOG_ALL);
    SetTraceLogCallback([](int logType, const char* text, va_list args) {
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), text, args);
        printf("%s\n", buffer);
        });

    // Renderer setup — created ONCE outside the play loop so the window persists
    Renderer renderer(1024, 768);

    bool playAgain = true;

    while (playAgain && !renderer.WindowShouldClose()) {

        // ── GAME SETUP (fresh each run) ──────────────────────────────────────

        FileManager fm;

        std::unordered_map<std::string, Item> allItems = fm.loadItems("items.json");
        std::unordered_map<std::string, Room> allRooms = fm.loadRooms("rooms.json", allItems);

        if (allRooms.empty()) {
            std::cout << "ERROR: No rooms loaded from rooms.json – check file and JSON validity.\n";
            return 1;
        }

        auto startIt = allRooms.find("cell_1");
        if (startIt == allRooms.end()) {
            std::cout << "ERROR: Starting room 'cell_1' not found.\n";
            return 1;
        }

        Player player("Ferengate");
        player.setCurrentRoom(&startIt->second);
        bool running = true;
        std::string userInput = "";
        std::vector<std::string> displayLines;
        CommandParser parser(&player, running, &fm, &renderer, &allRooms);

        // Reset renderer state for a fresh run
        renderer.SetDayProgress(0.0f);
        renderer.SnapToBottom();

        // Show starting room description immediately (no typing animation)
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

        //////////////////////* GAME LOOP */////////////////////
        while (!renderer.WindowShouldClose()) {
            renderer.UpdateDayProgress();
            renderer.UpdateScrollInput();
            renderer.UpdateTypingAnimation(GetFrameTime());

            // ── Always runs regardless of game state ──────────────────────────
            bool currentlyTyping = renderer.IsTypingActive();
            if (wasTyping && !currentlyTyping) {
                const auto& completedLines = renderer.GetLastTypedLines();
                for (const auto& line : completedLines) {
                    if (!line.empty()) displayLines.push_back(line);
                }
                renderer.SnapToBottom();
            }
            wasTyping = currentlyTyping;

            // ── Game state branches ───────────────────────────────────────────
            if (parser.gameOver) {
                // Wait for player to acknowledge, then break to play-again screen
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_SPACE)) {
                    break;
                }
            }
            else if (running) {
                // Debug keys — remove when done
                if (IsKeyPressed(KEY_N)) {
                    renderer.SetDayProgress(0.65f);
                }
                if (IsKeyPressed(KEY_LEFT_CONTROL)) {
                    renderer.SetTimeSpeed(renderer.GetTimeSpeed() > 1.0f ? 1.0f : 60.0f);
                }
                if (IsKeyPressed(KEY_F11)) {
                    ToggleFullscreen();
                }

                // Handle player input
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
                            newOutputLines.push_back("Invalid choice, enter a number corresponding to your exit.");
                        }
                    }
                    else {
                        newOutputLines = captureOutput([&]() { parser.parse(userInput); });
                    }

                    if (!newOutputLines.empty()) {
                        renderer.StartTypingAnimation(newOutputLines);
                        renderer.SnapToBottom();
                    }

                    userInput.clear();
                }
            }
            else {
                // running == false means handleQuit was called — exit immediately
                break;
            }

            // ── Draw ─────────────────────────────────────────────────────────
            BeginDrawing();
            renderer.DrawBackground();
            renderer.DrawTextOverlay(
                displayLines,
                parser.gameOver ? "Press ENTER to play again, ESC to quit" : inputHandler.GetBuffer()
            );
            EndDrawing();
        }

        // ── If quit was typed (not game over), don't show play again ─────────
        if (!running && !parser.gameOver) {
            break;
        }

        // ── Play again screen ─────────────────────────────────────────────────
        while (!renderer.WindowShouldClose()) {
            if (IsKeyPressed(KEY_ENTER)) { playAgain = true;  break; }
            if (IsKeyPressed(KEY_ESCAPE)) { playAgain = false; break; }

            BeginDrawing();
            renderer.DrawBackground();
            renderer.DrawTextOverlay(
                displayLines,
                parser.gameWon ? "You escaped! ENTER to play again, ESC to quit"
                : "Game Over. ENTER to play again, ESC to quit"
            );
            EndDrawing();
        }
    }

    return 0;
}