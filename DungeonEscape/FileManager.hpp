#pragma once
#include <string>
#include <unordered_map>
#include "Item.hpp"
#include "Room.hpp"

class FileManager {
private:
	//Private vars and methods

public:
	// Constructor
	FileManager();
	std::unordered_map<std::string, Item> loadItems(const std::string& filename);
	std::unordered_map<std::string, Room> loadRooms(const std::string& filename, std::unordered_map<std::string, Item>& allItems);
};