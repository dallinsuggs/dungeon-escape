#include "CommandParser.hpp"
#include <algorithm>
#include <cctype>
#include <string>
#include <thread>
#include <chrono>

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

// Returns true if resolved immediately (0 or 1 match handled now).
// Returns false if it prompted (will call onChosen later).
bool CommandParser::resolveOrPromptItem(
	const std::string& objectName,
	ItemScope scope,
	const std::string& MSG_NO_MATCH,
	const std::function<void(const std::string& id, Item* item)>& onChosen)
{
	auto& inventory = player->getInventory();
	auto& roomItems = player->getCurrentRoom()->getRoomItems();

	struct Match { std::string id; Item* ptr; bool inRoom; };
	std::vector<Match> matches;

	auto addMatches = [&](const std::unordered_map<std::string, Item*>& src, bool inRoomFlag) {
		auto ids = getItemIdsByName(src, objectName);
		for (auto& id : ids) {
			matches.push_back({ id, src.at(id), inRoomFlag });
		}
		};

	if (scope == ItemScope::RoomOnly || scope == ItemScope::RoomAndInventory) {
		addMatches(roomItems, true);
	}
	if (scope == ItemScope::InventoryOnly || scope == ItemScope::RoomAndInventory) {
		addMatches(inventory, false);
	}

	if (matches.empty()) {
		writeMessage(MSG_NO_MATCH, objectName);
		return true; // handled now (nothing pending)
	}

	if (matches.size() == 1) {
		onChosen(matches[0].id, matches[0].ptr);
		return true; // handled now
	}

	// multi -> build choices
	std::vector<Choice> choices;
	choices.reserve(matches.size());

	for (auto& m : matches) {
		Item* itemPtr = m.ptr;
		std::string id = m.id;

		choices.push_back(Choice{
			itemPtr->getName() + " (" + id + ")",
			[this, onChosen, id, itemPtr]() {
				onChosen(id, itemPtr);
			}
			});
	}

	std::string options;
	for (size_t i = 0; i < choices.size(); ++i) {
		options += std::to_string(i + 1) + " - " + choices[i].label + "\n";
	}

	MultiMsg msgs{
		MSG_MULTI_ITEMS,
		objectName,
		options,
		MSG_SELECT_CHOICE
	};

	promptChoice(choices, msgs);
	return false; // will resolve later via onChosen
}

//                END HELPER FUNCTIONS


// CLASS METHOD DEFINITIONS

