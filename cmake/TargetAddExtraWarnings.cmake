# Function to add extra warnings for my targets only (don't want to impose extra warnings on thirdparty dependencies)
function(target_add_extra_warnings target)
  if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    # VERY STRICT
    target_compile_options(${target}
      PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )

    # ignore some warnings
    target_compile_options(${target}
      PRIVATE
        -Wno-missing-braces # sometimes not having braces in if(a && b || c) is not that dangerous
        -Wno-volatile # glm
        # whatever:
        -Wno-unused-local-typedefs
        -Wno-unused-variable
        -Wno-unused-parameter
        -Wno-unused-function
        -Wno-unused-but-set-variable
        -Wno-missing-field-initializers
      )

    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
      target_compile_options(${target}
        PRIVATE
          -Wno-null-conversion # ImGui
          -Wno-unused-parameter
          -Wno-unused-function
          -Wno-gnu-string-literal-operator-template # fmt
          -Wgnu-empty-initializer
        )
    endif()
  endif()

  if(MSVC)
    target_compile_options(${target}
      PRIVATE /W3
	    # /permissive- - disabled until MS fixes it!
        /wd4127 # Conditional expression is constant
        /wd4324 # Structure was padded due to alignment specifier
        /wd4505 # Unreferenced local function has been removed
      )
  endif()
endfunction()

