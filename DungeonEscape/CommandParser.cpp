#include "CommandParser.hpp"


// INTERNAL HELPERS

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
bool CommandParser::isValidWord(const std::vector<std::string>& tokens, int index, const std::unordered_map<std::string, Item>& inventory, const std::unordered_map<std::string, Item>& roomItems, int ignoreIndex1, int ignoreIndex2, int ignoreIndex3)
{
	if (index == ignoreIndex1 || index == ignoreIndex2 || index == ignoreIndex3)
		return false;
	if (inventory.find(tokens[index]) != inventory.end())
		return true;
	if (roomItems.find(tokens[index]) != roomItems.end())
		return true;
	return false;
}
//                END HELPER FUNCTIONS


// CLASS METHOD DEFINITIONS

// Constructor
CommandParser::CommandParser() {

	// Initialize verbs umap
	verbs["use"] = &CommandParser::handleUse;
	verbs["open"] = &CommandParser::handleOpen;
	verbs["pick"] = &CommandParser::handlePick;
	// later: verbs["look"] = &CommandParser::handleLook;

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
void CommandParser::parse(std::string& input, std::unordered_map<std::string, Item>& inventory, std::unordered_map<std::string, Item>& roomItems, bool& testSuccess) {

	// Declare variables
	std::string verb = "";
	std::string preposition = "";
	std::string object1 = "";
	std::string object2 = "";
	int verbIndex = -1;
	int prepIndex = -1;
	int object1Index = -1;
	inventoryMap = &inventory;
	roomItemsMap = &roomItems;

	// Declare command struct

	ParsedCommand cmd;

	// Lower case input
	std::transform(input.begin(), input.end(), input.begin(),
		[](unsigned char c) { return std::tolower(c); });

	// Add each separate word to tokens vector
	std::vector<std::string> tokens = splitString(input, ' ');

	// DEBUGGING:
	//std::cout << "Tokens: ";
	//for (auto& t : tokens) std::cout << t << "|";
	


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

	//std::cout << "\nVerb found: " << verb << "\n"; // DEBUGGING
	
	// object1
	for (int i = 0; i < tokens.size(); i++) {
		if (isValidWord(tokens, i, inventory, roomItems, cmd.indexMap["verb"], cmd.indexMap["preposition"])) {
			cmd.object1 = tokens[i];
			cmd.indexMap["object1"] = i;
			break;
		}
	}

	// object2
	for (int i = 0; i < tokens.size(); i++) {
		if (isValidWord(tokens, i, inventory, roomItems, cmd.indexMap["verb"], cmd.indexMap["preposition"], cmd.indexMap["object1"])) {
			cmd.object2 = tokens[i];
			cmd.indexMap["object2"] = i;
			break;
		}
	}



	// Set bool to true if verb and object are found   FOR TESTING
	testSuccess = (!cmd.verb.empty() && !cmd.object1.empty());

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
	(this->*verbs[cmd.verb])(cmd);

}

// Message writer function takes a message template and optionally an object variable
void CommandParser::writeMessage(const std::string& msgTemplate, const std::string& objectName)
{
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
	// TODO Replace with inventory print function from player class
	writeMessage("Here is your inventory.");
}

// PHRASAL VERB HANDLERS

// PickUp handler
void CommandParser::handlePickUp(std::unordered_map<std::string, Item>& inventory, std::unordered_map<std::string, Item>& roomItems, ParsedCommand& cmd)
{
	if (roomItems.find(cmd.object1) != roomItems.end()) {
		if (!(inventory.find(cmd.object1) != inventory.end())) {
			inventory[cmd.object1] = roomItems.find(cmd.object1)->second;
			roomItems.erase(cmd.object1);
			writeMessage(MSG_PICK_UP, cmd.object1);
		}
		else {
			writeMessage(MSG_ALREADY_HAVE, cmd.object1);;
		}
	}
	else {
		writeMessage(MSG_DONT_SEE, cmd.object1);
	}
}

void CommandParser::handlePick(ParsedCommand& cmd)
{
	std::cout << "Entering handlePick with object1 = " << cmd.object1 << "\n";
	std::cout << "Preposition: " + cmd.preposition + "\n";
	std::cout << "inventoryMap = " << inventoryMap << ", roomItemsMap = " << roomItemsMap << "\n";

	if (cmd.preposition == "up") {
		handlePickUp(*inventoryMap, *roomItemsMap, cmd);
	}
}
