#pragma once

// Minecraft's standard cape atlas and the UV layout used by the generated
// 10x16x1 cape model. Keep these values shared by input validation, the GUI
// preview, CSS artwork, and the generated mod textures.
namespace CapeLayout {

inline constexpr int kAtlasWidth = 64;
inline constexpr int kAtlasHeight = 32;

inline constexpr int kFaceWidth = 10;
inline constexpr int kFaceHeight = 16;
inline constexpr int kModelDepth = 1;

// BoxGeometry-style UV faces for a cape with u=0, v=0, width=10,
// height=16, depth=1:
//   top    (1, 0, 10x1)
//   bottom (11, 0, 10x1)
//   side   (0, 1, 1x16)
//   inside (1, 1, 10x16)
//   side   (11, 1, 1x16)
//   outside(12, 1, 10x16)
inline constexpr int kTopX = 1;
inline constexpr int kTopY = 0;
inline constexpr int kBottomX = 11;
inline constexpr int kBottomY = 0;
inline constexpr int kSideX = 0;
inline constexpr int kSideY = 1;
inline constexpr int kInsideX = 1;
inline constexpr int kInsideY = 1;
inline constexpr int kOutsideX = 12;
inline constexpr int kOutsideY = 1;

inline constexpr int kRequiredWidth = kOutsideX + kFaceWidth;
inline constexpr int kRequiredHeight = kOutsideY + kFaceHeight;

inline constexpr bool HasRequiredFaceBounds(int width, int height) noexcept {
	return width >= kRequiredWidth && height >= kRequiredHeight;
}

} // namespace CapeLayout
