function(open_al_soft_flags target)
  if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(${target}
      PRIVATE
        -Wno-inline
        -Wno-ignored-attributes
        -Wno-stringop-overflow
        -Wno-switch
        -Wno-maybe-uninitialized
    )
  endif()
endfunction()
