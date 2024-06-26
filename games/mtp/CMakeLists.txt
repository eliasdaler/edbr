cmake_minimum_required(VERSION 3.21)

project(
	mtpgame
	VERSION 0.1.0
	LANGUAGES CXX C
)

if (PROJECT_IS_TOP_LEVEL)
  set(EDBR_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
  add_subdirectory("${EDBR_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/edbr")
endif()

add_executable(mtpgame
  src/DevTools/CustomEntityTreeView.cpp

  src/AnimationSoundSystem.cpp
  src/EntityCreator.cpp
  src/EntityUtil.cpp
  src/FollowCameraController.cpp
  src/Level.cpp
  src/PhysicsSystem.cpp
  src/VirtualCharacterParams.cpp

  src/Game.cpp
  src/GameComponents.cpp
  src/GameComponentDisplayers.cpp
  src/GameEntityInit.cpp
  src/GameDevTools.cpp
  src/GameUI.cpp
  src/GameLevels.cpp

  src/MTPSaveFile.cpp

  src/main.cpp
)

set_target_properties(mtpgame PROPERTIES
  CXX_STANDARD 20
  CXX_EXTENSIONS OFF
)

target_add_extra_warnings(mtpgame)

target_include_directories(mtpgame PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src")

target_link_libraries(mtpgame PRIVATE
  edbr::edbr
)

symlink_assets(mtpgame)

# build shaders
get_edbr_common_3d_shaders(EDBR_3D_SHADERS)
target_shaders(mtpgame "${EDBR_3D_SHADERS}")

set_exe_target_common_props(mtpgame)

include("${CMAKE_CURRENT_LIST_DIR}/MTPInstall.cmake")

include(CTest)
if(EDBR_BUILD_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()
