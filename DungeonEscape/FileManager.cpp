#include "FileManager.hpp"
#include "Item.hpp"
#include <unordered_map>

// For json parsing
#include "rapidjson/document.h"      // Core DOM parser
#include "rapidjson/filereadstream.h" // For reading JSON from a FILE*
#include "rapidjson/error/en.h"       // Optional: human-readable parse errors

FileManager::FileManager()
{
}

// Function that reads data from item file and returns unordered map with all the items
std::unordered_map<std::string, Item> FileManager::loadItems(const std::string& filename)
{
	std::unordered_map<std::string, Item> items;

	FILE* fp = nullptr;
	errno_t err = fopen_s(&fp, filename.c_str(), "r");
	if (err != 0 || !fp) {
		std::cout << "File not found!";
		return items;
	}

	char readBuffer[65536]; // 64 KB buffer
	rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

	rapidjson::Document doc;
	doc.ParseStream(is);
	fclose(fp); // close file once parsed

	if (doc.HasParseError()) {
		printf("JSON parse error: %s\n", rapidjson::GetParseError_En(doc.GetParseError()));
		// handle error
	}

	for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); ++i) {
		std::string templateId = i->name.GetString(); // "chamber pot"
		const rapidjson::Value& itemObj = i->value; // The object with fields

		// Read fields
		std::string name, description;
		bool moveable = true, locked = false;

		if (itemObj.HasMember("name") && itemObj["name"].IsString()) {
			name = itemObj["name"].GetString();
		}
		if (itemObj.HasMember("description") && itemObj["description"].IsString()) {
			description = itemObj["description"].GetString();
		}
		if (itemObj.HasMember("moveable") && itemObj["moveable"].IsBool()) {
			moveable = itemObj["moveable"].GetBool();
		}
		if (itemObj.HasMember("locked") && itemObj["locked"].IsBool()) {
			locked = itemObj["locked"].GetBool();
		}
		
		// Create Item using item template from json file
		Item item(name, description, moveable, locked);

		// Insert into map
		items[templateId] = item;

		
	}

	return items;
}

std::unordered_map<std::string, Room> FileManager::loadRooms(const std::string& filename, std::unordered_map<std::string, Item>& allItems)
{
	std::unordered_map<std::string, Room> rooms;

	FILE* fp = nullptr;
	errno_t err = fopen_s(&fp, filename.c_str(), "r");
	if (err != 0 || !fp) {
		std::cout << "File not found!";
		return rooms;
	}

	char readBuffer[65536]; // 64 KB buffer
	rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

	rapidjson::Document doc;
	doc.ParseStream(is);
	fclose(fp); // close file once parsed

	if (doc.HasParseError()) {
		printf("JSON parse error: %s\n", rapidjson::GetParseError_En(doc.GetParseError()));
		// handle error
	}

	if (!doc.IsObject()) {
		std::cerr << "JSON root is not an object in file: " << filename << std::endl;
		return rooms;
	}

	// --- PASS 1: Create all rooms with everything but exits ----------------------------------------

	for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); ++i) {
		std::string templateId = i->name.GetString(); // "cell_1"
		if (!i->value.IsObject()) {
			std::cerr << "Room entry " << i->name.GetString() << " is not an object!\n";
			continue;
		}
		const auto& itemObj = i->value; // The object with fields

		// Read fields
		std::string name, description;
		bool moveable;

		if (itemObj.HasMember("name") && itemObj["name"].IsString()) {
			name = itemObj["name"].GetString();
		}
		if (itemObj.HasMember("description") && itemObj["description"].IsString()) {
			description = itemObj["description"].GetString();
		}
		// code to read roomItems
		std::unordered_map<std::string, Item*> roomItems;

		if (itemObj.HasMember("items") && itemObj["items"].IsArray()) {
			const auto& itemsArray = itemObj["items"];
			for (auto& itemVal : itemsArray.GetArray()) {
				if (itemVal.HasMember("template") && itemVal["template"].IsString() &&
					itemVal.HasMember("id") && itemVal["id"].IsString()) {
					
					std::string itemTemplateId = itemVal["template"].GetString();
					std::string itemId = itemVal["id"].GetString();

					// Look up the Item* from pre-loaded allItems map
					auto it = allItems.find(itemTemplateId);
					if (it != allItems.end()) {
						roomItems[itemId] = std::addressof(it->second);
					}
				}
			}
		}

		// Create Item using item template from json file
		Room room(name, description, roomItems);

		// Insert into map
		rooms.emplace(templateId, std::move(room));
		
	}

	// --- PASS 2: Populate exits ----------------------------------------------------------------

	for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); ++i) {
		std::string roomId = i->name.GetString();
		const auto& roomObj = i->value;

		std::cout << "Looking up room: " << roomId << std::endl;
		if (!rooms.count(roomId)) {
			std::cout << "Room not found in map!" << std::endl;
		}


		Room& room = rooms.at(roomId); // Reference so we can update actual room

		if (roomObj.HasMember("exits") && roomObj["exits"].IsObject()) {
			const auto& exitsObj = roomObj["exits"];
			for (auto exitIt = exitsObj.MemberBegin(); exitIt != exitsObj.MemberEnd(); ++exitIt) {
				std::string direction = exitIt->name.GetString();
				const auto& exitArray = exitIt->value;

				if (exitArray.IsArray()) {
					for (auto& exitVal : exitArray.GetArray()) {
						std::string targetId = exitVal["id"].GetString();
						std::string label = exitVal.HasMember("label") ? exitVal["label"].GetString() : "";

						Room* targetRoom = rooms.count(targetId) ? &rooms.at(targetId) : nullptr;
						if (targetRoom) {
							room.connectRoom(direction, targetRoom, label);
						}
					}
				}
			}
		}
	}
	return rooms;
}
