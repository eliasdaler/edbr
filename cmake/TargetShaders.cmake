find_program(GLSL_VALIDATOR glslangValidator
  HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/bin/ $ENV{VULKAN_SDK}/bin32/
)

function (target_shaders target shaders)
  set(SHADERS_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
  foreach (SHADER_PATH ${SHADERS})
    get_filename_component(SHADER_FILENAME "${SHADER_PATH}" NAME)
    set(SHADER_SPIRV_PATH "${SHADERS_BUILD_DIR}/${SHADER_FILENAME}.spirv")
    add_custom_command(
      COMMENT "Building ${SHADER_FILENAME}"
      OUTPUT "${SHADER_SPIRV_PATH}"
      COMMAND ${GLSL_VALIDATOR} -V "${SHADER_PATH}" -o "${SHADER_SPIRV_PATH}" --quiet
      DEPENDS "${SHADER_PATH}"
    )
    list(APPEND SPIRV_BINARY_FILES ${SHADER_SPIRV_PATH})
  endforeach()

  set(shaders_target_name "${target}_build_shaders")
  add_custom_target(${shaders_target_name}
    DEPENDS ${SPIRV_BINARY_FILES}
  )

  add_dependencies(${target} ${shaders_target_name})
endfunction()
