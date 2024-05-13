add_executable(image_resource_builder
  src/main.cpp
)

target_link_libraries(image_resource_builder
  PRIVATE
    CLI11::CLI11
    cute_headers::aseprite
    stb::image
    nlohmann_json::nlohmann_json
)
