# EDBR - Elias Daler's Bikeshed Renderer

This project initially started for learning Vulkan but quickly grew into a full-fledged game engine with which I write my games with.

This is a monorepo and contains both the engine (in `edbr` directory) and various games/test projects (in `games` directory).

The article which explains some implementation details: https://edw.is/learning-vulkan/

Playable demo of cat adventure game: https://eliasdaler.itch.io/project-mtp-dev

## Build dependencies

* Vulkan SDK
* CMake 3.21+
* Linux: whatever SDL2 needs

## License and contributions

This is a "source available" repository and not open source (yet). Feel free to look around and use code for inspiration. When the time comes, I might release this under a more friendly open source license (possibly even public domain).

Feel free to open issues if something doesn't work.

**Sorry, but I currently won't accept any PRs until the project has a real license. It might be hard to re-license it later with external contributions.**

## Current progress

![dev_tools](screenshots/10_dev_tools.png)

![dialogue](screenshots/12_dialogue.png)

![burger_joint](screenshots/11_burger_joint.png)

https://github.com/eliasdaler/edbr/assets/1285136/f689d3dd-5556-4a7c-9765-aa7724cf416b

## Features

* glTF scene loading
* Basic PBR lighting (no IBL) - support for normal, metallic/roughness maps and emissive textures
* Frustum culling
* Compute skinning + skeletal animation
* Exponential fog
* MSAA
* Bindless textures and samplers (via descriptor indexing)
* Directional, point and spot lights
* Cascaded shadow maps
* Efficient sprite and rect drawing (using batching)
* Text drawing with UTF-8 support
* Custom Dear ImGui backend with sRGB and bindless texture support
* Sound via OpenAL-soft
* ogg music playback
* Jolt Physics integration
* UI system with auto-layout, auto-resize and pixel perfect drawing support

## Vulkan usage

* Vulkan 1.3
* No render passes used, everything is dynamic rendering
* Bindless textures (aka descriptor indexing)
* Using Vulkan Memory Allocator, volk and vk-bootstrap to make the code simpler

