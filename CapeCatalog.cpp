#include "CapeCatalog.hpp"

#include <algorithm>

const std::array<CapeCatalogEntry, 46>& OfficialCapeCatalog() {
	static const std::array<CapeCatalogEntry, 46> capes = {{
		{"migrator", "Migrator", "2340c0e03dd24a11b15a8b33c2a7e9e32abb2051b2481d0ba7defd635ca7a933"},
		{"pan", "Pan", "28de4a81688ad18b49e735a273e086c18f1e3966956123ccb574034c06f5d336"},
		{"15th-anniversary", "15th Anniversary", "cd9d82ab17fd92022dbd4a86cde4c382a7540e117fae7b9a2853658505a80625"},
		{"common", "Common", "5ec930cdd2629c8771655c60eebeb867b4b6559b0e6d3bc71c40c96347fa03f0"},
		{"vanilla", "Vanilla", "f9a76537647989f9a0b6d001e320dac591c359e9e61a31f4ce11c88f207f0ad4"},
		{"cherry", "Cherry Blossom", "afd553b39358a24edfe3b8a9a939fa5fa4faa4d9a9c3d6af8eafb377fa05c2bb"},
		{"purple-heart", "Purple Heart", "cb40a92e32b57fd732a00fc325e7afb00a7ca74936ad50d8e860152e482cfbde"},
		{"followers", "Follower's", "569b7f2a1d00d26f30efe3f9ab9ac817b1e6d35f4f3cfb0324ef2d328223d350"},
		{"menace", "Menace", "dbc21e222528e30dc88445314f7be6ff12d3aeebc3c192054fba7e3b3f8c77b1"},
		{"copper", "Copper", "5e6f3193e74cd16cdd6637d9bae5484e3a37ff2a14c2d157c659a07810b1bdca"},
		{"home", "Home", "1de21419009db483900da6298a1e6cbf9f1bc1523a0dcdc16263fab150693edd"},
		{"mojang-office", "Mojang Office", "5c29410057e32abec02d870ecb52ec25fb45ea81e785a7854ae8429d7236ca26"},
		{"yearn", "Yearn", "308b32a9e303155a0b4262f9e5483ad4a22e3412e84fe8385a0bdd73dc41fa89"},
		{"founders", "Founder's", "99aba02ef05ec6aa4d42db8ee43796d6cd50e4b2954ab29f0caeb85f96bf52a1"},
		{"builder", "Builder", "2c579968c64c1719740fd8c2a451461879b238002574fce48f7d1a7c36a1c7d4"},
		{"zombie-horse", "Zombie Horse", "a3f6e4f14801f3ea55e3d95b9b4ef3b5e8802d947f669de93d6ec4b9354a436b"},
		{"mcc", "MCC 15th Year", "56c35628fe1c4d59dd52561a3d03bfa4e1a76d397c8b9c476c2f77cb6aebb1df"},
		{"minecraft-experience", "Minecraft Experience", "7658c5025c77cfac7574aab3af94a46a8886e3b7722a895255fbf22ab8652434"},
		{"minecon-2016", "MineCon 2016", "e7dfea16dc83c97df01a12fabbd1216359c0cd0ea42f9999b6e97c584963e980"},
		{"moonlight-trail", "Moonlight Trail Cape", "fe8a02dfe9e390e44ff33d69feef9d3943f76d3901015bbd50f0b67722d288bd"},
		{"minecon-2015", "MineCon 2015", "b0cc08840700447322d953a02b965f1d65a13a603bf64b17c803c21446fe1635"},
		{"minecon-2013", "MineCon 2013", "153b1a0dfcbae953cdeb6f2c2bf6bf79943239b1372780da44bcbb29273131da"},
		{"crafter", "Crafter", "479eacefa3cdd7aca94207f36c0dd449653ddf259daf40544a5866baf05eee22"},
		{"minecon-2012", "MineCon 2012", "a2e8d97ec79100e90a75d369d1b3ba81273c4f82bc1b737e934eed4a854be1b6"},
		{"minecon-2011", "MineCon 2011", "953cac8b779fe41383e675ee2b86071a71658f2180f56fbce8aa315ea70e2ed6"},
		{"realms-mapmaker", "Realms Mapmaker", "17912790ff164b93196f08ba71d0e62129304776d0f347334f8a6eae509f8a56"},
		{"mojang", "Mojang", "5786fe99be377dfb6858859f926c4dbc995751e91cee373468c5fbf4865e7151"},
		{"mojang-studios", "Mojang Studios", "9e507afc56359978a3eb3e32367042b853cddd0995d17d0da995662913fb00f7"},
		{"translator", "Translator", "1bf91499701404e21bd46b0191d63239a4ef76ebde88d27e4d430ac211df681e"},
		{"mojira-moderator", "Mojira Moderator", "ae677f7d98ac70a533713518416df4452fe5700365c09cf45d0d156ea9396551"},
		{"mojang-classic", "Mojang (Classic)", "8f120319222a9f4a104e2f5cb97b2cda93199a2ee9e1585cb8d09d6f687cb761"},
		{"cobalt", "Cobalt", "ca35c56efe71ed290385f4ab5346a1826b546a54d519e6a3ff01efa01acce81"},
		{"scrolls", "Scrolls", "3efadf6510961830f9fcc077f19b4daf286d502b5f5aafbd807c7bbffcaca245"},
		{"translator-chinese", "Translator (Chinese)", "2262fb1d24912209490586ecae98aca8500df3eff91f2a07da37ee524e7e3cb6"},
		{"turtle", "Turtle", "5048ea61566353397247d2b7d946034de926b997d5e66c86483dfb1e031aee95"},
		{"test", "Test", "7a93b1867eb599f2b76e6e1c30a0ddb530e6f4c7bce6515d1ba72b206df30e39"},
		{"valentine", "Valentine", "e578ef995fabcf0a94768f9651ac3aaba30c59ef85d2438e9b3e0cc1d810652b"},
		{"birthday", "Birthday", "2056f2eebd759cce93460907186ef44e9192954ae12b227d817eb4b55627a7fc"},
		{"db", "dB", "bcfbe84c6542a4a5c213c1cacf8979b5e913dcb4ad783a8b80e3c4a7d5c8bdac"},
		{"millionth-customer", "Millionth Customer", "70efffaf86fe5bc089608d3cb297d3e276b9eb7a8f9f2fe6659c23a2d8b18edf"},
		{"oxeye", "Oxeye", "7706b5f5fc90329691e59277dcc66ba20572219fa8e5da472afd5235fad12cc8"},
		{"prismarine", "Prismarine", "d8f8d13a1adf9636a16c31d47f3ecc9bb8d8533108aa5ad2a01b13b1a0c55eac"},
		{"snowman", "Snowman", "23ec737f18bfe4b547c95935fc297dd767bb84ee55bfd855144d279ac9bfd9fe"},
		{"spade", "Spade", "2e002d5e1758e79ba51d08d92a0f3a95119f2f435ae7704916507b6c565a7da8"},
		{"translator-japanese", "Translator (Japanese)", "ca29f5dd9e94fb1748203b92e36b66fda80750c87ebc18d6eafdb0e28cc1d05f"},
		{"blueprint", "Blueprint", "fdcf48f01ec480d1d7cbec27f7ddce48c9da2be6724641109444dae58d4cd013"},
	}};
	return capes;
}

const CapeCatalogEntry* FindOfficialCape(const std::string& id) {
	const auto& capes = OfficialCapeCatalog();
	const auto match = std::find_if(capes.begin(), capes.end(), [&](const CapeCatalogEntry& cape) {
		return id == cape.id;
	});
	return match == capes.end() ? nullptr : &*match;
}

std::string OfficialCapeResourceKey(const CapeCatalogEntry& cape) {
	return "Resources/capes/" + std::string(cape.id) + ".png";
}
