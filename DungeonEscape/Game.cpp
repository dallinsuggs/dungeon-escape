#include "CommandParser.hpp"
#include "Player.hpp"
#include "Item.hpp"
#include "Room.hpp"
#include "FileManager.hpp"

#include <unordered_map>
#include <iostream>



int main() {

	// FileManager setup
	FileManager myFileManager;

	// Items setup
	std::unordered_map<std::string, Item> allItems = myFileManager.loadItems("items.txt");

	// Rooms setup
	std::unordered_map<std::string, Room> allRooms = myFileManager.loadRooms("rooms.txt", allItems);

	// Initial setup
	Player player("Ferengate");
	//Room roomCell(CELL_ID, CELL_NAME, CELL_DESC, cellItems);
	bool running = true;
	std::string userInput = "";

	CommandParser parser(&player, &allRooms["cell_1"], running);

	parser.writeMessage(allRooms["cell_1"].describeSelf());

	// Enter game loop
	while (running) {

		// Prompt player input
		std::cout << ">";
		std::getline(std::cin, userInput);

		// Send input to parser
		parser.parse(userInput);

		// Display output

	}
}