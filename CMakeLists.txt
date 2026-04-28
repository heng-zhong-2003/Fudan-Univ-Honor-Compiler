cmake_minimum_required(VERSION 3.20)

project(FDMJCompiler)

# generate compile_commands.json to assist clangd linting
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include_directories(include/backend/llvm)
include_directories(include/backend/arm)
include_directories(include/optimizer)
include_directories(include/frontend)
include_directories(${PROJECT_BINARY_DIR}/include/frontend/)
include_directories(include/utils/dsa)
include_directories(include/utils/printer)
include_directories(vendor/lxml)

add_subdirectory(lib)
add_subdirectory(tools)
add_subdirectory(vendor)
