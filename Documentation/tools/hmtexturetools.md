---
outline: deep
prev: false
---

# HMTextureTools

This tool converts TEXT/D formats from Hitman: World of Assassination games and Hitman: Absolution to TGA, and vice-versa.
It is a CLI tool, meaning there is no GUI and you must use a terminal i.e. PowerShell.

:::warning Windows Only
Currently, this tool only works on Windows by default due to the usage of DirectXTex, it can run in Wine, but may be finicky to get working or may not work at all.
:::

## Quickstart

### Hitman 3

Converting a TEXT + TEXD to TGA:
```
HMTextureTools convert H3 <path to TEXT> <path to TGA> --texd <path to TEXD>
```
To convert just a TEXT, omit `--texd <path to TEXD>`

Rebuilding a TGA to TEXT + TEXD:
```
HMTextureTools rebuild H3 <path to TGA> <path to TEXT> --rebuildboth \
    --texdoutput <path to TEXD>
```
To build just a TEXT, omit `--texdoutput <path to TEXD>` and `--rebuildboth`.  
It is not possible to just build a TEXD in Hitman 3, a TEXT is required for a TEXD to work.

### Hitman 2016/2

Converting a TEXD to TGA:
```
HMTextureTools convert {H2016|H2} <path to TEXD> <path to TGA> --istexd
```
To convert just a TEXT, replace `<path to TEXD>` with `<path to TEXT>` and omit `--istexd`.

Rebuilding a TGA to TEXT + TEXD:
```
HMTextureTools rebuild {H2016|H2} <path to TGA> <path to TEXT> \ 
    --texdoutput <path to TEXD> --rebuildboth
```
To build just a TEXT, omit `--texdoutput <path to TEXD>` and `--rebuildboth`.  
To build just a TEXD, omit `--rebuildboth` and add `--istexd`.

## Usage

```
usage: HMTextureTools [--texd path] [--port game] [--rebuildboth]
    [--texdoutput path] [--istexd] [--metapath path] [--ps4swizzle]
    mode game texture output_path

positional arguments:
    mode                the mode to use: convert or rebuild.
    game                the version of the game the texture(s) are from:
                            HMA, H2016, H2, or H3
    texture             path to the texture. convert requires TEXT/D, rebuild
                            requires a TGA. (For H3, pass a TEXT).
    output_path         the path to the output file.

optional arguments:
    --texd <path>       path to the input TEXD, H3 only, does nothing for 
                            rebuild or other games.
    -p, --port <game>   the game to port to, HMA is unsupported. convert only.
    --rebuildboth       use this option for the input TGA to be downscaled to
                            make a TEXT and TEXD. rebuild only.
    --texdoutput <path> path where the TEXD should be output. H3 only.
    --istexd            use this option when the input texture is a TEXD.
                            (H2016/2 only, rebuilding overrides this).
    --metapath <path>   path to use for inputting/outputting the meta.
    --ps4swizzle        use this option if you want to deswizzle/swizzle
                            textures (porting will only deswizzle).
```
