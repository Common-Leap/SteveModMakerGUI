#pragma once

#include <iostream>
#include <opencv2/opencv.hpp>

constexpr uint32_t HEADER_SIZE = 32 + 40;
constexpr uint32_t MEM_POOL_SIZE = 0x150;
constexpr uint32_t DATA_PTR_SIZE = 0x8;

constexpr uint32_t START_OF_STR_SECTION = HEADER_SIZE + MEM_POOL_SIZE + DATA_PTR_SIZE;

constexpr uint32_t STR_HEADER_SIZE = 0x14;
constexpr uint32_t EMPTY_STR_SIZE = 0x4;

constexpr uint32_t FILENAME_STR_OFFSET = START_OF_STR_SECTION + STR_HEADER_SIZE + EMPTY_STR_SIZE;

constexpr uint32_t BRTD_SECTION_START = 0xFF0;
constexpr uint32_t START_OF_TEXTURE_DATA = BRTD_SECTION_START + 16;

constexpr char DIC_SECTION[] = "\x5F\x44\x49\x43\x01\x00\x00\x00\xFF\xFF\xFF\xFF\x01\x00\x00\x00\xB4\x01\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x00\x00\x01\x00\xB8\x01\x00\x00\x00\x00\x00\x00";

#pragma pack(push, 1)

// Can be memory-mapped.
struct BRTISection {
    char Magic[4] = { 'B', 'R', 'T', 'I' };
    uint32_t Size;
    uint64_t Size2;
    uint8_t  Flags;
    uint8_t  Dim;
    uint16_t TileMode;
    uint16_t Swizzle;
    uint16_t MipCount;
    uint32_t NumMultiSample;
    uint32_t Format;
    uint32_t unk2;
    uint32_t Width;
    uint32_t Height;
    uint32_t Depth;
    uint32_t ArrayLength;
    int32_t  SizeRange;
    uint32_t unk4[6];
    uint32_t ImageSize;
    uint32_t Align;
    uint32_t CompSel;
    uint32_t TY;

    uint64_t NameOffset;
    uint64_t ParentOffset;

    uint64_t UnkOffset;
    uint64_t unk5;
    uint64_t EndOffset;
    uint64_t UnkOffset2;
    uint64_t unk6;
    uint64_t unk7;
};

// Can be memory-mapped.
struct BRTDSection {
    char Magic[4] = { 'B', 'R', 'T', 'D' };
    uint32_t Padding;
    uint64_t ImageSize;
};

// Can be memory-mapped.
struct BNTXHeader {
    char Magic[4] = { 'B', 'N', 'T', 'X' };
    uint32_t Padding;
    uint16_t Version[2];
    uint16_t BOM;
    uint16_t Revision;
    uint32_t FileNameOffset;
    uint16_t Padding2;
    uint16_t STROffset;
    uint32_t RLTOffset;
    uint32_t FileSize;
};

// Can be memory-mapped.
struct NXHeader {
    char Magic[4] = { 'N', 'X', ' ', ' ' };
    uint32_t Count;
    uint64_t BRTIFarOffset;
    uint64_t BRTDOffset;
    uint64_t DICOffset;
    uint64_t DICSize;
};

// Can be memory-mapped.
struct RLTSection {
    uint64_t Offset;
    uint32_t Position;
    uint32_t Size;
    uint32_t Index;
    uint32_t Count;
};

// Can be memory-mapped.
struct RLTEntry {
    uint32_t Position;
    uint16_t StructCount;
    uint8_t  OffsetCount;
    uint8_t  PaddingCount;
};

#pragma pack(pop)

// CANNOT be memory-mapped.
struct RLTTable {
    char Magic[4] = { '_', 'R', 'L', 'T' };
    uint32_t RLTSectionPosition;
    uint32_t RLTSectionCount;
    std::vector<RLTSection> Sections;
    std::vector<RLTEntry> Entries;

    void Parse(std::istream& stream);

    void Write(std::ostream& stream);

    static RLTTable FromSectionSizes(const uint32_t ImageSize, const uint32_t str_section_size, const uint32_t dict_section_size);

    uint64_t GetSize();
};

// Can be memory-mapped, but likely to change in the future.
struct DICSection {
    char Magic[4] = { '_', 'D', 'I', 'C' };
    uint32_t unk;
    int32_t  unk2;
    uint32_t unk3;
    uint64_t unk4;
    uint32_t unk5;
    uint32_t unk6;
    uint64_t unk7;

    static DICSection Default();

    void Parse(std::istream& stream);

    void Write(std::ostream& stream);

    uint64_t GetSize();
};

// CANNOT be memory-mapped.
struct STRSection {
    char Magic[4] = { '_', 'S', 'T', 'R' };
    uint32_t SizeOfSTRAndDIC;
    uint32_t SizeOfSTRAndDIC2;
    uint32_t unk3;
    uint32_t StringCount;
    uint32_t Reserved; // TODO: What does this actually do though?
    std::vector<std::string> Strings;

    static STRSection FromStringAndDICSection(const std::string& input, DICSection& dic_section);

    void Parse(std::istream& stream);

    void Write(std::ostream& stream);

    uint64_t GetSize();
};

class BNTX {
private:
    BNTXHeader header;
    NXHeader nx_header;
    char mem_pool[0x150] = { 0 };
    uint64_t BRTIOffset;

    // At this point the file needs to be manually parsed.
    STRSection str_section;

    DICSection dic_section;
    BRTISection brti_section;
    BRTDSection brtd_section;

    std::vector<uint8_t> IMAGE_DATA;

    RLTTable relocation_table;
public:
    BNTX(cv::Mat& mat, const std::string& name);

    bool Write(const std::string& path);
};
