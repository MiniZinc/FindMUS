if(USE_SYSTEM_LIBMINIZINC)
  find_package(libminizinc)
endif()

if(NOT libminizinc_FOUND)
  message("cmake-helpers/configure_libminizinc.cmake: Not using system libminizinc (set USE_SYSTEM_LIBMINIZINC). Submodule will be compiled instead.")
  set(LIBMINIZINC_SOURCE_DIR "${CMAKE_SOURCE_DIR}/submodules/libminizinc_develop/")
  add_subdirectory(${LIBMINIZINC_SOURCE_DIR} EXCLUDE_FROM_ALL)
  set(libminizinc_INCLUDE_DIRS 
    ${CMAKE_BINARY_DIR}/submodules/libminizinc_develop/include
    ${LIBMINIZINC_SOURCE_DIR}/include
    ${LIBMINIZINC_SOURCE_DIR}/lib/cached/) # Include the cached directory last
  set(libminizinc_FOUND true)
else()
  message("System MiniZinc library found at ${libminizinc_INCLUDE_DIRS}!")
endif()

message("cmake-helpers/configure_libminizinc.cmake: Using includes: ${libminizinc_INCLUDE_DIRS}")
include_directories(${libminizinc_INCLUDE_DIRS})
