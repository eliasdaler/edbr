add_executable(aseprite_convert
  src/main.cpp
)

target_link_libraries(aseprite_convert
  PRIVATE
    CLI11::CLI11
    cute_headers::aseprite
    stb::image
    nlohmann_json::nlohmann_json
)
