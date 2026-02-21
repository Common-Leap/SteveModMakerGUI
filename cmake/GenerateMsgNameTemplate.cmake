if(NOT DEFINED SLOT_ONE_FILE OR NOT DEFINED OTHER_SLOTS_FILE OR NOT DEFINED OUTPUT_FILE)
    message(FATAL_ERROR "Missing required variables: SLOT_ONE_FILE, OTHER_SLOTS_FILE, OUTPUT_FILE")
endif()

file(READ "${SLOT_ONE_FILE}" SLOT_ONE_HEX HEX)
file(READ "${OTHER_SLOTS_FILE}" OTHER_SLOTS_HEX HEX)

function(hex_to_cpp_bytes HEX_INPUT OUT_BYTES OUT_COUNT)
    string(LENGTH "${HEX_INPUT}" HEX_LEN)
    if(HEX_LEN EQUAL 0)
        set(${OUT_BYTES} "" PARENT_SCOPE)
        set(${OUT_COUNT} 0 PARENT_SCOPE)
        return()
    endif()

    math(EXPR BYTE_COUNT "${HEX_LEN} / 2")
    set(BYTES_TEXT "")
    set(PER_LINE 12)
    math(EXPR LAST_INDEX "${BYTE_COUNT} - 1")
    foreach(I RANGE 0 ${LAST_INDEX})
        math(EXPR OFFSET "${I} * 2")
        string(SUBSTRING "${HEX_INPUT}" ${OFFSET} 2 BYTE_HEX)
        string(APPEND BYTES_TEXT "  0x${BYTE_HEX}")
        if(NOT I EQUAL LAST_INDEX)
            string(APPEND BYTES_TEXT ",")
        endif()
        math(EXPR NEXT "${I} + 1")
        math(EXPR MODULO "${NEXT} % ${PER_LINE}")
        if(MODULO EQUAL 0)
            string(APPEND BYTES_TEXT "\n")
        else()
            string(APPEND BYTES_TEXT " ")
        endif()
    endforeach()

    set(${OUT_BYTES} "${BYTES_TEXT}" PARENT_SCOPE)
    set(${OUT_COUNT} "${BYTE_COUNT}" PARENT_SCOPE)
endfunction()

hex_to_cpp_bytes("${SLOT_ONE_HEX}" SLOT_ONE_CPP SLOT_ONE_COUNT)
hex_to_cpp_bytes("${OTHER_SLOTS_HEX}" OTHER_SLOTS_CPP OTHER_SLOTS_COUNT)

set(GENERATED_TEXT
"#include \"MsgNameTemplate.hpp\"\n\n"
"namespace MsgNameTemplate {\n\n"
"static const std::uint8_t kSlotOneData[] = {\n${SLOT_ONE_CPP}\n};\n"
"static const std::size_t kSlotOneData_len = ${SLOT_ONE_COUNT};\n\n"
"static const std::uint8_t kOtherSlotsData[] = {\n${OTHER_SLOTS_CPP}\n};\n"
"static const std::size_t kOtherSlotsData_len = ${OTHER_SLOTS_COUNT};\n\n"
"const std::uint8_t* DataForSlot(bool slot_one) {\n"
"    return slot_one ? kSlotOneData : kOtherSlotsData;\n"
"}\n\n"
"std::size_t SizeForSlot(bool slot_one) {\n"
"    return slot_one ? kSlotOneData_len : kOtherSlotsData_len;\n"
"}\n\n"
"}  // namespace MsgNameTemplate\n"
)

get_filename_component(OUTPUT_DIR "${OUTPUT_FILE}" DIRECTORY)
file(MAKE_DIRECTORY "${OUTPUT_DIR}")
file(WRITE "${OUTPUT_FILE}" "${GENERATED_TEXT}")
