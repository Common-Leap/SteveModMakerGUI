#include "tegra_swizzle.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace {

constexpr size_t kGobWidthInBytes = 64;
constexpr size_t kGobHeightInLines = 8;
constexpr size_t kGobSizeInBytes = 512;
constexpr size_t kMaxBlockHeight = 16;

size_t DivRoundUp(size_t x, size_t d) {
    return (x + d - 1) / d;
}

size_t RoundUp(size_t x, size_t n) {
    return DivRoundUp(x, n) * n;
}

size_t ClampBlockHeight(size_t block_height) {
    if (block_height == 0) {
        return 1;
    }

    size_t clamped = 1;
    while (clamped < block_height && clamped < kMaxBlockHeight) {
        clamped <<= 1;
    }
    return std::min(clamped, kMaxBlockHeight);
}

size_t BlockLinearAddress(
    size_t x,
    size_t y,
    size_t width,
    size_t block_height,
    size_t bytes_per_pixel
) {
    const size_t image_width_in_gobs = DivRoundUp(width * bytes_per_pixel, kGobWidthInBytes);
    const size_t x_bytes = x * bytes_per_pixel;

    const size_t gob_address =
        (y / (kGobHeightInLines * block_height)) * kGobSizeInBytes * block_height * image_width_in_gobs +
        (x_bytes / kGobWidthInBytes) * kGobSizeInBytes * block_height +
        ((y % (kGobHeightInLines * block_height)) / kGobHeightInLines) * kGobSizeInBytes;

    return gob_address +
        ((x_bytes % kGobWidthInBytes) / 32) * 256 +
        ((y % kGobHeightInLines) / 2) * 64 +
        ((x_bytes % 32) / 16) * 32 +
        (y % 2) * 16 +
        (x_bytes % 16);
}

} // namespace

extern "C" size_t swizzled_mip_size(
    size_t width,
    size_t height,
    size_t depth,
    size_t block_height,
    size_t bytes_per_pixel
) {
    if (width == 0 || height == 0 || depth == 0 || bytes_per_pixel == 0) {
        return 0;
    }

    const size_t effective_block_height = ClampBlockHeight(block_height);
    const size_t pitch = RoundUp(width * bytes_per_pixel, kGobWidthInBytes);
    const size_t height_in_lines = RoundUp(height, effective_block_height * kGobHeightInLines);
    return pitch * height_in_lines * depth;
}

extern "C" size_t deswizzled_mip_size(
    size_t width,
    size_t height,
    size_t depth,
    size_t bytes_per_pixel
) {
    if (width == 0 || height == 0 || depth == 0 || bytes_per_pixel == 0) {
        return 0;
    }
    return width * height * depth * bytes_per_pixel;
}

extern "C" size_t block_height_mip0(size_t height) {
    if (height == 0) {
        return 1;
    }

    const size_t block_lines = DivRoundUp(height, kGobHeightInLines);
    size_t block_height = 1;
    while (block_height < block_lines && block_height < kMaxBlockHeight) {
        block_height <<= 1;
    }
    return std::min(block_height, kMaxBlockHeight);
}

extern "C" size_t mip_block_height(size_t mip_height, size_t block_height_mip0_value) {
    size_t block_height = ClampBlockHeight(block_height_mip0_value);
    const size_t mip_lines = std::max<size_t>(1, DivRoundUp(mip_height, kGobHeightInLines));

    while (block_height > 1 && mip_lines <= (block_height / 2)) {
        block_height >>= 1;
    }

    return block_height;
}

extern "C" void swizzle_block_linear(
    size_t width,
    size_t height,
    size_t depth,
    unsigned char* source,
    size_t source_len,
    unsigned char* destination,
    size_t destination_len,
    size_t block_height,
    size_t bytes_per_pixel
) {
    if (source == nullptr || destination == nullptr || width == 0 || height == 0 || depth == 0 || bytes_per_pixel == 0) {
        return;
    }

    std::memset(destination, 0, destination_len);

    const size_t effective_block_height = ClampBlockHeight(block_height);
    const size_t swizzled_slice_size = swizzled_mip_size(width, height, 1, effective_block_height, bytes_per_pixel);

    for (size_t z = 0; z < depth; z++) {
        const size_t swizzled_slice_offset = z * swizzled_slice_size;
        for (size_t y = 0; y < height; y++) {
            for (size_t x = 0; x < width; x++) {
                const size_t linear_index = ((z * height + y) * width + x) * bytes_per_pixel;
                const size_t swizzled_index =
                    swizzled_slice_offset + BlockLinearAddress(x, y, width, effective_block_height, bytes_per_pixel);

                if (linear_index + bytes_per_pixel > source_len || swizzled_index + bytes_per_pixel > destination_len) {
                    continue;
                }

                std::memcpy(destination + swizzled_index, source + linear_index, bytes_per_pixel);
            }
        }
    }
}

extern "C" void deswizzle_block_linear(
    size_t width,
    size_t height,
    size_t depth,
    unsigned char* source,
    size_t source_len,
    unsigned char* destination,
    size_t destination_len,
    size_t block_height,
    size_t bytes_per_pixel
) {
    if (source == nullptr || destination == nullptr || width == 0 || height == 0 || depth == 0 || bytes_per_pixel == 0) {
        return;
    }

    std::memset(destination, 0, destination_len);

    const size_t effective_block_height = ClampBlockHeight(block_height);
    const size_t swizzled_slice_size = swizzled_mip_size(width, height, 1, effective_block_height, bytes_per_pixel);

    for (size_t z = 0; z < depth; z++) {
        const size_t swizzled_slice_offset = z * swizzled_slice_size;
        for (size_t y = 0; y < height; y++) {
            for (size_t x = 0; x < width; x++) {
                const size_t linear_index = ((z * height + y) * width + x) * bytes_per_pixel;
                const size_t swizzled_index =
                    swizzled_slice_offset + BlockLinearAddress(x, y, width, effective_block_height, bytes_per_pixel);

                if (swizzled_index + bytes_per_pixel > source_len || linear_index + bytes_per_pixel > destination_len) {
                    continue;
                }

                std::memcpy(destination + linear_index, source + swizzled_index, bytes_per_pixel);
            }
        }
    }
}
