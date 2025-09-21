#include <iostream>
#include "CommandParser.hpp"
#include <fstream>
#include <sstream>

// Internal helpers

// Split string helper
std::vector<std::string> CommandParser::splitString(std::string& input, char delimiter)
{
	std::istringstream iss(input);

	std::vector<std::string> tokens;

	std::string token;

	while (std::getline(iss, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}

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

}



// VERB HANDLERS

// Use handler
void CommandParser::handleUse(const std::string& object1, const std::string& object2) {
	if (object2.empty()) {
		// Handle single-object case
	}
	else {
		// Handle two object case
	}
}
