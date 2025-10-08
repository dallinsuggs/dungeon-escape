#pragma once // ensures the header is included only once
#include <string>
#include <iostream>

class Item { 
private:
	std::string name;
	std::string description;
	bool moveable; // whether the item can be picked up or not
	//std::string effect; // not sure about this one, int? vector?

public:
	// constructor 
	Item(const std::string& name, const std::string& description, bool moveable /*const std::string& effect*/);

	// Overload equality operator for comparisons
	bool operator==(const Item& other) const {
		return name == other.name; // compare by name (or both name + description)
	}

	// Overload the output stream operator for easy printing, friend function
	friend std::ostream& operator<<(std::ostream& os, const Item& item);
};
