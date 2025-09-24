#include "Item.hpp"
#include <iostream>
#include <string>

// Constructor
Item::Item(const std::string& name, const std::string& description, const std::string& effect)
	: name(name), description(description), effect(effect) {}

std::ostream& operator<<(std::ostream& os, const Item& item) {
	os << "Item: " << item.name << "\nDescription: " << item.description << "\nEffect: " << item.effect;
	return os;
}

void Item::use() const {
	std::cout << "You use the " << name << "." << std::endl;
}