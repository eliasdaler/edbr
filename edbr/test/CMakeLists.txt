project(UnitTest
  LANGUAGES CXX
  VERSION 0.1
)

add_executable(unit_test)
add_dependencies(check unit_test)

set_target_properties(unit_test PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF
)

target_sources(unit_test
  PRIVATE
    TestBasic.cpp
)

target_include_directories(unit_test
  PRIVATE
    ${edge_SOURCE_DIR}/src
)

set(EXECUTABLE_NAME "unit_test")
set_target_properties(unit_test PROPERTIES
  OUTPUT_NAME ${EXECUTABLE_NAME}
)

target_link_libraries(unit_test
  PUBLIC
    edbr::edbr
    GTest::gtest_main
)

target_add_extra_warnings(unit_test)

# discover Google Test tests
include(GoogleTest)
gtest_discover_tests(unit_test)

# Set correct working directory for debug in MSVC
if(MSVC)
  set_target_properties(unit_test
    PROPERTIES
      VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
  )
endif()
