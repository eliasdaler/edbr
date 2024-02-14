function(fetch_submodules)
  find_package(Git REQUIRED)
  message(STATUS "Fetching submodules...")
  execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --depth 1
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
  if(NOT GIT_SUBMOD_RESULT EQUAL "0")
    message(FATAL_ERROR "git submodule update --init --depth 1 failed with ${GIT_SUBMOD_RESULT}, please try checking out submodules manually")
  endif()
endfunction()
