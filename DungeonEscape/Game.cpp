#include "CommandParser.hpp"
#include "Player.hpp"
#include "Item.hpp"
#include "Room.hpp"

#include <unordered_map>
#include <iostream>

// For the window
#include "raylib.h"
#include <chrono> // for time tracking
#include <sstream> // for capture
#include <vector> // for lines
#include <functional> // for lambda
#include <cmath> // for sinf


// Function that reads data from item file and returns unordered map with all the items
std::unordered_map<std::string, Item> loadItems(const std::string& filename)
{
	std::unordered_map<std::string, Item> items;
	std::ifstream inFile(filename);

	if (!inFile) {
		std::cerr << "Could not open file for writing\n";
	}

	std::string line;
	while (std::getline(inFile, line)) {
		std::stringstream ss(line);
		std::string name, description, moveable;

		if (std::getline(ss, name, '|') && std::getline(ss, description, '|') && std::getline(ss, moveable)) {
			bool isMoveable = (moveable == "1");
			items.emplace(name, Item(name, description, isMoveable));
		}

	}
	return items;
}

// Sets up cell's item list
std::unordered_map<std::string, Item*> createCellItems(std::unordered_map<std::string, Item>& allItems) {
	return {
		{allItems["chamber pot"].getName(), &allItems["chamber pot"]},
		{allItems["brick"].getName(), &allItems["brick"]},
		{allItems["sheet"].getName(), &allItems["sheet"]},
		{allItems["door"].getName(), &allItems["door"]}
	};
}

// Capture cout to lines for Raylib display
std::vector<std::string> captureOutput(std::function<void()> func) {
	std::ostringstream oss;
	std::streambuf* old = std::cout.rdbuf(oss.rdbuf()); // Redirect cout to oss
	func(); // Call the function that produces output
	std::cout.rdbuf(old); // Restore original cout buffer
	std::istringstream iss(oss.str());  // Fixed: 'iss' not 'isspace'
	std::vector<std::string> lines;
	std::string line;
	while (std::getline(iss, line)) {  // Now uses 'iss'
		if (!line.empty()) lines.push_back(line);
	}
	return lines;
}

// Wrap text in window width, draw multiple lines, update currentY position
void DrawWrappedText(const char* text, int x, int y, int maxWidth, int fontSize, Color color, int& currentY) {
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
				lineY += fontSize + 2;  // Advance to next wrapped line
			}
			currentLine = word;
		}
	}
	if (!currentLine.empty()) {
		DrawText(currentLine.c_str(), x, lineY, fontSize, color);
		lineY += fontSize + 2;  // Advance after last line
	}
	currentY = lineY + 6;  // Gap to next entry (tweak if needed)
}

