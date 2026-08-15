#pragma once

#include <array>
#include <string>

struct CapeCatalogEntry {
	const char* id;
	const char* name;
	const char* texture_hash;
};

const std::array<CapeCatalogEntry, 46>& OfficialCapeCatalog();
const CapeCatalogEntry* FindOfficialCape(const std::string& id);
std::string OfficialCapeResourceKey(const CapeCatalogEntry& cape);
