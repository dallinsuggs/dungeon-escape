#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <functional>

#include "Item.hpp"
#include "Player.hpp"
#include "Room.hpp"
#include "FileManager.hpp"
#include "Renderer.h"

class CommandParser {
public:
	// msgs struct
	struct MultiMsg {
		std::string msg1;
		std::string msg1Param1 = "";
		std::string msg1Param2 = "";
		std::string msg2;
		std::string msg2Param1 = "";
		std::string msg2Param2 = "";
	};
	// Choice struct
	struct Choice {
		std::string label;
		std::function<void()> action;
	};
	// Pending choice state
	struct PendingChoice {
		std::vector<Choice> choices;
		bool active = false;
	};
	PendingChoice pendingChoice;
	bool gameOver = false;
	bool gameWon = false;
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

	enum class ItemScope {
		RoomOnly,
		InventoryOnly,
		RoomAndInventory
	};

	

	std::unordered_map<std::string, void (CommandParser::*)(ParsedCommand&)> verbs;
	std::unordered_set<std::string> prepositions;
	std::unordered_map<std::string, int> itemUseCount; // For items that are consumable, i.e. animal bones

	// Pointers for inventory, roomItems, and more
	Player* player;
	bool& running;
	FileManager* fileManager = nullptr;
	Renderer* renderer = nullptr;
	std::unordered_map<std::string, Room>* allRooms = nullptr;

	// const string messages no object
	const std::string MSG_DONT_KNOW_HOW = "I don't know how to do that.";

	// const string messages with object
	const std::string MSG = "You don't know how to do that.";
	const std::string MSG_TAKE = "You now possess a {object1}.";
	const std::string MSG_ALREADY_HAVE = "You already have a {object1} in your inventory.";
	const std::string MSG_DONT_SEE = "You don't see a {object1} here.";
	const std::string MSG_DROP = "You drop the {object1}.";
	const std::string MSG_DONT_HAVE = "You don't have a {object1}.";
	const std::string MSG_CANT_TAKE = "You can't pick up the {object1}, it's either too heavy or securely attached.";
	const std::string MSG_NO_EXIT = "You don't see any path or exit to the {object1}.";
	const std::string MSG_MULTI_EXITS = "There are multiple exits to the {object1}: \n{object2}";
	const std::string MSG_SELECT_CHOICE = "Type the number of your choice and press enter.";
	const std::string MSG_MULTI_ITEMS = "There are multiple {object1} items: \n{object2}";
	const std::string MSG_VERB_WHAT = "{object1} what?";
	const std::string MSG_LOCKED = "It seems the door to that is locked.";
	const std::string MSG_TOILET_DEATH = "You approach the toilet and peer down a dark hole. After a moment's hesitation you leap inside, falling for several seconds before a voilent impact. You feel your legs shatter beneath you and everything cuts to black. Game Over.";
	const std::string MSG_NO_MATCH = "I don't see a {object1} here.";
	const std::string MSG_VERB_WHAT_ON_WHAT = "{object1} what on {object2}?";
	const std::string MSG_PICK_LOCK = "Using the {object1} you are able to pick the lock, but it feels like it may break soon. The {object2} is now unlocked.";
	const std::string MSG_PICK_LOCK_BREAK = "Using the {object1} you are able to pick the lock, but it breaks. The {object2} is now unlocked.";

	// booleans
	bool doorLocked = true;
	bool doorOpen = false;

	// Internal helpers
	void promptChoice(const std::vector<Choice>& choices, MultiMsg msgs);
	std::vector<std::string> getItemIdsByName(const std::unordered_map<std::string, Item*>& itemList, const std::string& objectName);
	std::vector<std::string> getAllItemIdsByName(const std::unordered_map<std::string, Item*>& inventory, const std::unordered_map<std::string, Item*>& roomItems, const std::string& objectName);
	std::string resolveSingleItemId(const std::unordered_map<std::string, Item*>& itemList, const std::string& objectName);
	std::string resolveAllSingleItemId(const std::unordered_map<std::string, Item*>& inventory, const std::unordered_map<std::string, Item*>& roomItems, const std::string& objectName);
	std::vector<std::string> splitString(std::string& input, char delimiter = ' ');
	bool resolveOrPromptItem(const std::string& objectName, ItemScope scope, const std::string& MSG_NO_MATCH, const std::function<void(const std::string& id, Item* item)>& onChosen);

	bool isValidWord(
		const std::unordered_map<std::string, Item*>& inventory, 
		const std::unordered_map<std::string, Item*>& roomItems, 
		const std::string& word
	);

	struct ObjectMatch {
		std::string name;
		int tokenCount;
		bool valid = false;
	};

	ObjectMatch findLongestMatchingObject(int startIndex,
		int tokensLength,
		const std::vector<std::string>& tokens,
		const std::unordered_map<std::string, Item*>& inventory,
		const std::unordered_map<std::string, Item*>& roomItems,
		int ignoreIndex1 = -1,
		int ignoreIndex2 = -1,
		int ignoreIndex3 = -1);



public:

	// Constructor
	CommandParser(Player* p, bool& runningFlag, FileManager* fm, Renderer* renderer, std::unordered_map<std::string, Room>* rooms);

	// Parse function (primary function to interpret player input and delegate work to handler functions)
	void parse(std::string& input);

	// Message writer function takes a message template and optionally an object variable
	void writeMessage(const std::string& messageTemplate, const std::string& object1Name = "", const std::string& object2Name = "");

	

	// VERB HANDLERS

	// Help handler
	void handleHelp(ParsedCommand& cmd);
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
	// Go handler
	void handleGo(ParsedCommand& cmd);
	// Unlock handler
	void handleUnlock(ParsedCommand& cmd);

	// Quit handler
	void handleQuit(ParsedCommand& cmd);

	
	
};