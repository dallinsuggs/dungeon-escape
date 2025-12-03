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
		writeMessage("Multiple {object}s found: {object}", options);
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
		writeMessage("Multiple {object}s found: {object}", options);
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
CommandParser::CommandParser(Player* p, Room* r, bool& runningFlag) 
	: player(p), room(r), running(runningFlag) {

	// Initialize verbs umap
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
		ObjectMatch objectMatch = findLongestMatchingObject(i, static_cast<int>(tokens.size()), tokens, player->getInventory(), room->getRoomItems(), cmd.indexMap["verb"], cmd.indexMap["preposition"]);
		if (!objectMatch.name.empty()) {
			cmd.object1 = objectMatch.name;
			cmd.indexMap["object1"] = i;
			object1TokensUsed = objectMatch.tokenCount;
			break;
		}
	}

	// object2
	for (int i = cmd.indexMap["object1"] + object1TokensUsed; i < tokens.size(); i++) {
		ObjectMatch objectMatch = findLongestMatchingObject(i, static_cast<int>(tokens.size()), tokens, player->getInventory(), room->getRoomItems(), cmd.indexMap["verb"], cmd.indexMap["preposition"]);
		cmd.object2 = objectMatch.name;
		cmd.indexMap["object2"] = i;
		break;
	}

	// write to output file
	std::ofstream outFile("test", std::ios::app);

	if (!outFile) {
		std::cerr << "Could not open file for writing\n";
		return;
	}

	outFile << verb << "|" << object1 << "|" << object2 << "\n";
	outFile.close();

	// write tokens to file
	std::ofstream outFile2("testTokens", std::ios::app);

	if (!outFile2) {
		std::cerr << "Could not open file for writing\n";
		return;
	}

	for (int i = 0; i < tokens.size(); i++) {
		outFile2 << tokens[i];
		if (i < tokens.size() - 1) outFile2 << "|";
	}
	outFile2 << "\n";
	outFile2.close();

	// struct to file output
	std::ofstream outFile3("testStruct", std::ios::app);

	if (!outFile3) {
		std::cerr << "Could not open file for writing\n";
		return;
	}

	outFile3 << "|struct|\n";
	outFile3 << "Verb: " << cmd.verb << "|";
	outFile3 << cmd.indexMap["verb"] << "\n";
	outFile3 << "Preposition: " << cmd.preposition << "|";
	outFile3 << cmd.indexMap["preposition"] << "\n";
	outFile3 << "Object1: " << cmd.object1 << "|";
	outFile3 << cmd.indexMap["object1"] << "\n";
	outFile3 << "Object2: " << cmd.object2 << "|";
	outFile3 << cmd.indexMap["object2"] << "\n";
	outFile3 << "|\n";

	outFile3.close();

	//                                       END OF TESTING

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
void CommandParser::writeMessage(const std::string& msgTemplate, const std::string& objectName)
{
	std::cout << "\n";

	std::string output = msgTemplate;
	if (!objectName.empty()) {
		size_t pos = output.find("{object}");
		if (pos != std::string::npos) {
			output.replace(pos, 8, objectName);
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
	auto& roomItems = room->getRoomItems();
	
	std::string targetId = resolveSingleItemId(inventory, cmd.object1);
	if (targetId.empty()) {
		writeMessage(MSG_DONT_HAVE, cmd.object1);
		return; // either not found or multiple matches
	}

	// Drop the item
	roomItems[targetId] = inventory.at(targetId);
	inventory.erase(targetId);
	writeMessage(MSG_DROP, cmd.object1);
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
	auto& roomItems = room->getRoomItems();

	std::string targetId = resolveSingleItemId(roomItems, cmd.object1);
	if (targetId.empty()) {
		writeMessage(MSG_DONT_SEE, cmd.object1);
		return; // either not found or multiple matches
	}
	// Now proceed with taking the item from the room

	//// If moveable and presently accessible to player, take item
	auto roomIt = roomItems.find(targetId);
	if (!roomIt->second->isMoveable()) {
		writeMessage(MSG_CANT_TAKE, cmd.object1);
		return;
	}

	if (inventory.count(targetId)) {
		writeMessage(MSG_ALREADY_HAVE, cmd.object1);
		return;
	}

	inventory[targetId] = roomIt->second;
	roomItems.erase(roomIt);
	writeMessage(MSG_TAKE, cmd.object1);
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
	if (target.empty() || roomWords.count(target) || target == room->getName()) {
		writeMessage(room->describeSelf());
		return;
	}

	auto& inventory = player->getInventory();
	auto& roomItems = room->getRoomItems();

	std::string targetId = resolveAllSingleItemId(inventory, roomItems, target);
	if (targetId.empty()) return; // either not found or multiple matches

	// Now we know exactly which item to describe
	Item* itemPtr = roomItems.count(targetId) ? roomItems.at(targetId) : inventory.at(targetId);
	writeMessage(itemPtr->getDescription());
}

// Quit handler
void CommandParser::handleQuit(ParsedCommand& cmd)
{
	running = false;
}
