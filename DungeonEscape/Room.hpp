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
public:
	struct ExitOption { Room* room; std::string label; };
private:
	std::string name;
	std::string description; // Text describing the room
	std::unordered_map<std::string, Item*> roomItems; // Items lying around in the room
	
	std::map<std::string, std::vector<ExitOption>> exits; // Maps directions ("north") to connected rooms

public:
	// Constructor
	Room(const std::string& name, const std::string& description, const std::unordered_map<std::string, Item*>& roomItems, const std::map<std::string, std::vector<ExitOption>>& exits = {});


	// getters
	std::unordered_map<std::string, Item*>& getRoomItems();

	std::string getId() const;
	std::string getDescription() const;
	std::string getName() const;

	// describeSelf dynamically describes the actual state of the room given a room description and a list of items present
	std::string describeSelf() const;

	// Connect this room to another in a given direction
	void connectRoom(const std::string& direction, Room* otherRoom, const std::string& label);

	// Look up the room in a given direction
	const std::vector<ExitOption>* getExits(const std::string& direction) const;

	// Overload the << operator so we can print the room with std::cout << room;
	friend std::ostream& operator<<(std::ostream& os, const Room& room);
};