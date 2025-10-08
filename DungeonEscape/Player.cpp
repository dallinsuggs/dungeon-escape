#include "Player.hpp"
#include <iostream>

Player::Player(const std::string& name, int health, std::unordered_map<std::string, Item> inventory)
	: name(name), health(health), currentRoom(nullptr), inventory(inventory) {}

// Put the player in a specific room (e.g. at the start of the game)
void Player::setCurrentRoom(Room* room) {
	currentRoom = room;
}

// Return the player's inventory
std::unordered_map<std::string, Item>& Player::getInventory() {
	return inventory;
}

// Attempt to move in a direction.
// We ask the current room for the exit and update currentRoom if it exists. 
//void Player::move(const std::string& direction) {
//	if (currentRoom) {
//		Room* next = currentRoom->getExit(direction);
//		if (next) {
//			currentRoom = next;
//			std::cout << "You move " << direction << ".\n";
//		} else {
//			std::cout << "No exit that way.\n";
//		}
//	} else {
//		std::cout << "You are not in any room yet!\n";
//	}
//}

// Define how a Player prints itself when used with std::cout << player;
std::ostream& operator<<(std::ostream& os, const Player& player) {
	os << "Player: " << player.name << "\n"
		<< "Health: " << player.health << "\n";

	// Show the current room if the player is in one
	if (player.currentRoom) {
		os << "Current location: " << *player.currentRoom << "\n";
	}
	return os;
}