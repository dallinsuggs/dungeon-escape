#pragma once
#include <string>
#include <unordered_map>
#include <ostream>
#include "Item.hpp"
#include "Room.hpp"

// A Player represents the person playing the game.
// They have a name, health, an inventory of items, and a current location.
class Player {
private:
	std::string name;
	Room* currentRoom;
	std::unordered_map<std::string, Item*> inventory; //map item name to Item object for easy lookup

public:
	// Constructor
	Player(const std::string& name);

	// Place the player into a starting room
	void setCurrentRoom(Room* room);

	// get the player's inventory
	std::unordered_map<std::string, Item*>& getInventory();

	// Move to another room if there is an exit in that direction
	//void move(const std::string& direction);

	// Overload << operator so we can print a Player with std::cout << player;
	friend std::ostream& operator<<(std::ostream& os, const Player& player);


	// VERB FUNCTIONS

	// Return inventory output as string
	std::string printInventory();
};