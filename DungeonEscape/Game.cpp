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

    // Items setup
    std::unordered_map<std::string, Item> allItems = fm.loadItems("items.json");

    // Load rooms
    std::unordered_map<std::string, Room> allRooms = fm.loadRooms("rooms.json", allItems);

    // Initial setup
    Player player("Ferengate");
    bool running = true;
    std::string userInput = "";
    std::vector<std::string> displayLines;
    CommandParser parser(&player, &allRooms.at("cell_1"), running);
    displayLines = captureOutput([&]() { parser.writeMessage(allRooms.at("cell_1").describeSelf()); });

    // Input handler
    InputHandler inputHandler;

    //////////////////////* GAME LOOP HERE */////////////////////
    // Enter game loop
    while (!renderer.WindowShouldClose() && running) {
        // Update day progress (30-min cycle)
        renderer.UpdateDayProgress();



        // TESTING PURPOSES ONLY COMMENT OUT WHEN DONE
        if (IsKeyPressed(KEY_LEFT_CONTROL)) {
            if (renderer.GetTimeSpeed() > 1.0f) { 
                renderer.SetTimeSpeed(1.0f); // Back to normal speed 
            } else {
                renderer.SetTimeSpeed(60.0f); // Speed up time for testing
            }
        }



        // Handle input
        if (inputHandler.UpdateInput(userInput)) {
            // Echo the input as history
			std::string inputEcho = "> " + userInput;
			displayLines.push_back(inputEcho);

            auto newLines = captureOutput([&]() { parser.parse(userInput); });
            for (const auto& line : newLines) {
                if (!line.empty()) displayLines.push_back(line);
            }
            if (displayLines.size() > 20) { // Trim to last 20 lines
                displayLines.erase(displayLines.begin(), displayLines.begin() + (displayLines.size() - 20));
            }
            userInput.clear(); // Reset for next
        }

        // Draw everything
        BeginDrawing();
        renderer.DrawBackground();
        renderer.DrawTextOverlay(displayLines, inputHandler.GetBuffer()); // Pass handler's buffer
        EndDrawing();
    }

    return 0;
}