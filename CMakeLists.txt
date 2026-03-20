cmake_minimum_required(VERSION 3.14)

include(cmake/prelude.cmake)

project(
    nalay
    VERSION 0.1.0
    DESCRIPTION "nizar's amazing layouting library"
)

include(cmake/project-is-top-level.cmake)
include(cmake/variables.cmake)

# ---- Declare library ----

add_library(nalay_nalay INTERFACE)
add_library(nalay::nalay ALIAS nalay_nalay)

set_property(
    TARGET nalay_nalay PROPERTY
    EXPORT_NAME nalay
)

target_include_directories(
    nalay_nalay ${warning_guard}
    INTERFACE
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

target_compile_features(nalay_nalay INTERFACE cxx_std_23)

find_package(fmt REQUIRED)
target_link_libraries(nalay_nalay INTERFACE fmt::fmt)

# ---- Install rules ----

if(NOT CMAKE_SKIP_INSTALL_RULES)
  include(cmake/install-rules.cmake)
endif()

# ---- Developer mode ----

if(NOT nalay_DEVELOPER_MODE)
  return()
elseif(NOT PROJECT_IS_TOP_LEVEL)
  message(
      AUTHOR_WARNING
      "Developer mode is intended for developers of nalay"
  )
endif()

include(cmake/dev-mode.cmake)
