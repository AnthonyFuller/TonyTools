# Tony's Tools
[![Build](https://github.com/AnthonyFuller/TonyTools/actions/workflows/build.yml/badge.svg)](https://github.com/AnthonyFuller/TonyTools/actions/workflows/build.yml)  
Some tools I've made, found or edited that are useful to me.

Each tool will include its own usage instructions. To display them just run the tool (all command line).

## Tools Included
All are by me unless specified.
- HMTextureTools
    - Allows you to convert and rebuild various texture formats from the Hitman WOA Trilogy from various platforms.

- HMLanguageTools
    - Allows you to convert and rebuild various language formats from the Hitman WOA Trilogy.
    - DLGE support is currently still in development.

- HMAScrambler
   - Allows you to convert .scrambled to .ini (and vice versa) from the game Hitman Absolution.

- BOXCExporter \[Credit to @dafitius for informing me of the format, DEV\]
    - Allows you to convert BOXC to position and textures.

- JSONPatchCreator \[DEV\]
    - A simple JSON patch creator for testing.

- MJBATesting \[DEV\]
    - A project for me to reverse engineer the MJBA format.

- SCDA \[DEV\]
    - A library/tool for messing with SCDA files.

- VTXD \[DEV\]
    - A library/tool for messing with VTXD files.

## Third-Party Libraries
Various third party libraries are used for my projects, and where the library allows, they have been included through CMake's `FetchContent` feature.
A list of all third-party libraries can be found below:
- [argparse](https://github.com/p-ranav/argparse)
- [DirectXTex](https://github.com/microsoft/DirectXTex)
- [lz4](https://github.com/lz4/lz4/)
- [libmorton](https://github.com/Forceflow/libmorton/)
- [nlohmann json](https://github.com/nlohmann/json)
- [ZHMTools](https://github.com/OrfeasZ/ZHMTools)
- [hash library](https://github.com/stbrumme/hash-library)
