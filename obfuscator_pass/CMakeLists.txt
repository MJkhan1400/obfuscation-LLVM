cmake_minimum_required(VERSION 3.10)
project(ObfuscatorPass CXX C)

# Find LLVM
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Add LLVM include directories and definitions
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Define the pass as a shared library
add_library(ObfuscatorPass MODULE Obfuscator.cpp)

# Set C++17 standard
set_target_properties(ObfuscatorPass PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    PREFIX ""  # Don't add 'lib' prefix on some platforms
)

# Link against LLVM libraries
llvm_map_components_to_libnames(llvm_libs support core passes)
target_link_libraries(ObfuscatorPass PRIVATE ${llvm_libs})
