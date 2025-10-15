#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

#include "Item.hpp"
#include "Player.hpp"
#include "Room.hpp"

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

	// Pointers for inventory, roomItems
	Player* player;
	Room* room;
	bool& running;

	// const string messages no object
	const std::string MSG_DONT_KNOW_HOW = "I don't know how to do that.";

	// const string messages with object
	const std::string MSG_TAKE = "You now possess a {object}.";
	const std::string MSG_ALREADY_HAVE = "You already have a {object} in your inventory.";
	const std::string MSG_DONT_SEE = "You don't see a {object} here.";
	const std::string MSG_DROP = "You drop the {object}.";
	const std::string MSG_DONT_HAVE = "You don't have a {object}.";

	// booleans
	bool doorLocked = true;
	bool doorOpen = false;

	// Internal helpers
	std::vector<std::string> splitString(std::string& input, char delimiter = ' ');

	bool isValidWord(
		const std::unordered_map<std::string, Item>& inventory, 
		const std::unordered_map<std::string, Item>& roomItems, 
		std::string word
	);

	struct ObjectMatch {
		std::string name;
		int tokenCount;
		int objectIndex;
	};

	ObjectMatch findLongestMatchingObject(int startIndex,
		int tokensLength,
		const std::vector<std::string>& tokens,
		const std::unordered_map<std::string, Item>& inventory,
		const std::unordered_map<std::string, Item>& roomItems,
		int ignoreIndex1 = -1,
		int ignoreIndex2 = -1,
		int ignoreIndex3 = -1);



public:

	// Constructor
	CommandParser(Player* p, Room* r, bool& runningFlag);

	// Parse function (primary function to interpret player input and delegate work to handler functions)
	void parse(
		std::string& input, 
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
	// Drop handler
	void handleDrop(ParsedCommand& cmd);
	// Put handler
	void handlePut(ParsedCommand& cmd);
	// Take handler
	void handleTake(ParsedCommand& cmd);
	// Pick handler
	void handlePick(ParsedCommand& cmd);
	// Look handler
	void handleLook(ParsedCommand& cmd);
	// Examine handler
	void handleExamine(ParsedCommand& cmd);

	// Quit handler
	void handleQuit(ParsedCommand& cmd);

	
	
};