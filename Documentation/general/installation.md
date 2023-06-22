---
outline: deep
---

# Installation

The latest download for the main set of tools (this is the one you probably want) can be found [here](https://github.com/AnthonyFuller/TonyTools/releases/latest/download/TonyTools.zip).

Development tools come with no support or guarantees for how they may work, but they can be downloaded [here](https://github.com/AnthonyFuller/TonyTools/releases/latest/download/TonyTools-Dev.zip).

## ResourceLib

Some tools and libraries require [ResourceLib](https://github.com/OrfeasZ/ZHMTools/releases/latest) from [ZHMTools](https://github.com/OrfeasZ/ZHMTools).

[HMLanguages](/libraries/hmlanguages) requires ResourceLib either next to the executable it's been compiled into or on your PATH.
This also extends to [HMLangaugeTools](/tools/hmlanguagetools).

:::tip
This does not apply to the Linux version of HMLanguageTools as ResourceLib is statically linked.
:::

## Tools
All tools are CLI, to install them, you can either:
- Extract them to a directory that is on your PATH, so they can be used anywhere.
- Extract them to a folder and use them directly from there.

## Libraries
Libraries themselves are statically linked, meaning there are no shared libraries to link to.

Installation instructions for these can vary depending on your workflow. CMake is recommended alongside [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html).

An example on how to include them in a CMake project can be seen below:
```cmake
cmake_minimum_required(VERSION 3.25.0)
include(FetchContent)

project(MyProject)

set(TONYTOOLS_BUILD_TOOLS OFF)
FetchContent_Declare(tonytools
    GIT_REPOSITORY  https://github.com/AnthonyFuller/TonyTools.git
    GIT_TAG         v1.6.1
)

FetchContent_MakeAvailable(tonytools)

add_executable(MyExe
    src/main.cpp
)

add_dependencies(MyExe TonyTools::HMLanguages)
target_link_libraries(MyExe TonyTools::HMLanguages)
```
