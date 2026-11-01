cmake_minimum_required(VERSION 3.14)
project(MODQuantum LANGUAGES CXX)

# Quantum++ 6.0 requires C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Eigen 3 is the only external dependency (header-only).
# If CMake cannot find it, pass  -DEIGEN3_INCLUDE_DIR=<path containing the Eigen/ folder>
find_package(Eigen3 3.3 QUIET NO_MODULE)
if(NOT Eigen3_FOUND AND NOT EIGEN3_INCLUDE_DIR)
  message(FATAL_ERROR "Eigen 3 not found. Install it or pass -DEIGEN3_INCLUDE_DIR=/path/to/eigen3")
endif()

add_executable(modquantum
  src/main.cpp
  src/chsh.cpp
  src/bb84.cpp)

target_include_directories(modquantum PRIVATE include third_party/qpp third_party/qasmtools/include)
if(Eigen3_FOUND)
  target_link_libraries(modquantum PRIVATE Eigen3::Eigen)
else()
  target_include_directories(modquantum PRIVATE ${EIGEN3_INCLUDE_DIR})
endif()

# Optional OpenMP (Quantum++ can use it; results do not depend on it)
find_package(OpenMP QUIET)
if(OpenMP_CXX_FOUND)
  target_link_libraries(modquantum PRIVATE OpenMP::OpenMP_CXX)
endif()
if(MSVC)
  target_compile_options(modquantum PRIVATE /W3 /bigobj)
else()
  target_compile_options(modquantum PRIVATE -Wall -Wextra)
endif()
