---
outline: deep
---

# BOXCExporter

:::warning
This tool is currently in-development.
:::

This tool exports BOXC from Hitman 2 and 3 to JSON (for positions) and TGA files.

## Usage

```
usage: BOXCExporter [--noconvert] mode boxc output_path

positional arguments:
    mode            the mode to use: convert or rebuild.
    boxc            path to the BOXC file.
    output_path     path to the output folder.

optional arguments:
    --noconvert     use this option to skip converting textures to TGA.
```