int main() {
	// Window setup
	const int screenWidth = 800;
	const int screenHeight = 600;
	InitWindow(screenWidth, screenHeight, "Dungeon Escape");
	SetTargetFPS(60);

	// Time tracking for day cycle
	auto startTime = std::chrono::steady_clock::now();
	float dayProgress = 0.0f;

	// Items setup
	std::unordered_map<std::string, Item> allItems = loadItems("items.txt");
	
	// Cell room setup
	const std::string CELL_ID = "cell_1";
	const std::string CELL_NAME = "cell";
	const std::string CELL_DESC = "You are in a small, dank dungeon cell with an iron-reinforced wooden door and a simple straw mattress.";
	std::unordered_map<std::string, Item*> cellItems = createCellItems(allItems);
	


	// Initial setup
	Player player("Ferengate");
	Room roomCell(CELL_ID, CELL_NAME, CELL_DESC, cellItems);
	bool running = true;
	std::string userInput = "";
	std::vector<std::string> displayLines;

	// Input buffer for Raylib (replaces console getline)
	char inputBuffer[256] = "\0";
	int letterCount = 0;

	CommandParser parser(&player, &roomCell, running);

	// parser.writeMessage(roomCell.describeSelf());
	displayLines = captureOutput([&]() { parser.writeMessage(roomCell.describeSelf()); });

	

	//////////////////////* GAME LOOP HERE */////////////////////
	// Enter game loop
	while (!WindowShouldClose() && running) {
		/* THIS IS THE BACKGROUND DESIGN */
		// Update day progress (30-min cycle)
		auto currentTime = std::chrono::steady_clock::now();
		float elapsedSeconds = std::chrono::duration<float>(currentTime - startTime).count();
		dayProgress = fmod(elapsedSeconds / 1800.0f, 1.0f); // 1800 seconds = 30 minutes

		// Raylib input: Builds string non-blockingly
		int key = GetCharPressed();
		while (key > 0) {
			if (key >= 32 && key <= 125 && letterCount < 255) {  // Printable keys only (includes space)
				inputBuffer[letterCount++] = (char)key;
				inputBuffer[letterCount] = '\0';  // Null-terminate
			}
			key = GetCharPressed();  // Next char in queue
		}

		// Handle special keys (Enter, Backspace)
		if (IsKeyPressed(KEY_ENTER) && letterCount > 0) {
			userInput = std::string(inputBuffer);
			auto newLines = captureOutput([&]() { parser.parse(userInput); });
			for (const auto& line : newLines) {
				if (!line.empty()) displayLines.push_back(line);
			}
			if (displayLines.size() > 20) {  // Trim to last 20 lines
				displayLines.erase(displayLines.begin(), displayLines.begin() + (displayLines.size() - 20));
			}
			letterCount = 0;
			inputBuffer[0] = '\0';
			userInput.clear();
		}
		else if (IsKeyPressed(KEY_BACKSPACE) && letterCount > 0) {
			letterCount--;
			inputBuffer[letterCount] = '\0';
		}



		//////////////////////* BEGIN DRAWING HERE */////////////////////
		BeginDrawing();
		ClearBackground(SKYBLUE);

	
		// Sky fade from dawn to dusk
		if (dayProgress > 0.5f) {
			DrawRectangle(0, 0, screenWidth, screenHeight / 2,
				Color{ 255, 165, 0,(unsigned char)(255 * (dayProgress - 0.5f) * 2) });
		}
		// Castle silhouette (bottom)
		DrawRectangle(screenWidth / 2 - 100, screenHeight - 150, 200, 150, GRAY);
		DrawRectangle(screenWidth / 2 - 50, screenHeight - 250, 100, 100, DARKGRAY);
		
		// Sun arc and color change
		float sunX = screenWidth * (dayProgress * 2.0f);
		if (sunX > screenWidth) sunX = 2 * screenWidth - sunX; // Reflect for setting sun
		float sunY = screenHeight * (0.5f - 0.3f * sinf(dayProgress * PI * 2.0f)); // Arc path
		Color sunColor = (dayProgress < 0.3f) ? YELLOW : ((dayProgress > 0.7f) ? ORANGE : GOLD); // Change color at dawn/dusk
		DrawCircle(sunX, sunY, 30, sunColor);

		// Set semi-transparent overlay for game text (readable against background)
		DrawRectangle(20, 20, screenWidth - 40, screenHeight - 100, Fade(BLACK, 0.2f));
		// Draw wrapped output lines
		int yPos = 50;
		const int textAreaWidth = screenWidth - 80;
		for (const auto& line : displayLines) {
			if (yPos < screenHeight - 100) {
				DrawWrappedText(line.c_str(), 40, yPos, textAreaWidth, 16, WHITE, yPos);
			}
		}
		// Input prompt
		DrawText("> ", 40, screenHeight - 60, 20, WHITE);
		DrawText(inputBuffer, 80, screenHeight - 60, 20, WHITE);


		EndDrawing();



		// Prompt player input
		// std::cout << ">";
		// std::getline(std::cin, userInput);

		// Send input to parser
		// parser.parse(userInput);

		// Display output

	}
	CloseWindow();
	return 0;
}