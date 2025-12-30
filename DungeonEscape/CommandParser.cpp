#include "CommandParser.hpp"
#include <algorithm>
#include <cctype>
#include <string>

std::string normalize(const std::string& s) {
	std::string out = s;
	out.erase(out.begin(), std::find_if(out.begin(), out.end(), [](unsigned char c) { return !std::isspace(c); }));
	out.erase(std::find_if(out.rbegin(), out.rend(), [](unsigned char c) { return !std::isspace(c); }).base(), out.end());
	std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return std::tolower(c); });
	return out;
}

// INTERNAL HELPERS


void CommandParser::promptChoice(const std::vector<Choice>& choices, MultiMsg msgs)
{
	pendingChoice.choices = choices;
	pendingChoice.active = true;
	
	writeMessage(msgs.msg1, msgs.msg1Param1, msgs.msg1Param2);
	if (!msgs.msg2.empty()) {
		writeMessage(msgs.msg2, msgs.msg2Param1, msgs.msg2Param2);
	}
}

// This function checks a single unordered map of Items and returns the IDs of all Items with names matching a given string
std::vector<std::string> CommandParser::getItemIdsByName(const std::unordered_map<std::string, Item*>& itemList, const std::string& objectName)
{
	std::vector<std::string> matches;

	for (const auto& pair : itemList) {
		if (normalize(pair.second->getName()) == objectName) {
			matches.push_back(pair.first);
		}
	}

	return matches; // empty if no matches
}

// This function checks two unordered maps of Items and returns the IDs of all Items with names matching a given string
std::vector<std::string> CommandParser::getAllItemIdsByName(const std::unordered_map<std::string, Item*>& inventory, const std::unordered_map<std::string, Item*>& roomItems, const std::string& objectName)
{
	// Get all matching IDs by name (from room and inventory)
	std::vector<std::string> matches;

	// Check room items
	auto roomMatches = getItemIdsByName(roomItems, objectName);
	matches.insert(matches.end(), roomMatches.begin(), roomMatches.end());

	// Check player inventory
	auto inventoryMatches = getItemIdsByName(inventory, objectName);
	matches.insert(matches.end(), inventoryMatches.begin(), inventoryMatches.end());

	return matches;
}

// Takes a vector of strings, matches representing the IDs of Items, and if that is empty it returns an empty string. If there is 1 string in matches, it returns that string. If there are multiple it returns a string combining all of them
std::string CommandParser::resolveSingleItemId(const std::unordered_map<std::string, Item*>& itemList, const std::string& objectName)
{
	auto matches = getItemIdsByName(itemList, objectName);

	if (matches.empty()) {
		return "";
	}

	if (matches.size() == 1) {
		return matches[0];
	}

	// Multiple matches found
	std::string options;
	for (auto& id : matches) {
		options += itemList.at(id)->getName() + " (" + id + "), ";
	}
	if (!options.empty()) {
		options.pop_back(); options.pop_back(); // remove trailing ", "
		writeMessage("Multiple {object1}s found: {object2}", objectName, options);
	}


	return "";
}

std::string CommandParser::resolveAllSingleItemId(const std::unordered_map<std::string, Item*>& inventory, const std::unordered_map<std::string, Item*>& roomItems, const std::string& objectName)
{
	auto matches = getAllItemIdsByName(inventory, roomItems, objectName);

	if (matches.empty()) {
		return "";
	}

	if (matches.size() == 1) {
		return matches[0];
	}

	// Multiple matches: prompt player
	std::string options;
	for (auto& id : matches) {
		Item* itemPtr = roomItems.count(id) ? roomItems.at(id) : inventory.at(id);
		options += itemPtr->getName() + " (" + id + "), ";
	}
	if (!options.empty()) {
		options.pop_back(); options.pop_back(); // remove trailing ", "
		writeMessage("Multiple {object1}s found: {object2}", options);
	}
	return "";
}

