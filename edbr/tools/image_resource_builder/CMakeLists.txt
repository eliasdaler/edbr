add_executable(image_resource_builder
  src/main.cpp
)

set_target_properties(image_resource_builder PROPERTIES
  CXX_STANDARD 20
  CXX_EXTENSIONS OFF
)

target_link_libraries(image_resource_builder
  PRIVATE
    CLI11::CLI11
    cute_headers::aseprite
    stb::image
    nlohmann_json::nlohmann_json
)
