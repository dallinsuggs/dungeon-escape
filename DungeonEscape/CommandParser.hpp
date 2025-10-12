#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "Item.hpp"

class CommandParser {
private:
	// struct
	struct ParsedCommand {
		std::string verb;
		std::string preposition;
		std::string object1;
		std::string object2;

		std::unordered_map<std::string, int> indexMap{
		{"verb", -1},
		{"preposition", -1},
		{"object1", -1},
		{"object2", -1}
		};
	};

	std::unordered_map<std::string, void (CommandParser::*)(ParsedCommand&)> verbs;
	std::unordered_set<std::string> prepositions;

	// Pointers for inventory and roomItems
	std::unordered_map<std::string, Item>* inventoryMap;
	std::unordered_map<std::string, Item>* roomItemsMap;

	// const string messages no object
	const std::string MSG_DONT_KNOW_HOW = "I don't know how to do that.";

	// const string messages with object
	const std::string MSG_PICK_UP = "You now possess a {object}.";
	const std::string MSG_ALREADY_HAVE = "You already have a {object} in your inventory.";
	const std::string MSG_DONT_SEE = "I don't see a {object} here.";

	// booleans
	bool doorLocked = true;
	bool doorOpen = false;

	// Internal helpers
	std::vector<std::string> splitString(std::string& input, char delimiter = ' ');

	bool isValidWord(
		const std::vector<std::string>& tokens,
		int index,                                // index of token I want to check
		const std::unordered_map<std::string, Item>& inventory,
		const std::unordered_map<std::string, Item>& roomItems,
		int ignoreIndex1 = -1,                    // default -1 if no index needs to be ignored
		int ignoreIndex2 = -1,
		int ignoreIndex3 = -1
	);

public:

	// Constructor
	CommandParser();

	// Parse function (primary function to interpret player input and delegate work to handler functions)
	void parse(
		std::string& input, 
		std::unordered_map<std::string, Item>& inventory, 
		std::unordered_map<std::string, Item>& roomItems,
		bool& testSuccess
	);

	// Message writer function takes a message template and optionally an object variable
	void writeMessage(const std::string& messageTemplate, const std::string& objectName = "");

	

	// VERB HANDLERS

	// Use handler
	void handleUse(ParsedCommand& cmd);
	// Open handler
	void handleOpen(ParsedCommand& cmd);
	// Inventory handler
	void handleInventory(ParsedCommand& cmd);

	// PHRASAL VERB HANDLERS

	// PickUp handler
	void handlePickUp(std::unordered_map<std::string, Item>& inventory, std::unordered_map<std::string, Item>& roomItems, ParsedCommand& cmd);
	void handlePick(ParsedCommand& cmd);  // wrapper for the handler
};