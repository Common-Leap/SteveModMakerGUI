#pragma once

#include <string>
#include <vector>

struct CapeCatalogEntry {
	const char* id;
	const char* name;
	const char* texture_hash;
};

const std::vector<CapeCatalogEntry>& OfficialCapeCatalog();
const CapeCatalogEntry* FindOfficialCape(const std::string& id);
std::string OfficialCapeResourceKey(const CapeCatalogEntry& cape);
