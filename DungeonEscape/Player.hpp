#pragma once
#include <string>
#include <vector>
#include <ostream>
#include "Item.hpp"
#include "Room.hpp"

// A Player represents the person playing the game.
// They have a name, health, an inventory of items, and a current location.
class Player {
private:
	std::string name;
	int health;
	Room* currentRoom;
	std::vector<Item> inventory;

public:
	// Constructor
	Player(const std::string& name, int health);

	// Place the player into a starting room
	void setCurrentRoom(Room* room);

	// Move to another room if there is an exit in that direction
	void move(const std::string& direction);

	// Pick up an item (adds to inventory)
	void pickUp(const Item& item);

	// Drop an item by name (removes from inventory)
	void drop(const Item& item);

	// Overload << operator so we can print a Player with std::cout << player;
	friend std::ostream& operator<<(std::ostream& os, const Player& player);
};