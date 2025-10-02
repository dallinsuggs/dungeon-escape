#pragma once // ensures the header is included only once
#include <string>
#include <iostream>

class Item { 
private:
	std::string name;
	std::string description;
	std::string effect; // not sure about this one, int? vector?

public:
	// constructor to initialize immutable members
	Item(const std::string& name, const std::string& description, const std::string& effect);

	// Overload equality operator for comparisons
	bool operator==(const Item& other) const {
		return name == other.name; // compare by name (or both name + description)
	}

	// Overload the output stream operator for easy printing, friend function
	friend std::ostream& operator<<(std::ostream& os, const Item& item);

	// possible behavior for Item class
	void use() const;
};