// Split string helper
std::vector<std::string> CommandParser::splitString(std::string& input, char delimiter)
{
	std::istringstream iss(input);

	std::vector<std::string> tokens;

	std::string token;

	// TODO: Have this check one word ahead to check for phrasal verbs like "pick up"

	while (std::getline(iss, token, delimiter)) {
		if (token == "on" || token == "with" || token == "to") {
			continue; // skip specified prepositions
		}
		tokens.push_back(token);
	}

	return tokens;
}

// Is valid word bool checks if a word is present in an unordered map of items or objects
bool CommandParser::isValidWord(const std::unordered_map<std::string, Item*>& inventory, const std::unordered_map<std::string, Item*>& roomItems, const std::string& word)
{


	for (const auto& pair : roomItems) {
		if (normalize(pair.second->getName()) == word) {
			return true;
		}
	}
	for (const auto& pair : inventory) {
		if (normalize(pair.second->getName()) == word) {
			return true;
		}
	}
	return false;
}

// Find longest matching object in a vector of tokens
CommandParser::ObjectMatch CommandParser::findLongestMatchingObject(int startIndex, int tokensLength, const std::vector<std::string>& tokens, const std::unordered_map<std::string, Item*>& inventory, const std::unordered_map<std::string, Item*>& roomItems, int ignoreIndex1, int ignoreIndex2, int ignoreIndex3) {
	

	// Set startIndex to the first non-ignored index
	while (startIndex < tokensLength &&
		(startIndex == ignoreIndex1 || startIndex == ignoreIndex2 || startIndex == ignoreIndex3)) {
		startIndex++;
	}

	if (startIndex >= tokensLength) {
		return { "", 0, false };

	}

	std::string longestMatch = tokens[startIndex];
	std::string currentObject = longestMatch;
	int tokensUsed = 1;
	bool foundValid = isValidWord(inventory, roomItems, currentObject);  //The boolean will be set instantly depending whether or not the first non-ignored word was "valid"
	
	// Check for multi-word objects:
	for (int i = startIndex + 1; i < tokensLength; ++i) {
		if (i == ignoreIndex1 || i == ignoreIndex2 || i == ignoreIndex3) continue;

		currentObject += " " + tokens[i];
		if (isValidWord(inventory, roomItems, currentObject)) {
			longestMatch = currentObject;
			tokensUsed = i - startIndex + 1;;
			foundValid = true;
		}
	}

	return { longestMatch, tokensUsed, foundValid };
}

//                END HELPER FUNCTIONS


// CLASS METHOD DEFINITIONS

// Constructor
CommandParser::CommandParser(Player* p, bool& runningFlag)
	: player(p), running(runningFlag) {

	// Initialize verbs umap
	verbs["help"] = &CommandParser::handleHelp;
	verbs["h"] = &CommandParser::handleHelp;
	verbs["use"] = &CommandParser::handleUse;
	verbs["open"] = &CommandParser::handleOpen;
	verbs["pick"] = &CommandParser::handlePick;
	verbs["take"] = &CommandParser::handleTake;
	verbs["grab"] = &CommandParser::handleTake;
	verbs["get"] = &CommandParser::handleTake;
	verbs["drop"] = &CommandParser::handleDrop;
	verbs["put"] = &CommandParser::handlePut;
	verbs["quit"] = &CommandParser::handleQuit;
	verbs["exit"] = &CommandParser::handleQuit;
	verbs["q"] = &CommandParser::handleQuit;
	verbs["inventory"] = &CommandParser::handleInventory;
	verbs["i"] = &CommandParser::handleInventory;
	verbs["look"] = &CommandParser::handleLook;
	verbs["examine"] = &CommandParser::handleExamine;
	verbs["go"] = &CommandParser::handleGo;
	verbs["unlock"] = &CommandParser::handleUnlock;
	// sit
	// lay
	// tear (sheet?)
	// throw
	// punch
	// go (north, west, door)
	// enter


	// Initialize prepositions set
	prepositions.insert("up");
	prepositions.insert("down");
	prepositions.insert("under");
	prepositions.insert("on");
	prepositions.insert("to");
	prepositions.insert("with");
	prepositions.insert("at");
}

