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
bool CommandParser::isValidWord(const std::unordered_map<std::string, Item>& inventory, const std::unordered_map<std::string, Item>& roomItems, std::string word)
{
	if (inventory.find(word) != inventory.end())
		return true;
	if (roomItems.find(word) != roomItems.end())
		return true;
	return false;
}

// Find longest matching object in a vector of tokens
CommandParser::ObjectMatch CommandParser::findLongestMatchingObject(int startIndex, int tokensLength, const std::vector<std::string>& tokens, const std::unordered_map<std::string, Item>& inventory, const std::unordered_map<std::string, Item>& roomItems, int ignoreIndex1, int ignoreIndex2, int ignoreIndex3) {
	std::string longestMatch = "";
	int tokensUsed = 0;
	std::string currentObject = tokens[startIndex];

	// Check for single-word object:
	if (isValidWord(inventory, roomItems, currentObject)) {
		longestMatch = currentObject;
		tokensUsed = 1;
	}
	
	// Check for multi-word objects:
	for (int i = startIndex + 1; i < tokensLength; i++) {
		if (i != ignoreIndex1 && i != ignoreIndex2 && i != ignoreIndex3) {
			currentObject = currentObject + " " + tokens[i];
			if (isValidWord(inventory, roomItems, currentObject)) {
				longestMatch = currentObject;
				tokensUsed = i - startIndex + 1;;
			}
		}
	}

	return { longestMatch, tokensUsed };
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
	verbs["quit"] = &CommandParser::handleQuit;
	verbs["exit"] = &CommandParser::handleQuit;
	verbs["inventory"] = &CommandParser::handleInventory;
	verbs["look"] = &CommandParser::handleLook;
	verbs["examine"] = &CommandParser::handleExamine;

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
void CommandParser::parse(std::string& input, bool& testSuccess) {

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
		break;
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
	// TODO Replace with inventory print function from player class
	writeMessage(player->printInventory());
}

void CommandParser::handleDrop(ParsedCommand& cmd)
{
	if (player->getInventory().find(cmd.object1) != player->getInventory().end()) {
		room->getRoomItems()[cmd.object1] = player->getInventory().find(cmd.object1)->second;
		player->getInventory().erase(cmd.object1);
		writeMessage(MSG_DROP, cmd.object1);
	}
	else {
		writeMessage(MSG_DONT_HAVE, cmd.object1);
	}
}

// Take handler
void CommandParser::handleTake(ParsedCommand& cmd)
{
	if (room->getRoomItems().find(cmd.object1) != room->getRoomItems().end()) {
		if (!(player->getInventory().find(cmd.object1) != player->getInventory().end())) {
			player->getInventory()[cmd.object1] = room->getRoomItems().find(cmd.object1)->second;
			room->getRoomItems().erase(cmd.object1);
			writeMessage(MSG_TAKE, cmd.object1);
		}
		else {
			writeMessage(MSG_ALREADY_HAVE, cmd.object1);
		}
	}
	else {
		writeMessage(MSG_DONT_SEE, cmd.object1);
	}
}

// Pick handler
void CommandParser::handlePick(ParsedCommand& cmd)
{
	if (cmd.preposition == "up") {
		handleTake(cmd);
	}
}

void CommandParser::handleLook(ParsedCommand& cmd)
{
	if (cmd.preposition.empty() || cmd.preposition == "at") {
		handleExamine(cmd);
	}
}

void CommandParser::handleExamine(ParsedCommand& cmd) {
	std::string target = cmd.object1;

	// No object specified - look around the room
	static const std::unordered_set<std::string> roomWords = { "room", "around", "area" };
	if (target.empty() || roomWords.count(target) || target == room->getName()) {
		writeMessage(room->getDescription());
		return;
	}

	// Check if player has the target
	auto& inventory = player->getInventory();
	auto invIt = inventory.find(target);
	if (invIt != inventory.end()) {
		writeMessage(invIt->second.getDescription());
		return;
	}

	// Check if target is in the room
	auto& roomItems = room->getRoomItems();
	auto roomIt = roomItems.find(target);
	if (roomIt != roomItems.end()) {
		writeMessage(roomIt->second.getDescription());
		return;
	}

	// If target couldn't be located
	writeMessage(MSG_DONT_SEE, target);
}

// Quit handler
void CommandParser::handleQuit(ParsedCommand& cmd)
{
	running = false;
}