// Constructor
CommandParser::CommandParser(Player* p, bool& runningFlag, FileManager* fm)
	: player(p), running(runningFlag), fileManager(fm) {

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
	// Basic validation
	if (cmd.object1.empty()) { writeMessage(MSG_VERB_WHAT, cmd.verb); return; }
	if (cmd.object2.empty()) { writeMessage(MSG_VERB_WHAT_ON_WHAT, cmd.object1, cmd.object2); return; }
	if (cmd.preposition != "on") { writeMessage(MSG_DONT_KNOW_HOW); return; }

	// Copy what we need (IMPORTANT: don’t capture cmd by reference)
	std::string obj1Name = cmd.object1;
	std::string obj2Name = cmd.object2;
	std::string prep = cmd.preposition;

	// Step 1: resolve the item being used (inventory only)
	bool finishedNow1 = resolveOrPromptItem(
		obj1Name,
		ItemScope::InventoryOnly,
		MSG_DONT_HAVE,
		[this, obj1Name, obj2Name, prep](const std::string& id1, Item* item1)
		{
			// Step 2: resolve the target being used on
			bool finishedNow2 = resolveOrPromptItem(
				obj2Name,
				ItemScope::RoomAndInventory,
				MSG_DONT_SEE,
				[this, id1, item1, obj1Name, obj2Name, prep](const std::string& id2, Item* item2)
				{
					// Step 3: now we have both, do the actual use logic
					if (!item1 || !item2) { writeMessage(MSG_DONT_KNOW_HOW); return; }

					// Example: "use animal bone on door"
					if (prep == "on" && item1->getName() == "animal bone" && item2->getName() == "door") {
						
						item2->setLocked(false);
						writeMessage(MSG_PICK_LOCK, item2->getName());
						return;
					}

					writeMessage(MSG_DONT_KNOW_HOW);
				}
			);

			// If it prompted for object2, we must stop here and wait for the numeric input.
			if (!finishedNow2) return;
		}
	);

	// If it prompted for object1, stop now and wait for numeric input.
	if (!finishedNow1) return;

	// If finishedNow1 == true, its callback already ran and either:
	// - completed the chain immediately, or
	// - prompted for object2 and returned.

	//if (cmd.object1.empty() || cmd.object2.empty()) {
	//	writeMessage("Use what on what?");
	//	return;
	//}

	//std::string item1 = normalize(cmd.object1);
	//std::string item2 = normalize(cmd.object2);

	//// Robust full-input check (ignores the parser bug with "on")
	//std::string fullInput = normalize(cmd.object1 + " " + cmd.object2);

	//// ── ESCAPE INTERACTIONS (checked FIRST) ─────────────────────────────

	//// 1. Animal bone on door
	//if (fullInput.find("bone") != std::string::npos && fullInput.find("door") != std::string::npos) {
	//	auto doorIds = getItemIdsByName(player->getCurrentRoom()->getRoomItems(), "door");
	//	if (doorIds.empty()) {
	//		writeMessage("You don't see a door here.");
	//		return;
	//	}
	//	Item* door = fileManager->getItem(doorIds[0]);
	//	if (door) {
	//		door->setLocked(false);
	//		writeMessage("You carefully work the animal bone in the lock... *click!* The door swings open.");
	//	}
	//	return;
	//}

	//// 2. Brick on toilet
	//if (fullInput.find("brick") != std::string::npos && fullInput.find("toilet") != std::string::npos) {
	//	Item* toilet = fileManager->getItem("guardroom_toilet");
	//	if (!toilet) {
	//		writeMessage("There is no toilet here.");
	//		return;
	//	}
	//	if (!toilet->isLocked()) {
	//		writeMessage("The toilet seat is already pried open.");
	//		return;
	//	}
	//	toilet->setLocked(false);
	//	writeMessage("You wedge the brick under the wooden seat and pry with all your strength. The seat cracks open, revealing a dark, foul-smelling chute that drops straight down to the moat.");
	//	return;
	//}

	//// 3. Rope on toilet → WIN CONDITION
	//if (fullInput.find("rope") != std::string::npos && fullInput.find("toilet") != std::string::npos) {
	//	Item* toilet = fileManager->getItem("guardroom_toilet");
	//	if (!toilet || toilet->isLocked()) {
	//		writeMessage("The toilet seat is still fixed in place. You need to pry it open first.");
	//		return;
	//	}

	//	auto ropeIds = getItemIdsByName(player->getInventory(), "rope");
	//	if (ropeIds.size() >= 3) {
	//		writeMessage("You quickly knot the three ropes together into one long line, tie it securely around the toilet frame, and lower yourself into the stinking chute.\n\n"
	//			"After a long, slippery descent you splash into the cold moat water below... and swim to freedom under the cover of night.\n\n"
	//			"You have escaped the dungeon!\n\n"
	//			"Thank you for playing Dungeon Escape.");
	//		running = false;
	//	}
	//	else {
	//		writeMessage("You only have " + std::to_string(ropeIds.size()) + " rope(s). You need three lengths knotted together to reach the bottom safely.");
	//	}
	//	return;
	//}
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

	bool finishedNow = resolveOrPromptItem(
		cmd.object1,
		ItemScope::InventoryOnly,
		MSG_DONT_HAVE,
		[this, &inventory, &roomItems](const std::string& chosenId, Item* itemPtr)
		{
			// chosenId is guaranteed to exist in inventory for InventoryOnly scope
			roomItems[chosenId] = itemPtr;
			inventory.erase(chosenId);
			writeMessage(MSG_DROP, itemPtr->getName());
		}
	);

	// If it prompted, stop here and wait for numeric input.
	if (!finishedNow) return;
}


// Put handler
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

	bool finishedNow = resolveOrPromptItem(
		cmd.object1,
		ItemScope::RoomOnly,
		MSG_DONT_SEE,
		[this, &inventory, &roomItems](const std::string& chosenId, Item* itemPtr)
		{
			// itemPtr == roomItems.at(chosenId) for RoomOnly scope
			if (!itemPtr->isMoveable()) {
				writeMessage(MSG_CANT_TAKE, itemPtr->getName());
				return;
			}

			// This check is kind of redundant for "take" (it’s not in inventory yet),
			// but keep it if you want to be safe.
			if (inventory.count(chosenId)) {
				writeMessage(MSG_ALREADY_HAVE, itemPtr->getName());
				return;
			}

			inventory[chosenId] = itemPtr;
			roomItems.erase(chosenId);
			writeMessage(MSG_TAKE, itemPtr->getName());
		}
	);

	// If it prompted, we wait for numeric selection.
	if (!finishedNow) return;

	// If finishedNow == true, either:
	// - it already took the item via callback (single match), OR
	// - it printed MSG_DONT_SEE (no match)
	// so nothing else to do here.
}


