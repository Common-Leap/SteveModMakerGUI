
#include <fstream>
#include <algorithm>

#include "Nutexb.hpp"
#include "tegra_swizzle.hpp"

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

bool NUTEXB::ReplaceTextureFromMat(cv::Mat& mat) {
	if (mat.empty()) {
		return false;
	}

	if (mat.type() != CV_8UC4) {
		return false;
	}

	const uint32_t width = footer.width;
	const uint32_t height = footer.height;
	const uint32_t mip_count = footer.mip_count > 0 ? footer.mip_count : 1;

	if (width == 0 || height == 0) {
		return false;
	}

	cv::Mat base_level;
	if ((uint32_t)mat.cols == width && (uint32_t)mat.rows == height) {
		base_level = mat;
	}
	else {
		cv::resize(mat, base_level, cv::Size((int)width, (int)height), 0, 0, cv::INTER_AREA);
	}

	if (!base_level.isContinuous()) {
		base_level = base_level.clone();
	}

	const size_t block_height_0 = block_height_mip0(height);
	std::vector<uint8_t> new_image_data;

	for (uint32_t mip = 0; mip < mip_count && mip < 16; mip++) {
		const uint32_t mip_width = std::max(1u, width >> mip);
		const uint32_t mip_height = std::max(1u, height >> mip);
		const size_t mip_block = mip_block_height(mip_height, block_height_0);
		const size_t swizzled_size = swizzled_mip_size(mip_width, mip_height, 1, mip_block, 4);

		cv::Mat mip_level;
		if (mip == 0) {
			mip_level = base_level;
		}
		else {
			cv::resize(base_level, mip_level, cv::Size((int)mip_width, (int)mip_height), 0, 0, cv::INTER_AREA);
		}

		if (!mip_level.isContinuous()) {
			mip_level = mip_level.clone();
		}

		const size_t input_size = (size_t)mip_width * (size_t)mip_height * 4;
		std::vector<uint8_t> swizzled(swizzled_size);
		swizzle_block_linear(
			mip_width,
			mip_height,
			1,
			mip_level.data,
			input_size,
			swizzled.data(),
			swizzled_size,
			mip_block,
			4
		);

		footer.mip_sizes[mip] = (uint32_t)swizzled_size;
		new_image_data.insert(new_image_data.end(), swizzled.begin(), swizzled.end());
	}

	for (uint32_t mip = mip_count; mip < 16; mip++) {
		footer.mip_sizes[mip] = 0;
	}

	IMAGE_DATA = std::move(new_image_data);
	footer.size = (uint32_t)IMAGE_DATA.size();
	return true;
}

NUTEXB::NUTEXB(const std::string& internal_name, void* data, size_t size) {
	IMAGE_DATA.resize(size);
	memcpy(&IMAGE_DATA[0], data, size);
	footer.size = size;
	strncpy(footer.internal_name, internal_name.c_str(), 0x40 - 1);
	footer.internal_name[0x40 - 1] = '\0';
}

NUTEXB::NUTEXB(const std::string& internal_name, cv::Mat& mat, NUTEXBFormat format) {

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
	footer.major_version = 1;
	footer.minor_version = 2;

	ReplaceTextureFromMat(mat);
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
