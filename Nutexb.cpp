
#include <fstream>
#include <algorithm>
#include <cstring>

#include "Nutexb.hpp"
#include "tegra_swizzle.hpp"

namespace {

constexpr uint32_t kBlockLinearAlignment = 0x1000;

void InitializeFooterMagic(NUTEXBFooter& footer) {
	std::memcpy(footer.XNT, " XNT", 4);
	std::memcpy(footer.XET, " XET", 4);
}

} // namespace

bool NUTEXB::Open(const std::filesystem::path& filepath) {
	std::ifstream FSTREAM(filepath, std::ios::in | std::ios::binary);
	if (!FSTREAM) {
		return false;
	}
	FSTREAM.seekg(0, std::ios_base::end);
	const std::streamoff file_size = FSTREAM.tellg();
	if (file_size < static_cast<std::streamoff>(sizeof(footer))) {
		return false;
	}
	const std::streamoff footer_offset = file_size - static_cast<std::streamoff>(sizeof(footer));
	FSTREAM.seekg(footer_offset);
	FSTREAM.read((char*)&footer, sizeof(footer));
	if (!FSTREAM || footer.size == 0 || footer.size > static_cast<uint64_t>(footer_offset)) {
		return false;
	}
	FSTREAM.seekg(0);
	IMAGE_DATA.resize(footer.size);
	FSTREAM.read(reinterpret_cast<char*>(IMAGE_DATA.data()), static_cast<std::streamsize>(footer.size));
	FSTREAM.close();
	return static_cast<bool>(FSTREAM);
};

bool NUTEXB::Open(const uint8_t* data, size_t size) {
	if (data == nullptr || size < sizeof(NUTEXBFooter)) {
		return false;
	}

	const size_t footer_offset = size - sizeof(NUTEXBFooter);
	memcpy(&footer, data + footer_offset, sizeof(NUTEXBFooter));
	if (footer.size == 0 || footer.size > footer_offset || footer.size > size - sizeof(NUTEXBFooter)) {
		return false;
	}

	IMAGE_DATA.assign(data, data + footer.size);
	return true;
}

bool NUTEXB::ReplaceTextureFromMat(cv::Mat& mat) {
	if (mat.empty()) {
		return false;
	}

	if (mat.type() != CV_8UC4) {
		return false;
	}

	const bool rgba_format = footer.format == NUTEXBFormat::R8G8B8A8_UNORM
		|| footer.format == NUTEXBFormat::R8G8B8A8_SRGB;
	const bool bgra_format = footer.format == NUTEXBFormat::B8G8R8A8_UNORM
		|| footer.format == NUTEXBFormat::B8G8R8A8_SRGB;
	if (!rgba_format && !bgra_format) {
		// Raw four-channel pixels cannot replace a block-compressed texture while
		// retaining its footer. Callers should generate a new uncompressed NUTEXB.
		return false;
	}

	const uint32_t width = footer.width;
	const uint32_t height = footer.height;
	const uint32_t mip_count = std::min(std::max(footer.mip_count, 1u), 16u);
	footer.mip_count = mip_count;

	if (width == 0 || height == 0) {
		return false;
	}

	// OpenCV stores 4-channel images as BGRA. Convert to the format expected by the NUTEXB footer.
	cv::Mat source;
	if (rgba_format) {
		cv::cvtColor(mat, source, cv::COLOR_BGRA2RGBA);
	}
	else {
		source = mat;
	}

	cv::Mat base_level;
	if ((uint32_t)source.cols == width && (uint32_t)source.rows == height) {
		base_level = source;
	}
	else {
		cv::resize(source, base_level, cv::Size((int)width, (int)height), 0, 0, cv::INTER_AREA);
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

		// NUTEXB stores the linear byte count for each mip here. The tiled
		// payload is larger because each block-linear GOB is padded, but writing
		// that padded count makes the runtime advance to the wrong mip offsets.
		footer.mip_sizes[mip] = (uint32_t)input_size;
		new_image_data.insert(new_image_data.end(), swizzled.begin(), swizzled.end());
	}

	for (uint32_t mip = mip_count; mip < 16; mip++) {
		footer.mip_sizes[mip] = 0;
	}

	IMAGE_DATA = std::move(new_image_data);
	footer.size = (uint32_t)IMAGE_DATA.size();
	footer.alignment = kBlockLinearAlignment;
	return true;
}

NUTEXB::NUTEXB(const std::string& internal_name, void* data, size_t size) {
	InitializeFooterMagic(footer);
	IMAGE_DATA.resize(size);
	if (size > 0 && data != nullptr) {
		memcpy(IMAGE_DATA.data(), data, size);
	}
	else if (size > 0) {
		IMAGE_DATA.clear();
		size = 0;
	}
	footer.size = size;
	strncpy(footer.internal_name, internal_name.c_str(), 0x40 - 1);
	footer.internal_name[0x40 - 1] = '\0';
}

NUTEXB::NUTEXB(const std::string& internal_name, cv::Mat& mat, NUTEXBFormat format) {
	InitializeFooterMagic(footer);

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
	// 0x1000 identifies a Tegra block-linear surface. A zero value describes
	// an unswizzled linear payload, which makes the game interpret the GOB-
	// tiled bytes as ordinary scanlines.
	footer.alignment = kBlockLinearAlignment;
	footer.array_count = 1;
	footer.major_version = 1;
	footer.minor_version = 2;

	ReplaceTextureFromMat(mat);
}

NUTEXBFooter& NUTEXB::GetFooter() { return footer; }

bool NUTEXB::Save(const std::filesystem::path& filepath, unsigned int pad) {
	if (footer.size == 0 || IMAGE_DATA.size() < footer.size) {
		return false;
	}
	std::ofstream NUT_OUT(filepath, std::ios::out | std::ios::binary);
	if (!NUT_OUT) {
		return false;
	}
	NUT_OUT.write(reinterpret_cast<const char*>(IMAGE_DATA.data()), static_cast<std::streamsize>(footer.size));

	char null = 0;
	for (unsigned int x = 0; x < pad; x++)
		NUT_OUT.write(&null, 1);

	NUT_OUT.write(reinterpret_cast<char*>(&footer), sizeof(footer));
	NUT_OUT.close();
	return static_cast<bool>(NUT_OUT);
}

bool NUTEXB::Save(std::ostream& NUT_OUT, unsigned int pad) {
	if (!NUT_OUT || footer.size == 0 || IMAGE_DATA.size() < footer.size) {
		return false;
	}
	NUT_OUT.write(reinterpret_cast<const char*>(IMAGE_DATA.data()), static_cast<std::streamsize>(footer.size));

	char null = 0;
	for (unsigned int x = 0; x < pad; x++)
		NUT_OUT.write(&null, 1);

	NUT_OUT.write(reinterpret_cast<char*>(&footer), sizeof(footer));
	return static_cast<bool>(NUT_OUT);
}
