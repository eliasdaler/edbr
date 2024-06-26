set(GAME_INSTALL_DIR "mtpgame")

install(TARGETS mtpgame
  RUNTIME DESTINATION ${GAME_INSTALL_DIR}
)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets"
  DESTINATION "${GAME_INSTALL_DIR}"
  # exclude test levels
  PATTERN "assets/models/levels/damaged_helmet" EXCLUDE
  PATTERN "assets/models/levels/normal_map_test" EXCLUDE
  PATTERN "assets/models/levels/burger_joint" EXCLUDE
  PATTERN "assets/levels/burger_joint.json" EXCLUDE
)

set(SHADERS_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
if (MSVC)
  set(SHADERS_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/shaders")
endif()

install(DIRECTORY "${SHADERS_BUILD_DIR}"
  DESTINATION "${GAME_INSTALL_DIR}"
)

if (WIN32)
  install(TARGETS OpenAL
    RUNTIME DESTINATION "${GAME_INSTALL_DIR}"
  )
endif()
