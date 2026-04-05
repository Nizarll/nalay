cmake_minimum_required(VERSION 3.14)

project(nalayExamples LANGUAGES CXX)

include(../cmake/project-is-top-level.cmake)
include(../cmake/folders.cmake)

# ---- Dependencies ----

if(PROJECT_IS_TOP_LEVEL)
  find_package(nalay REQUIRED)
endif()

find_package(raylib REQUIRED)
# ---- Examples ----

add_subdirectory(/home/nizar/projects/nalay/tracy tracy)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")


add_executable(nalay_example source/example.cpp)
# target_compile_definitions(nalay_example PRIVATE TRACY_ENABLE)
target_link_libraries(nalay_example PRIVATE
  #  TracyClient
  nalay::nalay
  raylib
)
target_compile_features(nalay_example PRIVATE cxx_std_23)
target_compile_options(nalay_example PRIVATE -g3)

# ---- End-of-file commands ----

add_folders(Examples)
