#include "Room.hpp"

Room::Room(const std::string& description) : description(description) {}

// Add an item to this room's vector of items
void Room::addItem(const Item& item) {
	items.push_back(item);
}

// Store a pointer to another room in the exits map
void Room::connectRoom(const std::string& direction, Room* otherRoom) {
	exits[direction] = otherRoom;
}

// Try to find an exit in a direction (returns null pointer if none exists)
Room* Room::getExit(const std::string& direction) const {
	// https:// stackoverflow.com/questions/15451287/what-does-iterator-second-mean
	auto it = exits.find(direction);
	if (it != exits.end()) {
		return it->second; // found a connected room, second is the value of the map
	} 
	return nullptr; // no room in that direction
}

// Define how a Room prints itself when used with std::cout << room;
std::ostream& operator<<(std::ostream& os, const Room& room) {
	os << room.description << "\n";

	// Print items if the room has any
	if (!room.items.empty()) {
		os << "Items here:\n";
		for (const auto& item : room.items) {
			os << " - " << item << "\n"; // calls Item's operator<<
		}
	}

	// Print available exits
	if (!room.exits.empty()) {
		os << "Exits:\n";
		for (const auto& exit : room.exits) {
			os << " - " << exit.first << "\n";
		}
	}

	return os;
}