// Parse function (primary function to interpret player input and delegate work to handler functions)
void CommandParser::parse(std::string& input) {

	// Declare variables
	std::string verb = "";
	std::string preposition = "";
	std::string object1 = "";
	std::string object2 = "";
	int verbIndex = -1;
	int prepIndex = -1;
	int object1Index = -1;
	int object1TokenCount = 0;
	int object1TokensUsed = 0;

	// Declare command struct

	ParsedCommand cmd;

	// Lower case input
	std::transform(input.begin(), input.end(), input.begin(),
		[](unsigned char c) { return std::tolower(c); });

	// Add each separate word to tokens vector
	std::vector<std::string> tokens = splitString(input, ' ');
	


	// Get verb
	for (int i = 0; i < tokens.size(); i++) {
		if (verbs.find(tokens[i]) != verbs.end()) {
			cmd.verb = tokens[i];
			cmd.indexMap["verb"] = i;
			break;
		}
	}

	// preposition
	for (int i = 0; i < tokens.size(); i++) {
		if (i != cmd.indexMap["verb"]) {
			if (prepositions.count(tokens[i])) {
				cmd.preposition = tokens[i];
				cmd.indexMap["preposition"] = i;
				break;
			}
		}
	}
	
	// object1
	for (int i = 0; i < tokens.size(); i++) {
		ObjectMatch objectMatch = findLongestMatchingObject(i, static_cast<int>(tokens.size()), tokens, player->getInventory(), player->getCurrentRoom()->getRoomItems(), cmd.indexMap["verb"], cmd.indexMap["preposition"]);
		if (!objectMatch.name.empty()) {
			cmd.object1 = objectMatch.name;
			cmd.indexMap["object1"] = i;
			object1TokensUsed = objectMatch.tokenCount;
			break;
		}
	}

	// object2
	for (int i = cmd.indexMap["object1"] + object1TokensUsed; i < tokens.size(); i++) {
		ObjectMatch objectMatch = findLongestMatchingObject(i, static_cast<int>(tokens.size()), tokens, player->getInventory(), player->getCurrentRoom()->getRoomItems(), cmd.indexMap["verb"], cmd.indexMap["preposition"]);
		cmd.object2 = objectMatch.name;
		cmd.indexMap["object2"] = i;
		break;
	}

	

	// call verb handler function
	auto it = verbs.find(cmd.verb);
	if (it != verbs.end()) {
		(this->*verbs[cmd.verb])(cmd);
	}
	else {
		writeMessage(MSG_DONT_KNOW_HOW);
	}
	

}

// Message writer function takes a message template and optionally an object variable
void CommandParser::writeMessage(const std::string& msgTemplate, const std::string& object1Name, const std::string& object2Name)
{
	std::cout << "\n";

	std::string output = msgTemplate;
	if (!object1Name.empty()) {
		size_t pos = output.find("{object1}");
		if (pos != std::string::npos) {
			output.replace(pos, 9, object1Name);
		}
	}
	if (!object2Name.empty()) {
		size_t pos = output.find("{object2}");
		if (pos != std::string::npos) {
			output.replace(pos, 9, object2Name);
		}
	}
	std::cout << output << "\n";
}



// VERB HANDLERS

