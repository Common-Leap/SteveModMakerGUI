#pragma once

#include <cstddef>
#include <cstdint>

namespace MsgNameTemplate {

const std::uint8_t* DataForSlot(bool slot_one);
std::size_t SizeForSlot(bool slot_one);

}  // namespace MsgNameTemplate
