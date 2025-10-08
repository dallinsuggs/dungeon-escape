#pragma once

#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

class CommandParser {
private:
	std::unordered_map<std::string, void (CommandParser::*)(const std::string&, const std::string&)> verbs;

	// Pointers for inventory and roomItems
	std::unordered_map<std::string, std::string>* inventoryMap;
	std::unordered_map<std::string, std::string>* roomItemsMap;

	// const string messages
	const std::string msgDontKnow = "I don't know how to do that.\n";

	// booleans
	bool doorLocked = true;
	bool doorOpen = false;

	// Internal helpers
	std::vector<std::string> splitString(std::string& input, char delimiter = ' ');

	bool isValidWord(
		const std::vector<std::string>& tokens,
		int index,                                // index of token I want to check
		const std::unordered_map<std::string, std::string>& inventory,
		const std::unordered_map<std::string, std::string>& roomItems,
		int ignoreIndex1 = -1,                    // default -1 if no index needs to be ignored
		int ignoreIndex2 = -1
	);

public:

	// Constructor
	CommandParser();

	// TODO: Replace std::string with Item objects once Item class is implemented
	void parse(
		std::string& input, 
		std::unordered_map<std::string, std::string>& inventory, 
		std::unordered_map<std::string, std::string>& roomItems,
		bool& testSuccess
	);

	

	// VERB HANDLERS

	// Use handler
	void handleUse(const std::string& object1, const std::string& object2 = "");
	// Open handler
	void handleOpen(const std::string& object1, const std::string& object2 = "");
	// Inventory handler
	void handleInventory(const std::string& object1, const std::string& object2 = "");

	// PHRASAL VERB HANDLERS

	// PickUp handler
	void handlePickUp(std::unordered_map<std::string, std::string>& inventory, std::unordered_map<std::string, std::string>& roomItems, const std::string& object1);
	void handlePickUpWrapper(const std::string& object1, const std::string& object2 = "");  // wrapper for the handler
};