// Pick handler
void CommandParser::handlePick(ParsedCommand& cmd)
{
	if (cmd.preposition == "up") {
		handleTake(cmd);
	}
	else {
		writeMessage(MSG_DONT_KNOW_HOW);
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
	if (cmd.preposition == "down") {                 // go down <object>

		// Player typed just "go down" (no object)
		if (cmd.object1.empty()) {
			writeMessage(MSG_VERB_WHAT, "go down");
			return;
		}

		bool finishedNow = resolveOrPromptItem(
			cmd.object1,
			ItemScope::RoomOnly,
			MSG_DONT_SEE,
			[this](const std::string& chosenId, Item* itemPtr) {
				// (chosenId unused here, but fine to keep in the signature)
				if (itemPtr && itemPtr->getName() == "toilet") {
					writeMessage(MSG_TOILET_DEATH);
				}
				else {
					writeMessage(MSG_VERB_WHAT, "go down");
				}
			}
		);

		if (!finishedNow) return;

		return;
	}



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
	
	//// can pass helper
	//auto canPassExit = [&](const Room::ExitOption& exit) -> bool {
	//	if (exit.doorId.empty()) {

	//		return true;
	//	}

	//	auto& roomItems = player->getCurrentRoom()->getRoomItems();
	//	auto doorIt = roomItems.find(exit.doorId);
	//	if (doorIt == roomItems.end()) {
	//		return true;
	//	}

	//	Item* door = doorIt->second;
	//	if (door->isLocked()) {
	//		writeMessage(MSG_LOCKED);
	//		return false;
	//	}
	//	return true;
	//};

	auto canPassExit = [&](const Room::ExitOption& exit) -> bool {

		writeMessage(std::string("DEBUG: exit.doorId = '") + exit.doorId + "'");

		if (exit.doorId.empty()) {
			writeMessage("DEBUG: doorId is empty -> treating as open passage");
			return true;
		}

		auto& roomItems = player->getCurrentRoom()->getRoomItems();

		writeMessage(std::string("DEBUG: roomItems.count(doorId) = ")
			+ (roomItems.count(exit.doorId) ? "1" : "0"));

		auto doorIt = roomItems.find(exit.doorId);
		if (doorIt == roomItems.end()) {
			writeMessage("DEBUG: doorId not found in roomItems -> letting you pass (your current behavior)");
			return true; // you can change to false later
		}

		Item* door = doorIt->second;

		writeMessage(std::string("DEBUG: door->isLocked() = ")
			+ (door->isLocked() ? "true" : "false"));

		if (door->isLocked()) {
			writeMessage(MSG_LOCKED);
			return false;
		}

		return true;
		};
       ///// end deubg canpassexit


	if (exitList.size() == 1) {
		const Room::ExitOption& exit = exitList[0];

		if (!canPassExit(exit)) {           // check for locked doors before going
			return;
		}

		player->setCurrentRoom(exitList[0].room);
		writeMessage(player->getCurrentRoom()->describeSelf());
		return;
	}

	// Set up choices vector for promptChoice
	std::vector<Choice> choices;
	choices.reserve(exitList.size());

	for (auto& exit : exitList) {
		Room* targetRoom = exit.room;
		std::string doorId = exit.doorId;
		std::string label = exit.label;

		choices.push_back(Choice{
			label,
			[this, targetRoom, doorId]() {
				if (!doorId.empty()) {
					auto& roomItems = player->getCurrentRoom()->getRoomItems();
					auto doorIt = roomItems.find(doorId);
					if (doorIt != roomItems.end() && doorIt->second->isLocked()) {
						writeMessage(MSG_LOCKED);
						return;
					}
				}

				player->setCurrentRoom(targetRoom);
				writeMessage(player->getCurrentRoom()->describeSelf());
			}

		});
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
			itemPtr->setLocked(false);
			std::string msg = itemPtr->isLocked() ? "The {object1} is locked." : "The {object1} is unlocked.";
			writeMessage(msg, itemPtr->getName());
			return;
		}
		else {
			writeMessage(MSG_DONT_KNOW_HOW);
			return;
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
					itemPtr->setLocked(false);
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
