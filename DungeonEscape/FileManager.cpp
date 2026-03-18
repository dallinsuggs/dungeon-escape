#define _CRT_SECURE_NO_WARNINGS
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
	std::cout << "Trying to load items.json...\n"; // debugging
	std::unordered_map<std::string, Item> items;

	FILE* fp = nullptr;
	fp = fopen(filename.c_str(), "r");
	if ( !fp) {
		std::cout << "File not found: items.json\n" << filename << "!\n";
		return items;  // early return on file error
	}
	std::cout << "items.json opened successfully.\n";

	char readBuffer[65536];
	rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));
	rapidjson::Document doc;
	doc.ParseStream(is);
	fclose(fp);

	if (doc.HasParseError()) {
		printf("JSON parse error in %s at offset %zu: %s\n",
			filename.c_str(),
			doc.GetErrorOffset(),
			rapidjson::GetParseError_En(doc.GetParseError()));
		return items;  // ← NEW: stop here if parsing failed
	}

	if (!doc.IsObject()) {
		std::cerr << "JSON root is not an object in file: " << filename << std::endl;
		return items;  // ← NEW: extra safety, same as loadRooms
	}

	for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); ++i) {
		std::string templateId = i->name.GetString();
		const rapidjson::Value& itemObj = i->value;

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

		Item item(name, description, moveable, locked);
		items[templateId] = item;
	}

	return items;
}

// Helper function to get an Item* by ID from the itemInstances map (returns nullptr if not found)
Item* FileManager::getItem(const std::string& id) {
	auto it = itemInstances.find(id);
	if (it != itemInstances.end()) {
		return it->second.get();
	}
	return nullptr; // not found
}


std::unordered_map<std::string, Room> FileManager::loadRooms(const std::string& filename, std::unordered_map<std::string, Item>& allItems)
{
	std::cout << "Trying to load rooms.json...\n";
	std::unordered_map<std::string, Room> rooms;
	itemInstances.clear();


	FILE* fp = nullptr;
	fp = fopen(filename.c_str(), "r");
	if (!fp) {
		std::cout << "File not found!";
		return rooms;
	}
	std::cout << "rooms.json opened successfully.\n";

	char readBuffer[65536]; // 64 KB buffer
	rapidjson::FileReadStream is(fp, readBuffer, sizeof(readBuffer));

	rapidjson::Document doc;
	doc.ParseStream(is);
	fclose(fp); // close file once parsed

	if (doc.HasParseError()) {
		std::cout << "Parse failed in rooms.json\n";
		printf("JSON parse error: %s\n", rapidjson::GetParseError_En(doc.GetParseError()));
		// handle error
	}

	if (!doc.IsObject()) {
		std::cout << "Root not object in rooms.json\n";
		std::cerr << "JSON root is not an object in file: " << filename << std::endl;
		return rooms;
	}

	// --- PASS 1: Create all rooms with everything but exits ----------------------------------------

	for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); ++i) {
		std::string roomId = i->name.GetString(); // "cell_1"
		if (!i->value.IsObject()) {
			std::cerr << "Room entry " << i->name.GetString() << " is not an object!\n";
			continue;
		}
		const auto& roomObj = i->value; // The object with fields

		// Read fields
		std::string name, description;
		bool moveable;

		if (roomObj.HasMember("name") && roomObj["name"].IsString()) {
			name = roomObj["name"].GetString();
		}
		if (roomObj.HasMember("description") && roomObj["description"].IsString()) {
			description = roomObj["description"].GetString();
		}
		// code to read roomItems
		std::unordered_map<std::string, Item*> roomItems;

		if (roomObj.HasMember("items") && roomObj["items"].IsArray()) {
			const auto& itemsArray = roomObj["items"];
			for (auto& itemVal : itemsArray.GetArray()) {
				if (itemVal.HasMember("template") && itemVal["template"].IsString() &&
					itemVal.HasMember("id") && itemVal["id"].IsString()) {
					
					std::string itemTemplateId = itemVal["template"].GetString();
					std::string itemId = itemVal["id"].GetString();

					// Look up the Item* from pre-loaded allItems map
					// 1) If instance already exists for this itemId, reuse it
					auto instIt = itemInstances.find(itemId);
					if (instIt != itemInstances.end()) {
						roomItems[itemId] = instIt->second.get();
						continue;
					}

					// 2) Otherwise create a new instance from the template
					auto templIt = allItems.find(itemTemplateId);
					if (templIt == allItems.end()) {
						std::cout << "Unknown item template: " << itemTemplateId << "\n";
						continue;
					}

					// Copy the template into a new instance.
					// This requires Item to be copyable (default is usually fine unless you added raw owning pointers).
					itemInstances[itemId] = std::make_unique<Item>(templIt->second);

					// Optional: allow per-instance overrides from rooms.json (locked/moveable/etc)
					if (itemVal.HasMember("locked") && itemVal["locked"].IsBool()) {
						itemInstances[itemId]->setLocked(itemVal["locked"].GetBool()); // if you have setter
					}
					if (itemVal.HasMember("moveable") && itemVal["moveable"].IsBool()) {
						itemInstances[itemId]->setMoveable(itemVal["moveable"].GetBool());
					}

					// 3) Store pointer in this room
					roomItems[itemId] = itemInstances[itemId].get();

				}
			}
		}

		// Create Item using item template from json file
		Room room(name, description, roomItems);
		room.setId(roomId);

		// Insert into map
		rooms.emplace(roomId, std::move(room));
		
	}

	// --- PASS 2: Populate exits ----------------------------------------------------------------

	for (auto i = doc.MemberBegin(); i != doc.MemberEnd(); ++i) {
		std::string roomId = i->name.GetString();
		const auto& roomObj = i->value;

		std::cout << "Looking up room: " << roomId << std::endl;
		if (!rooms.count(roomId)) {
			std::cout << "Room not found in map!" << std::endl;
			continue;
		}


		Room& room = rooms.at(roomId); // Reference so we can update actual room

		if (roomObj.HasMember("exits") && roomObj["exits"].IsObject()) {
			const auto& exitsObj = roomObj["exits"];
			for (auto exitIt = exitsObj.MemberBegin(); exitIt != exitsObj.MemberEnd(); ++exitIt) {
				std::string direction = exitIt->name.GetString();
				const auto& exitArray = exitIt->value;

				if (exitArray.IsArray()) {
					for (auto& exitVal : exitArray.GetArray()) {

						if (!exitVal.HasMember("id") || !exitVal["id"].IsString()) continue;
						std::string targetId = exitVal["id"].GetString();

						std::string label = (exitVal.HasMember("label") && exitVal["label"].IsString())
							? exitVal["label"].GetString()
							: "";

						std::string doorId = (exitVal.HasMember("doorId") && exitVal["doorId"].IsString())
							? exitVal["doorId"].GetString()
							: "";

						Room* targetRoom = rooms.count(targetId) ? &rooms.at(targetId) : nullptr;
						if (targetRoom) {
							room.connectRoom(direction, targetRoom, label, doorId);
						}
					}

				}
			}
		}
	}
	return rooms;
}