// Use handler
void CommandParser::handleUse(ParsedCommand& cmd) {

	if (cmd.object2.empty()) {
		// Handle single-object case
		if (cmd.object1 == "door") {
			if (doorLocked) {
				writeMessage("The door is locked, you will need to use the key on it first.");
			}
			else if (!doorLocked && !doorOpen) {
				writeMessage("You open the door.");
				doorOpen = true;
			}
			else {
				writeMessage("You close the door.");
				doorOpen = false;
			}	
		}
		else {
			writeMessage(MSG_DONT_KNOW_HOW);
		}
	}
	else {
		// Handle two object case
		if ((cmd.object1 == "key" && cmd.object2 == "door") ||
			(cmd.object2 == "key" && cmd.object1 == "door")) {
			if (doorLocked) {
				writeMessage("You unlock the door.");
				doorLocked = false;
			}
			else {
				writeMessage("The door is already unlocked.");
			}
		}
		else {
			writeMessage(MSG_DONT_KNOW_HOW);
		}
	}
}

// Open handler
void CommandParser::handleOpen(ParsedCommand& cmd)
{
}

// Inventory handler
void CommandParser::handleInventory(ParsedCommand& cmd)
{
	writeMessage(player->printInventory());
}

// Drop handler
void CommandParser::handleDrop(ParsedCommand& cmd)
{
	auto& inventory = player->getInventory();
	auto& roomItems = player->getCurrentRoom()->getRoomItems();

	// Get all item IDs in inventory matching the input
	auto matches = getItemIdsByName(inventory, cmd.object1);

	if (matches.empty()) {
		writeMessage(MSG_DONT_HAVE, cmd.object1);
		return;
	}

	// If only one match, drop immediately
	if (matches.size() == 1) {
		const std::string& targetId = matches[0];
		roomItems[targetId] = inventory[targetId];
		inventory.erase(targetId);
		writeMessage(MSG_DROP, inventory[targetId]->getName());
		return;
	}

	// Multiple matches: build choice list
	std::vector<Choice> choices;
	for (const auto& id : matches) {
		Item* itemPtr = inventory[id]; // capture pointer
		choices.push_back(Choice{
			itemPtr->getName() + " (" + id + ")", // label shown to player
			[this, &inventory, &roomItems, id, itemPtr]() { // action when chosen
				roomItems[id] = inventory[id];
				inventory.erase(id);
				writeMessage(MSG_DROP, itemPtr->getName());
			}
			});
	}

	// Build options string for display
	std::string options;
	for (size_t i = 0; i < choices.size(); ++i) {
		options += std::to_string(i + 1) + " - " + choices[i].label + "\n";
	}

	// Use promptChoice to show the options to the player
	MultiMsg msgs{
		MSG_MULTI_ITEMS,
		cmd.object1,
		options,
		MSG_SELECT_CHOICE
	};

	promptChoice(choices, msgs);
}

void CommandParser::handlePut(ParsedCommand& cmd)
{
	if (cmd.preposition == "down") {
		handleDrop(cmd);
	}
}

// Take handler
void CommandParser::handleTake(ParsedCommand& cmd)
{
	auto& inventory = player->getInventory();
	auto& roomItems = player->getCurrentRoom()->getRoomItems();

	// get all item IDs matching the input
	auto matches = getItemIdsByName(roomItems, cmd.object1);

	if (matches.empty()) {
		writeMessage(MSG_DONT_SEE, cmd.object1);
		return;
	}

	// If only one match, take immediately

	if (matches.size() == 1) {
		const std::string& targetId = matches[0];

		if (!roomItems[targetId]->isMoveable()) { // if unmoveable, can't take
			writeMessage(MSG_CANT_TAKE, cmd.object1);
			return;
		}

		if (inventory.count(targetId)) {
			writeMessage(MSG_ALREADY_HAVE, cmd.object1);
			return;
		}

		inventory[targetId] = roomItems[targetId];
		roomItems.erase(targetId);
		writeMessage(MSG_TAKE, cmd.object1);
		return;
	}

	// If multiple, build choice list
	std::vector<Choice> choices;
	for (const auto& id : matches) {
		Item* itemPtr = roomItems[id]; // capture the pointer
		choices.push_back(Choice{
			roomItems[id]->getName() + " (" + id + ")", // label shown to player
			[this, &inventory, &roomItems, id, itemPtr]() { // action when chosen
				if (!roomItems[id]->isMoveable()) {
					writeMessage(MSG_CANT_TAKE, roomItems[id]->getName());
					return;
				}
				if (inventory.count(id)) {
					writeMessage(MSG_ALREADY_HAVE, roomItems[id]->getName());
					return;
				}
				inventory[id] = roomItems[id];
				roomItems.erase(id);
				writeMessage(MSG_TAKE, itemPtr->getName());
			}
		});
	}

	std::string options;
	for (size_t i = 0; i < choices.size(); ++i) {
		options += std::to_string(i + 1) + " - " + choices[i].label + "\n";
	}

	// use promptChoice to show the options to the player
	MultiMsg msgs{
		MSG_MULTI_ITEMS,
		cmd.object1,
		options,
		MSG_SELECT_CHOICE
	};

	promptChoice(choices, msgs);
}

