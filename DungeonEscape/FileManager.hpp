#pragma once
#include <string>
#include <unordered_map>
#include "Item.hpp"
#include "Room.hpp"

class FileManager {
private:
	//Private vars and methods
	std::unordered_map < std::string, std::unique_ptr<Item> > itemInstances;

public:
	// Constructor
	FileManager();
	std::unordered_map<std::string, Item> loadItems(const std::string& filename);
	std::unordered_map<std::string, Room> loadRooms(const std::string& filename, std::unordered_map<std::string, Item>& allItems);
	
	// Helper function to get an Item* by ID from the itemInstances map (returns nullptr if not found)
	Item* getItem(const std::string& id);
};