cmake_minimum_required (VERSION 3.0.0)
project (findMUS CXX C)

option(USE_SYSTEM_LIBMINIZINC "Try to load libminizinc targets from CMake package" OFF)
option(BUILD_FINDMUS_EXAMPLES "Build set of cpp example SubProblems" OFF)

message("-- Configuring: C++ options")
include(cmake-helpers/configure_cpp_11.cmake)

message("-- Configuring: libminzinc")
include(cmake-helpers/configure_libminizinc.cmake)

message("-- Configuring: Chuffed")
include(cmake-helpers/configure_chuffed.cmake)

#### #### #### #### #### #### ####

include_directories("include" ${CMAKE_BINARY_DIR})

# The version number.
set (findMUS_VERSION_MAJOR 0)
set (findMUS_VERSION_MINOR 5)
set (findMUS_VERSION_PATCH 0)

add_library(hiermus
  lib/SubProblem.cpp
  lib/Node.cpp
  lib/string_utils.cpp
  lib/path_utils.cpp
  lib/MusEnumerator.cpp
  lib/Marco.cpp
  lib/HierMUSEnumer.cpp
  lib/FznSubProblem.cpp
  lib/Types.cpp
  lib/Selection.cpp
  lib/SubsetMap.cpp
  lib/ChuffedSubsetMap.cpp
  lib/Options.cpp

  include/SubProblem.h
  include/Node.h
  include/string_utils.h
  include/path_utils.h
  include/SubsetMap.h
  include/MusEnumerator.h
  include/Marco.h
  include/HierMUSEnumer.h
  include/FznSubProblem.h
  include/Types.h
  include/Selection.h
  include/ChuffedSubsetMap.h
  include/Options.h
)

if(libminizinc_FOUND)
    add_executable(findMUS src/findMUS.cpp)
    target_link_libraries(findMUS hiermus chuffed_fzn chuffed minizinc_fzn minizinc_solver minizinc_compiler ${CMAKE_THREAD_LIBS_INIT})

    if(BUILD_FINDMUS_EXAMPLES)
      target_include_directories(findMUS PRIVATE examples/cpp)
      add_library(findmus_examples
        examples/cpp/DemoSubProblems.cpp
        examples/cpp/DemoSubProblems.h
      )
      target_compile_definitions(hiermus PRIVATE BUILD_FINDMUS_EXAMPLES)
      target_compile_definitions(findMUS PRIVATE BUILD_FINDMUS_EXAMPLES)
      target_link_libraries(findMUS findmus_examples)
    endif()

    INSTALL(TARGETS findMUS
      RUNTIME DESTINATION bin
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib)
else()
    message("Error: Could not find MiniZinc library. findMUS cannot be compiled. Install libminizinc from source and set CMAKE_PREFIX_PATH or libminizinc_DIR.")
endif()