// Pick handler
void CommandParser::handlePick(ParsedCommand& cmd)
{
	if (cmd.preposition == "up") {
		handleTake(cmd);
	}
}

// Look handler
void CommandParser::handleLook(ParsedCommand& cmd)
{
	if (cmd.preposition.empty() || cmd.preposition == "at") {
		handleExamine(cmd);
	}
}

// Examine handler
void CommandParser::handleExamine(ParsedCommand& cmd) {
	std::string target = cmd.object1;

	// No object specified - look around the room
	static const std::unordered_set<std::string> roomWords = { "room", "around", "area" };
	if (target.empty() || roomWords.count(target) || target == player->getCurrentRoom()->getName()) {
		writeMessage(player->getCurrentRoom()->describeSelf());
		return;
	}

	auto& inventory = player->getInventory();
	auto& roomItems = player->getCurrentRoom()->getRoomItems();

	// Get all item IDs matching the input in both room and inventory
	auto matches = getAllItemIdsByName(inventory, roomItems, target);

	if (matches.empty()) {
		writeMessage(MSG_DONT_SEE, target);
		return;
	}

	// If only one match, examine immediately
	if (matches.size() == 1) {
		Item* itemPtr = roomItems.count(matches[0]) ? roomItems.at(matches[0]) : inventory.at(matches[0]);
		writeMessage(itemPtr->getDescription());
		return;
	}

	// Multiple matches: build choice list
	std::vector<Choice> choices;
	for (const auto& id : matches) {
		Item* itemPtr = roomItems.count(id) ? roomItems.at(id) : inventory.at(id);
		choices.push_back(Choice{
			itemPtr->getName() + " (" + id + ")", // label shown to player
			[this, itemPtr]() { // action when chosen
				writeMessage(itemPtr->getDescription());
			}
			});
	}

	// Build options string for display
	std::string options;
	for (size_t i = 0; i < choices.size(); ++i) {
		options += std::to_string(i + 1) + " - " + choices[i].label + "\n";
	}

	// Use promptChoice to show the options to the player
	MultiMsg msgs{
		MSG_MULTI_ITEMS,
		target,
		options,
		MSG_SELECT_CHOICE
	};

	promptChoice(choices, msgs);
}


