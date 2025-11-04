#pragma once
#include "Item.hpp"
#include "Room.hpp"

class FileManager {

private:


public:

	// Constructor declaration
	FileManager();

	// File Manager methods
	std::unordered_map<std::string, Item> loadItems(const std::string& filename);
	std::unordered_map<std::string, Room> loadRooms(const std::string& filename, std::unordered_map<std::string, Item>& allItems);


};