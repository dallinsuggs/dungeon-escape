#include "CommandParser.hpp"
#include "Player.hpp"
#include "Item.hpp"
#include "Room.hpp"

#include <unordered_map>
#include <iostream>

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



int main() {

	// Items setup
	std::unordered_map<std::string, Item> allItems = loadItems("game_items.txt");
	
	// Cell room setup
	const std::string CELL_NAME = "cell";
	const std::string CELL_DESC = "You are in a small, dank dungeon cell with an iron-reinforced wooden door and a simple straw mattress.";
	std::unordered_map<std::string, Item*> cellItems = createCellItems(allItems);

	// Initial setup
	Player player("Ferengate");
	Room roomCell(CELL_NAME, CELL_DESC, cellItems);
	bool running = true;
	std::string userInput = "";

	CommandParser parser(&player, &roomCell, running);

	parser.writeMessage(roomCell.describeSelf());

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