// Go handler
void CommandParser::handleGo(ParsedCommand& cmd)
{
	std::string dir = cmd.object1;
	// If player entered cardinal direction, and cardinal direction leads to another location, and no locked door, change location to new location

	// Check to see if direction exits
	auto& exits = player->getCurrentRoom()->getExits();

	auto it = exits.find(dir);
	if (it == exits.end()) {
		writeMessage(MSG_NO_EXIT, dir);
		return;
	}

	auto& exitList = it->second;


	if (exitList.size() == 1) {
		player->setCurrentRoom(exitList[0].room);
		writeMessage(player->getCurrentRoom()->describeSelf());
		return;
	}

	// Set up choices vector for promptChoice
	std::vector<Choice> choices;
	for (auto& exit : exitList) {
		choices.push_back({ exit.label, [this, &exit]() { player->setCurrentRoom(exit.room); writeMessage(player->getCurrentRoom()->describeSelf()); } });
	}

	// format choice text output
	std::string options;
	for (size_t i = 0; i < exitList.size(); ++i)
		options += std::to_string(i + 1) + " - " + exitList[i].label + "\n";

	MultiMsg msgs{
		MSG_MULTI_EXITS,
		dir,
		options,
		MSG_SELECT_CHOICE
	};

	// call promptChoice
	promptChoice(choices, msgs);

	//// write messages
	//writeMessage("Multiple exits to {object1}: \n{object2}", dir, options);
	//writeMessage("Type the number of your choice and press enter.");

}

// Unlock handler
void CommandParser::handleUnlock(ParsedCommand& cmd)
{
	std::string target = cmd.object1;

	if (target.empty()) {
		writeMessage(MSG_VERB_WHAT, cmd.verb);
		return;
	}

	auto& inventory = player->getInventory();
	auto& roomItems = player->getCurrentRoom()->getRoomItems();

	// Get all item IDs matching the input in both room and inventory
	auto matches = getAllItemIdsByName(inventory, roomItems, target);

	if (matches.empty()) {
		writeMessage(MSG_DONT_SEE, target);
		return;
	}

	// If only one match, examine immediately
	if (matches.size() == 1) {
		Item* itemPtr = roomItems.count(matches[0]) ? roomItems.at(matches[0]) : inventory.at(matches[0]);
		if (itemPtr->getName() == "door") {
			itemPtr->toggleLock();
			std::string msg = itemPtr->isLocked() ? "The {object1} is locked." : "The {object1} is unlocked.";
			writeMessage(msg, itemPtr->getName());
			return;
		}
		else {
			writeMessage(MSG_DONT_KNOW_HOW);
		}
	}

	// Multiple matches: build choice list
	std::vector<Choice> choices;
	for (const auto& id : matches) {
		Item* itemPtr = roomItems.count(id) ? roomItems.at(id) : inventory.at(id);
		choices.push_back(Choice{
			itemPtr->getName() + " (" + id + ")", // label shown to player
			[this, itemPtr]() { // action when chosen
				if (itemPtr->getName() == "door") {
					itemPtr->toggleLock();
					std::string msg = itemPtr->isLocked() ? "The {object1} is locked." : "The {object1} is unlocked.";
					writeMessage(msg, itemPtr->getName());
					return;
				}
				else {
					writeMessage(MSG_DONT_KNOW_HOW);
				}
			}
			});
	}

	// Build options string for display
	std::string options;
	for (size_t i = 0; i < choices.size(); ++i) {
		options += std::to_string(i + 1) + " - " + choices[i].label + "\n";
	}

	// Use promptChoice to show the options to the player
	MultiMsg msgs{
		MSG_MULTI_ITEMS,
		target,
		options,
		MSG_SELECT_CHOICE
	};

	promptChoice(choices, msgs);
	
}

// Help handler for displaying available commands
void CommandParser::handleHelp(ParsedCommand& cmd)
{
	writeMessage(
		"You remember a few useful actions:\n\n"
		"- look [object]: Look around or at an object.\n"
		"- go [direction]: Move in a direction (north, south, east, west, etc.).\n"
		"- take / pick up [object]: Pick up an item.\n"
		"- drop [object]: Drop an item from your inventory.\n"
		"- use [object1] [preposition] [object2]: Use an item on another item.\n"
		"- inventory / i: View your current inventory.\n"
		"- quit / exit / q: Exit the game.\n"
		"- help: Display this help message.\n"
		);
}


// Quit handler
void CommandParser::handleQuit(ParsedCommand& cmd)
{
	running = false;
}
