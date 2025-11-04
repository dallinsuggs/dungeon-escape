#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

#include "FileManager.hpp"


#include <string>
#include <cctype> // for std::isspace
#include <algorithm> // for std::find_if

// Trim from start (left)
static inline void ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(),
		[](unsigned char ch) { return !std::isspace(ch); }));
}

// Trim from end (right)
static inline void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(),
		[](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
}

// Trim both ends
static inline void trim(std::string& s) {
	ltrim(s);
	rtrim(s);
}


FileManager::FileManager()
{
}

// Function that reads data from item file and returns unordered map with all the items
std::unordered_map<std::string, Item> FileManager::loadItems(const std::string& filename)
{
	std::unordered_map<std::string, Item> items;
	std::ifstream inFile(filename);

	if (!inFile) {
		std::cerr << "Could not open file for reading: " << filename << "\n";
		return {};
	}

	std::string line;
	while (std::getline(inFile, line)) {
		std::stringstream ss(line);
		std::string name, description, moveable;

		if (std::getline(ss, name, '|') && std::getline(ss, description, '|') && std::getline(ss, moveable)) {
			bool isMoveable = (moveable == "1");
			items.emplace(name, Item(name, description, isMoveable));
		}

	}
	return items;
}

std::unordered_map<std::string, Room> FileManager::loadRooms(const std::string& filename, std::unordered_map<std::string, Item>& allItems)
{
	std::unordered_map<std::string, Room> rooms;
	std::ifstream inFile(filename);

	if (!inFile) {
		std::cerr << "Could not open file for reading: " << filename << "\n";
		return {};
	}

	std::string line;
	while (std::getline(inFile, line)) {
		std::stringstream ss(line);
		std::string id, name, description, itemsString, directions;
		std::unordered_map<std::string, Item*> items;

		if (std::getline(ss, id, '|') && std::getline(ss, name, '|') && std::getline(ss, description, '|') && std::getline(ss, itemsString, '|') && std::getline(ss, directions)) {

			// string stream for items
			std::stringstream itemsStream(itemsString);
			std::string itemName;

			while (std::getline(itemsStream, itemName, ',')) {
				trim(itemName);
				if (itemName.empty()) continue;
				if (allItems.find(itemName) != allItems.end()) {
					items[itemName] = &allItems[itemName];
				} else {
					std::cerr << "Warning: Item not found in allItems: " << itemName << "\n";
				}
			}


			rooms.emplace(id, Room(id, name, description, items));
		}

	}
	return rooms;
}

