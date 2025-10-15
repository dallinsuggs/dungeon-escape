#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <ostream>
#include "Item.hpp"

// A Room represents a location in the game world.
// It has a description, may contain items, and has exits leading to other rooms.
class Room {
private:
	std::string name;
	std::string description; // Text describing the room
	std::unordered_map<std::string, Item> roomItems; // Items lying around in the room
	std::map<std::string, Room*> exits; // Maps directions ("north") to connected rooms

public:
	// Constructor
	Room(const std::string& name, const std::string& description, std::unordered_map<std::string, Item> roomItems);

	// getters
	std::unordered_map<std::string, Item>& getRoomItems();

	std::string getDescription() const;
	std::string getName() const;

	// Connect this room to another in a given direction
	void connectRoom(const std::string& direction, Room* otherRoom);

	// Look up the room in a given direction
	Room* getExit(const std::string& direction) const;

	// Overload the << operator so we can print the room with std::cout << room;
	friend std::ostream& operator<<(std::ostream& os, const Room& room);
};