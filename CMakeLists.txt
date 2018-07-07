cmake_minimum_required (VERSION 3.0.0)
project (findMUS CXX C)

message("-- Configuring: C++ options")
include(cmake-helpers/configure_cpp_11.cmake)

option(USE_SYSTEM_LIBMINIZINC "Try to load libminizinc targets from CMake package" OFF)

message("-- Configuring: libminzinc")
include(cmake-helpers/configure_libminizinc.cmake)

message("-- Configuring: Chuffed")
include(cmake-helpers/configure_chuffed.cmake)

include_directories("include" ${CMAKE_BINARY_DIR})

# The version number.
set (findMUS_VERSION_MAJOR 0)
set (findMUS_VERSION_MINOR 1)
set (findMUS_VERSION_PATCH 0)

add_library(hiermus
  lib/DemoSubProblems.cpp  include/DemoSubProblems.h
  lib/SubProblem.cpp       include/SubProblem.h
  lib/Node.cpp             include/Node.h
  lib/string_utils.cpp     include/string_utils.h
  lib/path_utils.cpp       include/path_utils.h
  lib/SubsetMap.cpp        include/SubsetMap.h
  lib/MusEnumerator.cpp    include/MusEnumerator.h
  lib/Marco.cpp            include/Marco.h
  lib/ReMUS.cpp            include/ReMUS.h
  lib/StackMUS.cpp         include/StackMUS.h
  lib/HierMUSEnumer.cpp    include/HierMUSEnumer.h
  lib/FznSubProblem.cpp    include/FznSubProblem.h  
  lib/Types.cpp            include/Types.h)

if(libminizinc_FOUND)
    add_executable(findMUS src/findMUS.cpp)
    target_link_libraries(findMUS hiermus chuffed_fzn chuffed minizinc_fzn minizinc_solver minizinc ${CMAKE_THREAD_LIBS_INIT})

    INSTALL(TARGETS findMUS
      RUNTIME DESTINATION bin
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib)

else()
    message("Error: Could not find MiniZinc library. findMUS cannot be compiled. Install libminizinc from source and set CMAKE_PREFIX_PATH or libminizinc_DIR.")
endif()
