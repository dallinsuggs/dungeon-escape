#include "CommandParser.hpp"
#include "Player.hpp"
#include "Item.hpp"
#include "Room.hpp"

#include <unordered_map>
#include <iostream>

// ITEMS

Item chamberPot("chamber pot", "It is a smelly, stained, ceramic chamber pot with blue and white flower designs on the outside.", true);
Item brick{"brick", "It is a crumbling, dusty old brick.", true };
Item sheet{"sheet", "It is an odorous, soiled bedsheet that looks like it might have been white at some point.", true };
Item door{ "door", "It is a grimy wooden door with horizontal and vertical iron strips reinforcing it.", false };


// ROOM VARIABLES

// CELL

const std::string CELL_NAME = "cell";
const std::string CELL_DESC = "You are in a small, dank dungeon cell with an iron-reinforced wooden door and a simple straw mattress.";
std::unordered_map<std::string, Item> cellItems = {
	{chamberPot.getName(), chamberPot},
	{brick.getName(), brick},
	{sheet.getName(), sheet},
	{door.getName(), door}
};

int main() {
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