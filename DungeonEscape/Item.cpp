#include "Item.hpp"
#include <iostream>
#include <string>

// Constructor
Item::Item(const std::string& name, const std::string& description, bool moveable, bool locked)
	: name(name), description(description), moveable(moveable), locked(locked) { }

// Getters
std::string Item::getName() const { return name; }
std::string Item::getDescription() const { return description; }
bool Item::isMoveable() const { return moveable; }
bool Item::isLocked() const { return locked; }

// Setters
void Item::toggleLock() {
	locked = !locked;
}

// << overload print function
std::ostream& operator<<(std::ostream& os, const Item& item) {
	os << "Item: " << item.name << "\nDescription: " << item.description << "\nMoveable: " << item.moveable;
	return os;
}