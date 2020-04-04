# astrengine
In-House Game Engine by Alex Strand

## Features
* Itteration Flexibility
  * Library structure encourages Engine/Game or Engine/Tool separation
  * Start creating a new tool in only a few more lines-of-code than "Hello World"
  * Immediate Mode GUI via ImGui/CImGui for rapid tool development (with optional Nuklear backend)
  * Developer console with API to easily to expose functions and variables for testing
* Data IO
  * C99 POD shallow Serialization via Designated Initializer macros for easy save/load
  * Binary Container inspired by WAD
  * File System Abstraction for resource locators (WIP)
  * Register config variables for save/load from disk or from the dev console
* Vulkan
  * In-house Vulkan renderer
* Asset Pipeline
  * Asset conditioning tools hooked together by small Python script using WAF meta-build system
* More to Come
  * Mesh building API
  * Lightweight PBR renderer
  * Entity Component System
 
## Use Case
I am developing this as a hobby.

## Dependencies
### Precompiled Dependencies
* Windows https://drive.google.com/open?id=1qafd7WMtvplOJj94ipOUA_4B7pLW8xR2
### Default Assets and Shaders + Python Asset Build System
* https://github.com/astrand130/astrengine-assets
### Install Helpers
* Windows Batch: https://drive.google.com/open?id=1-DxLFx7Te8juICgSsDHFG8qtwOjb6Rmv
