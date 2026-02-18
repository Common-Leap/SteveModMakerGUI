
#include <fstream>

#include "Nutexb.hpp"

NUTEXBFooter::NUTEXBFooter() {
	memset(&this->mip_sizes, 0, 64);
	memset(&this->internal_name, 0, 64);
	memset(&this->major_version, 0, 4);
	// zero-initialize everything
}

bool NUTEXB::Open(const std::filesystem::path& filepath) {
	std::ifstream FSTREAM(filepath, std::ios::in | std::ios::binary);
	if (!FSTREAM) {
		return false;
	}
	FSTREAM.seekg(-0xB0, std::ios_base::end);
	FSTREAM.read((char*)&footer, sizeof(footer));
	FSTREAM.seekg(0);
	IMAGE_DATA.resize(footer.size);
	FSTREAM.read((char*)&IMAGE_DATA[0], footer.size);
	FSTREAM.close();
	return true;
};

NUTEXB::NUTEXB(const std::string& internal_name, void* data, size_t size) {
	IMAGE_DATA.resize(size);
	memcpy(&IMAGE_DATA[0], data, size);
	footer.size = size;
	strncpy(footer.internal_name, internal_name.c_str(), 0x40 - 1);
	footer.internal_name[0x40 - 1] = '\0';
}

NUTEXB::NUTEXB(const std::string& internal_name, cv::Mat& mat, NUTEXBFormat format) {

	IMAGE_DATA.resize(mat.cols * mat.rows * 4);
	memcpy(&IMAGE_DATA[0], mat.data, mat.cols * mat.rows * 4);

	footer.mip_sizes[0] = mat.cols * mat.rows * 4;

	strncpy(footer.internal_name, internal_name.c_str(), 0x40 - 1);
	footer.internal_name[0x40 - 1] = '\0';

	footer.width = mat.cols;
	footer.height = mat.rows;
	footer.depth = 1;
	footer.format = format;
	footer.unk = 4;
	footer.PADDING = 0;
	footer.unk2 = 4;
	footer.mip_count = 7;
	footer.alignment = 0;
	footer.array_count = 1;
	footer.size = mat.cols * mat.rows * 4;
	footer.major_version = 1;
	footer.minor_version = 2;
}

NUTEXBFooter& NUTEXB::GetFooter() { return footer; }

bool NUTEXB::Save(const std::filesystem::path& filepath, unsigned int pad) {
	std::ofstream NUT_OUT(filepath, std::ios::out | std::ios::binary);
	if (!NUT_OUT) {
		return false;
	}
	NUT_OUT.write((char*)&IMAGE_DATA[0], footer.size);

	char null = 0;
	for (unsigned int x = 0; x < pad; x++)
		NUT_OUT.write(&null, 1);

	NUT_OUT.write(reinterpret_cast<char*>(&footer), sizeof(footer));
	NUT_OUT.close();
	return true;
}

bool NUTEXB::Save(std::ostream& NUT_OUT, unsigned int pad) {
	if (!NUT_OUT) {
		return false;
	}
	NUT_OUT.write((char*)&IMAGE_DATA[0], footer.size);

	char null = 0;
	for (unsigned int x = 0; x < pad; x++)
		NUT_OUT.write(&null, 1);

	NUT_OUT.write(reinterpret_cast<char*>(&footer), sizeof(footer));
	return true;
}
