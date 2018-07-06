
set(CHUFFED_SOURCE_DIR "${CMAKE_SOURCE_DIR}/submodules/chuffed_assump/")
add_subdirectory(${CHUFFED_SOURCE_DIR} EXCLUDE_FROM_ALL)
include_directories(${CHUFFED_SOURCE_DIR})
