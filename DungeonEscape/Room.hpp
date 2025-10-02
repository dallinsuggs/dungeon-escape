#pragma once
#include <string>
#include <vector>
#include <map>
#include <ostream>
#include "Item.hpp"

// A Room represents a location in the game world.
// It has a description, may contain items, and has exits leading to other rooms.
class Room {
private:
	std::string description; // Text describing the room
	std::vector<Item> items; // Items lying around in the room
	std::map<std::string, Room*> exits; // Maps directions ("north") to connected rooms

public:
	// Constructor
	Room(const std::string& description);

	// Add an item into the room (e.g. "put sword in kitchen")
	void addItem(const Item& item);

	// Connect this room to another in a given direction
	void connectRoom(const std::string& direction, Room* otherRoom);

	// Look up the room in a given direction
	Room* getExit(const std::string& direction) const;

	// Overload the << operator so we can print the room with std::cout << room;
	friend std::ostream& operator<<(std::ostream& os, const Room& room);
};