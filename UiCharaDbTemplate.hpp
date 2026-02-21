#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace UiCharaDbTemplate {

const std::uint8_t* DataForSlot(const std::string& slot_code);
std::size_t SizeForSlot(const std::string& slot_code);

}  // namespace UiCharaDbTemplate
