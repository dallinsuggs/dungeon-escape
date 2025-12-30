#pragma once // ensures the header is included only once
#include <string>
#include <iostream>

class Item { 
private:
	std::string name;
	std::string description;
	bool moveable; // whether the item can be picked up or not
	bool locked = false;
	//std::string effect; // not sure about this one, int? vector?

public:
	// default constructor
	Item() : name(""), description(""), moveable(false), locked(false) {}
	// constructor 
	Item(const std::string& name, const std::string& description, bool moveable, bool locked /*const std::string& effect*/);

	// Getters
	std::string getName() const;
	std::string getDescription() const;
	bool isMoveable() const;
	bool isLocked() const;

	// Setters
	void toggleLock();

	// Overload equality operator for comparisons
	bool operator==(const Item& other) const {
		return name == other.name; // compare by name (or both name + description)
	}

	// Overload the output stream operator for easy printing, friend function
	friend std::ostream& operator<<(std::ostream& os, const Item& item);
};
