#include "Item.hpp"
#include <iostream>
#include <string>

// Constructor
Item::Item(const std::string& name, const std::string& description, bool moveable)
	: name(name), description(description), moveable(moveable) { }

std::ostream& operator<<(std::ostream& os, const Item& item) {
	os << "Item: " << item.name << "\nDescription: " << item.description << "\nMoveable: " << item.moveable;
	return os;
}