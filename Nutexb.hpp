#pragma once

#include <iostream>
#include <filesystem>
#include <vector>
#include <opencv2/opencv.hpp>

enum class NUTEXBFormat : uint8_t {
    R8G8B8A8_UNORM = 0,
    R8G8B8A8_SRGB = 5,
    R32G32B32A32_FLOAT = 52,
    B8G8R8A8_UNORM = 80,
    B8G8R8A8_SRGB = 85,
    BC1_UNORM = 128,
    BC1_SRGB = 133,
    BC2_UNORM = 144,
    BC2_SRGB = 149,
    BC3_UNORM = 160,
    BC3_SRGB = 165,
    BC4_UNORM = 176,
    BC4_SNORM = 181,
    BC5_UNORM = 192,
    BC5_SNORM = 197,
    BC6_UFLOAT = 215,
    BC7_UNORM = 224,
    BC7_SRGB = 229,
};

#pragma pack(push, 1)

struct NUTEXBFooter {
    uint32_t mip_sizes[16];
    const char XNT[4] = { ' ', 'X', 'N', 'T' };
    char internal_name[0x40];
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    NUTEXBFormat format;
    uint8_t unk;
    uint16_t PADDING;
    uint32_t unk2;
    uint32_t mip_count;
    uint32_t alignment;
    uint32_t array_count;
    uint32_t size;
    const char XET[4] = { ' ', 'X', 'E', 'T' };
    uint16_t major_version;
    uint16_t minor_version;

    NUTEXBFooter();
};

#pragma pack(pop)

class NUTEXB {
private:
    std::vector<uint8_t> IMAGE_DATA;
    NUTEXBFooter footer;
public:

    NUTEXB(const std::string& internal_name, void* data, size_t size);

    NUTEXB(const std::string& internal_name, cv::Mat& mat, NUTEXBFormat format = NUTEXBFormat::B8G8R8A8_SRGB);

    NUTEXBFooter& GetFooter();

    bool Open(const std::filesystem::path& filepath);

    bool Save(const std::filesystem::path& filepath, unsigned int pad = 0);

    bool Save(std::ostream& NUT_OUT, unsigned int pad = 0);
};