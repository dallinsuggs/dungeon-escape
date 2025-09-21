#include "CommandParser.hpp"
#include <unordered_map>
#include <cassert>


int main() {

	// Declare and initialize inventory and room object maps for testing purposes
	std::unordered_map<std::string, std::string> inventory;
	std::unordered_map<std::string, std::string> roomObjects;

	inventory["key"] = "key";
	roomObjects["door"] = "door";

	// Door open bool to test parsing logic outcomes
	bool testSuccess = false;

	
	// Initialize parser
	CommandParser myParser;

	// Define test user input
	std::string input = "use key on door";

	// Attempt to parse the input
	myParser.parse(input, inventory, roomObjects, testSuccess);

	// Check for desired outcome
	assert(testSuccess);

	// Try new input          1
	input = "use key";

	myParser.parse(input, inventory, roomObjects, testSuccess);

	assert(testSuccess);

	// Try new input           3
	input = "get use door";

	myParser.parse(input, inventory, roomObjects, testSuccess);

	assert(testSuccess);

}