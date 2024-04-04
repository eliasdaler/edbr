project(MTPUnitTest
  LANGUAGES CXX
  VERSION 0.1
)

add_executable(mtp_unit_test)
add_dependencies(check mtp_unit_test)

set_target_properties(mtp_unit_test PROPERTIES
    CXX_STANDARD 20
    CXX_EXTENSIONS OFF
)

target_sources(mtp_unit_test
  PRIVATE
    TestSceneInit.cpp
)

target_sources(mtp_unit_test
  PRIVATE
    # can't link to the executable...
    # FIXME: make object lib?
    "../src/JoltDebugRenderer.cpp"
    "../src/PhysicsSystem.cpp"
    "../src/GameSceneLoader.cpp"
    "../src/EntityUtil.cpp"
    "../src/EntityCreator.cpp"
)

target_include_directories(mtp_unit_test
  PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../src"
)

set(EXECUTABLE_NAME "mtp_unit_test")
set_target_properties(mtp_unit_test PROPERTIES
  OUTPUT_NAME ${EXECUTABLE_NAME}
)

target_link_libraries(mtp_unit_test
  PUBLIC
    edbr::edbr
    GTest::gtest_main
    GTest::gmock
)

target_add_extra_warnings(mtp_unit_test)

# discover Google Test tests
include(GoogleTest)
gtest_discover_tests(mtp_unit_test)

# Set correct working directory for debug in MSVC
if(MSVC)
  set_target_properties(mtp_unit_test
    PROPERTIES
      VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
  )
endif()
