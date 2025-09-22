#include "CommandParser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>

// Internal helpers

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
bool CommandParser::isValidWord(const std::vector<std::string>& tokens, int index, const std::unordered_map<std::string, std::string>& inventory, const std::unordered_map<std::string, std::string>& roomObjects, int ignoreIndex1, int ignoreIndex2)
{
	if (index == ignoreIndex1 || index == ignoreIndex2)
		return false;
	if (inventory.find(tokens[index]) != inventory.end())
		return true;
	if (roomObjects.find(tokens[index]) != roomObjects.end())
		return true;
	return false;
}


// Constructor
CommandParser::CommandParser() {
	verbs["use"] = &CommandParser::handleUse;
	verbs["open"] = &CommandParser::handleOpen;
	// later: verbs["look"] = &CommandParser::handleLook;
}

// Parse
void CommandParser::parse(std::string& input, std::unordered_map<std::string, std::string>& inventory, std::unordered_map<std::string, std::string>& roomObjects, bool& testSuccess) {

	// Declare variables
	std::string verb = "";
	std::string object1 = "";
	std::string object2 = "";
	int verbIndex = -1;
	int object1Index = -1;

	std::transform(input.begin(), input.end(), input.begin(),
		[](unsigned char c) { return std::tolower(c); });

	// Add each separate word to tokens vector
	std::vector<std::string> tokens = splitString(input, ' ');

	// Get verb
	for (int i = 0; i < tokens.size(); i++) {
		if (verbs.find(tokens[i]) != verbs.end()) {
			verb = tokens[i];
			verbIndex = i;
			break;
		}
	}
	
	// object1
	for (int i = 0; i < tokens.size(); i++) {
		if (isValidWord(tokens, i, inventory, roomObjects, verbIndex)) {
			object1 = tokens[i];
			object1Index = i;
			break;
		}
	}

	// object2
	for (int i = 0; i < tokens.size(); i++) {
		if (isValidWord(tokens, i, inventory, roomObjects, object1Index, verbIndex)) {
			object2 = tokens[i];
			break;
		}
	}

	// Set bool to true if verb and object are found   FOR TESTING
	testSuccess = (!verb.empty() && !object1.empty());

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

	//                                       END OF TESTING

	(this->*verbs[verb])(object1, object2);

}



// VERB HANDLERS

// Use handler
void CommandParser::handleUse(const std::string& object1, const std::string& object2) {
	if (object2.empty()) {
		// Handle single-object case
		if (object1 == "door") {
			if (doorLocked) {
				std::cout << "The door is locked, you will need to use the key on it first.\n";
			}
			else if (!doorLocked && !doorOpen) {
				std::cout << "You open the door.\n";
				doorOpen = true;
			}
			else {
				std::cout << "You close the door.\n";
				doorOpen = false;
			}	
		}
		else {
			std::cout << msgDontKnow;
		}
	}
	else {
		// Handle two object case
		if ((object1 == "key" && object2 == "door") ||
			(object2 == "key" && object1 == "door")) {
			if (doorLocked) {
				std::cout << "You unlock the door.\n";
				doorLocked = false;
			}
			else {
				std::cout << "The door is already unlocked.\n";
			}
		}
		else {
			std::cout << msgDontKnow;
		}
	}
}

void CommandParser::handleOpen(const std::string& object1, const std::string& object2)
{
}
