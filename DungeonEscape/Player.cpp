#include "Player.hpp"
#include <iostream>

Player::Player(const std::string& name, int health)
	: name(name), health(health), currentRoom(nullptr) {}

// Put the player in a specific room (e.g. at the start of the game)
void Player::setCurrentRoom(Room* room) {
	currentRoom = room;
}

// Attempt to move in a direction.
// We ask the current room for the exit and update currentRoom if it exists. 
void Player::move(const std::string& direction) {
	if (currentRoom) {
		Room* next = currentRoom->getExit(direction);
		if (next) {
			currentRoom = next;
			std::cout << "You move " << direction << ".\n";
		} else {
			std::cout << "No exit that way.\n";
		}
	} else {
		std::cout << "You are not in any room yet!\n";
	}
}

// Add an item into the player's inventory
void Player::pickUp(const Item& item) {
	inventory.push_back(item);
}

// Remove an item from the player's inventory by name
void Player::drop(const Item& item) {
	// Find the item in inventory
	auto it = std::find(inventory.begin(), inventory.end(), item);

	if (it != inventory.end()) { 
		// place it in the current room
		if (currentRoom) {
			currentRoom->addItem(*it);
		}

		// Remove from inventory
		std::cout << "You dropped the " << *it << ".\n";
		inventory.erase(it);
		return;
	}
	std::cout << "You don't have a " << *it << ".\n";
}

// Define how a Player prints itself when used with std::cout << player;
std::ostream& operator<<(std::ostream& os, const Player& player) {
	os << "Player: " << player.name << "\n"
		<< "Health: " << player.health << "\n";

	// Show the current room if the player is in one
	if (player.currentRoom) {
		os << "Current location: " << *player.currentRoom << "\n";
	}

	// Show inventory if it is not empty
	if (!player.inventory.empty()) {
		os << "Inventory:\n";
		for (const auto& item : player.inventory) {
			os << " - " << item << "\n"; // uses Item's operator<<
		}
	}

	return os;
}