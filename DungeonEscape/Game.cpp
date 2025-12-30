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

// so it doesn't open a console window on Windows
#ifdef _WIN32
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

// Capture cout to lines for Raylib display
std::vector<std::string> captureOutput(std::function<void()> func) {
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf()); // Redirect cout to oss
    func(); // Call the function that produces output
    std::cout.rdbuf(old); // Restore original cout buffer
    std::istringstream iss(oss.str()); // Fixed: 'iss' not 'isspace'
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(iss, line)) { // Now uses 'iss'
        if (!line.empty()) lines.push_back(line);
    }
    return lines;
}

//////////////////////* MAIN */////////////////////
int main() {
    // Set up file manager
    FileManager fm;

    // Renderer setup
    Renderer renderer(1024, 768);

    // Maximum lines to be displayed on screen
    const size_t MAX_LINES = 100;

    // Items setup
    std::unordered_map<std::string, Item> allItems = fm.loadItems("items.json");

    // Load rooms
    std::unordered_map<std::string, Room> allRooms = fm.loadRooms("rooms.json", allItems);

    // Initial setup
    Player player("Ferengate");
    player.setCurrentRoom(&allRooms.at("cell_1"));
    bool running = true;
    std::string userInput = "";
    std::vector<std::string> displayLines;
    CommandParser parser(&player, running);

    // Show the starting room description instantly (no typing animation for the very first message)
    std::vector<std::string> startingLines = captureOutput([&]() {
        parser.writeMessage(allRooms.at("cell_1").describeSelf());
        });

    // Remove empty lines just in case
    startingLines.erase(std::remove_if(startingLines.begin(), startingLines.end(),
        [](const std::string& s) { return s.empty(); }), startingLines.end());

    // Add directly to permanent history
    for (const auto& line : startingLines) {
        if (!line.empty()) {
            displayLines.push_back(line);
        }
    }

    // Optional: make sure player sees it right away
    renderer.SnapToBottom();

    // Input handler
    InputHandler inputHandler;

    //////////////////////* GAME LOOP HERE */////////////////////
    // Enter game loop
    // 
    // 
    while (!renderer.WindowShouldClose() && running) {
        // Update day progress (30-min cycle)
        renderer.UpdateDayProgress();
        renderer.UpdateScrollInput(); // handle scroll input
		renderer.UpdateTypingAnimation(GetFrameTime());

        // Check if typing just finished this frame
        static bool wasTyping = false;
        bool currentlyTyping = !renderer.IsTypingDone();

        if (wasTyping && !currentlyTyping) {
            // Typing just finished → append lines to permanent history
            const auto& completedLines = renderer.GetLastTypedLines();
            for (const auto& line : completedLines) {
                if (!line.empty()) {
                    displayLines.push_back(line);
                }
            }

            // Trim old lines if needed
            if (displayLines.size() > MAX_LINES) {
                displayLines.erase(displayLines.begin(), displayLines.begin() + (displayLines.size() - MAX_LINES));
            }

            renderer.SnapToBottom();  // Auto-scroll
        }
        wasTyping = currentlyTyping;

        

        // TESTING PURPOSES ONLY COMMENT OUT WHEN DONE
        if (IsKeyPressed(KEY_LEFT_CONTROL)) {
            if (renderer.GetTimeSpeed() > 1.0f) {
                renderer.SetTimeSpeed(1.0f); // Back to normal speed 
            }
            else {
                renderer.SetTimeSpeed(60.0f); // Speed up time for testing
            }
        }

        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
        }

        // Handle input
if (inputHandler.UpdateInput(userInput)) {
    // Compose input line
    std::string inputLine = "> " + userInput;

    // Echo input immediately
    displayLines.push_back(inputLine);

    // Compute wrapped height for proper spacing (matches DrawTextOverlay)
    int fontSize = 32; // same as in DrawTextOverlay
    int textAreaWidth = GetScreenWidth() - 80; // same as in DrawTextOverlay
    int inputHeight = renderer.GetWrappedHeight(inputLine.c_str(), textAreaWidth, fontSize);

    // Optionally auto-scroll to bottom after input
    renderer.SnapToBottom(0); // 0 = no extra padding; adjust if you want

    std::vector<std::string> newOutputLines;


    // Check if choice prompt bool is active
    if (parser.pendingChoice.active) {
        int choice = -1;
        try { choice = std::stoi(userInput) - 1; }
        catch (...) {}

        if (choice >= 0 && choice < parser.pendingChoice.choices.size()) {
            newOutputLines = captureOutput([&]() {
                parser.pendingChoice.choices[choice].action();
            });
            parser.pendingChoice.active = false;
        } else {
            newOutputLines.push_back("Invalid choice, enter a number corresponding to your exit.");
        }
    } else {                                                                                           // else, regular input parsing
        newOutputLines = captureOutput([&]() { parser.parse(userInput); });
    }

    // Only start animation if there are new lines
    if (!newOutputLines.empty()) {
        renderer.StartTypingAnimation(newOutputLines);
        // Do NOT append to displayLines here – it happens after animation completes
    }

    userInput.clear();
}



        // Draw everything
        BeginDrawing();
        renderer.DrawBackground();
        renderer.DrawTextOverlay(displayLines, inputHandler.GetBuffer()); // Pass handler's buffer
        EndDrawing();
    }

    return 0;
}