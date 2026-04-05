set(nalay_FOUND YES)

include(CMakeFindDependencyMacro)
find_dependency(fmt)

if(nalay_FOUND)
  include("${CMAKE_CURRENT_LIST_DIR}/nalayTargets.cmake")
endif()
