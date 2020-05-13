# astrengine
In-House Toy Game Engine by Alex Strand

![alt text](https://github.com/astrand130/astrengine/blob/master/docs/Screenshots/HelloTriangle.PNG "HelloTriangle.png")
*(Early Test Screenshot, See Hello Triangle [Upload](https://github.com/astrand130/astrengine/blob/eeef48faa7062fec7acaef64f72084d15573025a/source/tests/main.c#L247), [Shader](shaders/core/StandardScene_FX.glsl) and [Submission](https://github.com/astrand130/astrengine/blob/eeef48faa7062fec7acaef64f72084d15573025a/source/tests/main.c#L46))*


## Features
* Itteration Flexibility
  * Library structure encourages Engine/Game or Engine/Tool separation
  * Start creating a new tool in only a few more lines-of-code than "Hello World"
  * Immediate Mode GUI via [ImGui](https://github.com/ocornut/imgui)/[CImGui](https://github.com/cimgui/cimgui) for rapid tool development (with optional [Nuklear](https://github.com/Immediate-Mode-UI/Nuklear) backend)
  * Developer console with API to easily to expose functions and variables for testing
* Data IO
  * C99 POD shallow Serialization via Designated Initializer macros for easy save/load
  * Binary Container inspired by WAD
  * File System Abstraction for resource locators (WIP)
  * Register config variables for save/load from disk or from the dev console
* Rendering
  * In-house Vulkan renderer
* Asset Pipeline
  * Asset conditioning tools hooked together by small Python script using WAF meta-build system
* More to Come
  * Mesh building API
  * Lightweight PBR renderer
 
## Use Case
I am developing this as a hobby. Please don't use this in production with any risk.

## Dependencies
### Precompiled Dependencies
* SDL https://www.libsdl.org/download-2.0.php
* Shaderc https://github.com/google/shaderc/blob/master/downloads.md
* (Zip of All - Windows) https://drive.google.com/open?id=1qafd7WMtvplOJj94ipOUA_4B7pLW8xR2
### Default Assets and Shaders + WAF Asset Build System
* https://github.com/astrand130/astrengine-assets
### Installation Walkthrough
* Windows Batch: https://drive.google.com/open?id=1-DxLFx7Te8juICgSsDHFG8qtwOjb6Rmv
