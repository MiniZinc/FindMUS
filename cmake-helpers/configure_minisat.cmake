
set(MINISAT_SOURCE_DIR "${CMAKE_SOURCE_DIR}/mapsolvers/minisat/")
add_subdirectory(${MINISAT_SOURCE_DIR} EXCLUDE_FROM_ALL)
include_directories(${MINISAT_SOURCE_DIR})
