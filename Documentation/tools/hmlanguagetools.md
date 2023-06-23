---
outline: deep
---

# HMLanguageTools

This tool converts CLNG, DITL, DLGE, LOCR, and RTLV to JSON and vice-versa.
It is a CLI tool, meaning there is no GUI and you must use a terminal i.e. PowerShell.

:::warning
ResourceLib is required for this tool. For more information, see the [installation](/general/installation#resourcelib) page.
:::

:::tip Formats
Intricacies of the formats will **not** be discussed on this page, it will only go through usage of the tool.
For more information on the formats, read the [HMLanguages](/libraries/hmlanguages) page.
:::

## Quickstart

Converting any type file to JSON:
```
HMLanguageTools convert <game> <type> <input file path> <output file path> 
```
The above command assumes that the `.meta.JSON` file is located at `<input file path>.meta.JSON`, to specify it, add the `--metapath <meta file path>` option.  
If converting DLGE, and you want more accuracy for the random container weights, add the `--hexprecision` option when converting. It is not required for rebuilding.

:::info Note
For early H2016 LOCR files it may fail to convert. This is due to a different cipher being used, you can add the `--symmetric` flag to fix this.
:::

Rebuilding any JSON to a raw file + .meta.JSON:
```
HMLanguageTools rebuild <game> <type> <input json path> <output file path>
```
The above command will output the `.meta.JSON` file to `<output file path>.meta.JSON`, to specify it, add the `--metapath <meta file out path>` option.

:::danger Language Maps
When converting and rebuilding DLGE, you **must ensure that the language maps being used are correct for the languages in the file**. If there are more or less in the map, the tool will fail to convert/rebuild.

The same applies to RTLV, but only on rebuild.

If the default language maps are incorrect for the file you're converting/rebuilding, you must specify them using the `--langmap` option.

More information on language maps can be found on the [HMLanguages](/libraries/hmlanguages#language-maps) page.
:::

## Usage

```
usage: HMLanguageTools [--metapath path] [--langmap map]
    [--defaultlocale locale] [--hexprecision] [--symmetric]
    mode game type input_path output_path

positional arguments:
    mode            the mode to use: convert or rebuild.
    game            the version of the game to convert/rebuild from/to:
                        H2016, H2, or H3
    type            the type of the file:
                        CLNG, DITL, DLGE, LOCR, or RTLV
    input_path      path to the input file
    output_path     path to the output file

optional arguments:
    --metapath      input/output path for the .meta.JSON (RPKG Tool!)
    --langmap       custom language map, overrides the one provided by version:
                        e.g. xx,en,fr,it,de,es
    --defaultlocale the default audio locale, used for DLGE conversion
                        and rebuilding. default: en
    --hexprecision  use this option if random weights should be output as their
                        hex variants allowing for greater precision,
                        used for DLGE convert only
    --symmetric     if a symmetric cipher should be used, early H2016 LOCR only.
```
