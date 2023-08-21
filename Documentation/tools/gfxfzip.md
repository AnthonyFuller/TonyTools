---
outline: deep
---

# GFXFzip

This tool converts GFXF to zip and vice-versa.  
It is a CLI tool, meaning there is no GUI and you must use a terminal i.e. PowerShell.

:::warning
ResourceLib is required for this tool. For more information, see the [installation](/general/installation#resourcelib) page.

7zip (or more specifically, it's shared library) is required for this tool. For more information, see the [installation](/general/installation#_7zip) page.
:::

## Quickstart

Converting GFXF to zip:

```
GFXFzip convert <game> <input file path> <output file path>
```

The above command assumes that the `.meta.JSON` file is located at `<input file path>.meta.JSON`, to specify it,
add the `--metapath <meta file path>` option.

If using the `--folder` option, `<output file path>` should be a path to a folder, and the tool will output the
zip in that folder with the name `<input file name>.zip`.

Rebuilding zip to GFXF:

```
GFXFzip rebuild <game> <input file path> <output file path>
```

The above command will output the `.meta.JSON` file to `<output file path>.meta.JSON`, to specify it,
add the `--metapath <meta file out path>` option.

If using the `--folder` option, `<output file path>` should be a path to a folder, and the tool will output the
GFXF and meta.JSON in that folder with the name `<hash of GFXF>.GFXF` and `<hash of GFXF>.GFXF.meta.JSON` respectively.

## The Zip File

A quick few notes about the zip file the tool outputs:
- There can only be **one** file ending with `.gfx` and the tool outputs this as the name of the `.swf` from the file hash
    (if available, otherwise it will be the `<hash of GFXF>.gfx`).
    - If there is more than one, the tool will fail to convert.
- The `meta.json` file only contains the hash of the file being input originally, it is required.
    - This is used when the `--folder` option is specified, to allow for tools like SMF to use this tool without knowing the file hash.

## Usage

```
usage: GFXFzip [--metapath path] [--folder] mode game input_path output_path

positional arguments:
    mode            the mode to use: convert or rebuild.
    game            the version of the game to convert/rebuild from/to:
                        H2016, H2, or H3
    input_path      path to the input file
    output_path     path to the output file (or folder if using --folder)

optional arguments:
    --metapath      input/output path for the .meta.JSON (RPKG Tool!)
    --folder        if the output_path is set to a folder.
                        should be used if ther output hash is unknown
```
