cmake_minimum_required (VERSION 3.0.0)

if (USE_STDLIBCPP)
  set(LIBCPP libstdc++)
else()
  set(LIBCPP libc++)
endif()

if(APPLE)
  execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE OSX_SYSROOT OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(CMAKE_OSX_SYSROOT ${OSX_SYSROOT})
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE STRING "Minimum OS X deployment version")
endif(APPLE)

project (findMUS CXX C)

if ( (CMAKE_VERSION VERSION_GREATER 3.1.0) OR (CMAKE_VERSION VERSION_EQUAL 3.1.0) )
  set (CMAKE_CXX_STANDARD 11)
else()
  include(CheckCXXCompilerFlag)
  check_cxx_compiler_flag(-std=c++11 HAS_STDCPP11)
  if (HAS_STDCPP11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  else()
    check_cxx_compiler_flag(-std=c++0x HAS_STDCPP0X)
    if (HAS_STDCPP0X)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
    else()
      message(SEND_ERROR "CMake >= 3.1.0 required to detect whether compiler supports C++11")
    endif()
  endif()
endif()

set(CHUFFED_SOURCE_DIR "${CMAKE_SOURCE_DIR}/submodules/chuffed_assump/")
add_subdirectory(${CHUFFED_SOURCE_DIR} EXCLUDE_FROM_ALL)

include_directories("include" ${CMAKE_BINARY_DIR} ${CHUFFED_SOURCE_DIR})

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
  lib/Types.cpp            include/Types.h 
                           include/Options.h)

find_package(libminizinc PATHS ${CMAKE_INSTALL_PREFIX}/lib/CMake/ NO_DEFAULT_PATH)
if(libminizinc_FOUND)
    message("MiniZinc library found at ${libminizinc_INCLUDE_DIRS}!")

    include_directories(${libminizinc_INCLUDE_DIRS})
    add_executable(findMUS src/findMUS.cpp)
    target_link_libraries(findMUS hiermus chuffed_fzn chuffed minizinc_fzn minizinc_solver minizinc ${CMAKE_THREAD_LIBS_INIT})
else()
    message("Error: Could not find MiniZinc library. findMUS cannot be compiled. Install libminizinc from source and set CMAKE_PREFIX_PATH or libminizinc_DIR.")
endif()


