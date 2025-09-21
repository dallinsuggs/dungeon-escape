#pragma once

#include <string>
#include <unordered_map>

class CommandParser {
private:
	std::unordered_map<std::string, void (CommandParser::*)(const std::string&, const std::string&)> verbs;

	// Internal helpers
	std::vector<std::string> splitString(std::string& input, char delimiter = ' ');

	bool isValidWord(
		const std::vector<std::string>& tokens,
		int index,                                // index of token I want to check
		const std::unordered_map<std::string, std::string>& inventory,
		const std::unordered_map<std::string, std::string>& roomObjects,
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
		std::unordered_map<std::string, std::string>& roomObjects,
		bool& testSuccess
	);

	

	// VERB HANDLERS

	// Use handler
	void handleUse(const std::string& object1, const std::string& object2 = "");
};