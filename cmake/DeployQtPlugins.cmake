if(NOT DEFINED QT_QMAKE_EXE OR QT_QMAKE_EXE STREQUAL "")
    message(WARNING "[SteveModMaker] QT_QMAKE_EXE not provided; skipping Qt plugin deployment.")
    return()
endif()

if(NOT DEFINED OUTPUT_DIR OR OUTPUT_DIR STREQUAL "")
    message(WARNING "[SteveModMaker] OUTPUT_DIR not provided; skipping Qt plugin deployment.")
    return()
endif()

execute_process(
    COMMAND "${QT_QMAKE_EXE}" -query QT_INSTALL_PLUGINS
    RESULT_VARIABLE QUERY_RESULT
    OUTPUT_VARIABLE QT_PLUGINS_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT QUERY_RESULT EQUAL 0 OR QT_PLUGINS_DIR STREQUAL "")
    message(WARNING "[SteveModMaker] Failed to query Qt plugin path from qmake; skipping Qt plugin deployment.")
    return()
endif()

set(QT_PLATFORMS_DIR "${QT_PLUGINS_DIR}/platforms")
if(NOT EXISTS "${QT_PLATFORMS_DIR}")
    message(WARNING "[SteveModMaker] Qt platforms directory not found at: ${QT_PLATFORMS_DIR}")
    return()
endif()

set(DEST_PLATFORMS_DIR "${OUTPUT_DIR}/platforms")
file(MAKE_DIRECTORY "${DEST_PLATFORMS_DIR}")
file(COPY "${QT_PLATFORMS_DIR}/" DESTINATION "${DEST_PLATFORMS_DIR}")

message(STATUS "[SteveModMaker] Deployed Qt platform plugins to ${DEST_PLATFORMS_DIR}")
