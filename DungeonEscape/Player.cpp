#include "Player.hpp"
#include <iostream>

Player::Player(const std::string& name)
	: name(name), currentRoom(nullptr) {}

// Put the player in a specific room (e.g. at the start of the game)
void Player::setCurrentRoom(Room* room) {
	currentRoom = room;
}

// Return the player's inventory
std::unordered_map<std::string, Item*>& Player::getInventory() {
	return inventory;
}

void Player::moveToRoom(const std::string& newRoomId, Room* newRoom)
{
	currentRoomId = newRoomId;
	currentRoom = newRoom;
}


// Define how a Player prints itself when used with std::cout << player;
std::ostream& operator<<(std::ostream& os, const Player& player) {
	os << "Player: " << player.name << "\n";

	// Show the current room if the player is in one
	if (player.currentRoom) {
		os << "Current location: " << *player.currentRoom << "\n";
	}
	return os;
}

// VERB FUNCTIONS

std::string Player::printInventory()
{
	std::string output = "";
	if (this->inventory.empty()) {
		output += "You are carrying nothing.\n";
	}
	else {
		output += "You are carrying the following items: \n";
		for (const auto& pair : this->inventory) {
			output = output + "- " + pair.second->getName() + "\n";
		}
	}
	return output;
}