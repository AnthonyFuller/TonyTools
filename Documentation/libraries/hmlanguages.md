---
outline: deep
prev: false
next: false
---

<script setup lang="ts">
// Allows us to change images based on theme.
import { useData } from 'vitepress'

const { isDark } = useData()
</script>

# HMLanguages

This library allows you to convert between CLNG, DITL, DLGE, LOCR, and RTLV to JSON and vice-versa.  
On how to add the library to your project, see the [installation](/general/installation) page.

The entire library can be included in a file through `#include <TonyTools/Languages.h>`.

There is a [tool](/tools/hmlanguagetools) which implements this library in full. The [source](https://github.com/AnthonyFuller/TonyTools/blob/master/Tools/HMLanguageTools)
can be used as a reference implementation.

This page will go into detail about the formats, naming conventions used by the tool, and more. Technical details, such as structure of the raw files, will be in collapsible sections.

## Language Maps

HMLanguages uses something called a "language map" (langmap for short) when converting and rebuilding files. These aren't always required to be explicitly defined by the user, and most of the time can be left to the default ones. They're used for outputting files, and for some files don't need to be exact.

The first table below shows how each file type uses the language map, the second table shows the default language map for each version alongside a description of their source.

| File Type 	| Language Map Usage                                                                                                	|
|-----------	|-------------------------------------------------------------------------------------------------------------------	|
| CLNG      	| Used for `convert` only to get the keys for the `languages` object.                                               	|
| DITL      	| Not used.                                                                                                         	|
| DLGE      	| Used for `convert` and `rebuild` to know how many languages are in the file. They have to be **exactly correct**. 	|
| LOCR      	| Used for `convert` only to get the keys for the `languages` object.                                               	|
| RTLV      	| Used for `rebuild` only to get the depends index for videos.                                                      	|

<p style="text-align: center; font-style: italic;">Figure 1: Table of file types and their usage of language maps</p>

| Version 	| Default Language Map                   	| Notes                                              	|
|---------	|----------------------------------------	|----------------------------------------------------	|
| H2016   	| `xx,en,fr,it,de,es,ru,mx,br,pl,cn,jp`    	| Late H2016, earlier language files may have less.  	|
| H2      	| `xx,en,fr,it,de,es,ru,mx,br,pl,cn,jp,tc` 	| N/A                                                	|
| H3      	| `xx,en,fr,it,de,es,ru,cn,tc,jp`          	| Late H3, earlier versions use `xx,en,fr,it,de,es`. 	|

<p style="text-align: center; font-style: italic;">Figure 2: Table of game versions and their default language maps</p>

## API Overview

HMLanguages exposes a C++ API to allow conversion and rebuilding of file types, the individual functions will be laid out for the specific file types in the formats section below, but here, we shall go over two important constructs.

### Version Enum

This is used by virtually all file types, and is mainly used for getting the default language map. Some formats may have version specific quirks or it is required for ResourceLib.

The enum can be seen below, (0 and 1 are reserved for Hitman: Absolution and the 2016 Alpha):

```cpp
// This is in the TonyTools::Language namespace
enum class Version : uint8_t
{
    H2016 = 2,
    H2,
    H3
};
```

### Return Values

When using functions that convert a raw file + `.meta.json` to a HML JSON string, they return `std::string`.

If the library fails to convert a file, an error will be output to `stderr` and the function will return an empty string.

This can be checked like so:

```cpp
std::string converted = ...;

if (converted.empty())
    // do something
```

---

When using functions that rebuild a HML JSON string to the raw file + `.meta.json`, they return a struct that can be seen below:

```cpp
// This is in the TonyTools::Language namespace
struct Rebuilt
{
    std::vector<char> file; // contains the raw file bytes
    std::string meta;       // contains the JSON string of the .meta.json
};
```

If the library fails to rebuild a file, an error will be output to `stderr` and an empty struct will be returned.

This can be checked like so:

```cpp
TonyTools::Language::Rebuilt rebuilt = ...;

if (rebuilt.file.empty() || rebuilt.meta.empty())
    // do something
```

:::tip Note
There are currently long-term plans to use custom exceptions instead of just returning an empty struct so then the burden of error messages is on the program using the library.
:::

## Formats

The following sections will outline the formats, specifically their HMLanguages JSON representation alongside a description of how they work.
These sections also include how to use HMLanguages to convert and rebuild these file types.

The JSON representations **will not** be complete, but, will have enough detail as required. They will have certain elements omitted, this will be indicated with an ellipsis (...).

This library includes schemas in the output files, these have been omitted on this page, but links to the schemas are included.

But before we start, it is recommended to read the [glossary](#glossary) first to become familiar with some terms used.

---

### CLNG
*aka **DialogCascadingLanguageDependencies***

Not much is actually know about this file type, other than it corresponds to the current language map in the game version. HMLanguages can convert the file to an *assumed* representation (as the languages just have a `0x00` byte or a `0x01` byte which is presumed to be a boolean).

#### JSON Representation

[Schema](/schemas/clng.schema.json)
```json
{
    "hash": "...",
    "languages": {
        "xx": false,
        "en": false,
        "fr": true,
        ...
    }
}
```

#### API

:::tip An Outlier
CLNG is a format which does not require anything extra other than the HML JSON string to rebuild. This is because the language map is always stored in the file itself.
:::

```cpp
// CLNG + meta.json -> JSON
std::string json = TonyTools::Language::CLNG::Convert(
    Language::Version version,  // game version
    std::vector<char> data,     // raw CLNG data
    std::string metaJson,       // .meta.json string
    std::string langMap = ""    // optional custom langmap
);

// JSON -> CLNG + meta.json
TonyTools::Language::Rebuilt rebuild =
    TonyTools::Langauge::CLNG::Rebuild(
        std::string jsonString  // HML JSON string
    );
```

---

### DITL
*aka **DialogSoundTemplateList***

This format defines the [soundtag](#glossary) to the corresponding [Wwise](#glossary) [Event](#glossary) (WWEV) by its hash/path. The soundtag names (keys of the `soundtags` object) are CRC32 hashed in the file. As the names of the soundtag are in the actual event file, we know all of them. In the unlikely event that one is unknown by the internal hash list in the library, they will be output as the hexadecimal representation of the CRC32 hash found in the file.

#### JSON Representation

[Schema](/schemas/ditl.schema.json)
```json
{
    "hash": "...",
    "soundtags": {
        "In-world_AI_Important": "...",
        "In-world_AI_Normal": "...",
        ...
    }
}
```

#### API

:::tip An Outlier
DITL is the one format that doesn't require version to be specified for convert or rebuild! This is due to language maps not being used and there is no version specific quirks to the format.
:::

```cpp
// DITL + meta.json -> JSON
std::string json = TonyTools::Language::DITL::Convert(
    std::vector<char> data,     // raw CLNG data
    std::string metaJson        // .meta.json string
);

// JSON -> DITL + meta.json
TonyTools::Language::Rebuilt rebuild =
    TonyTools::Langauge::DITL::Rebuild(
        std::string jsonString  // HML JSON string
    );
```

---

### DLGE
*aka **DialogEvent***

Now, this format is **complex**, and that's putting it lightly. This ~~is~~ will be the longest section for a reason, so strap in, and make sure you know your [definitions](#glossary).

This format, in its simplest terms, defines dialogue. These could include subtitles (optional), [Wavs (WWES/M)](#glossary), and FaceFX animations (FXAS).

Going more in-depth, this format comprises different containers. The 4 container types are: **WavFile**, **Random**, **Switch**, and **Sequence**.

An overview of them can be found in the table below, we go more in-depth in the following section.

:::info
File table not found.
:::

<p style="text-align: center; font-style: italic;">Figure 3: Table of DLGE containers and a brief description of them</p>

A basic diagram (showcasing all containers) can be seen below.

<img v-if="isDark" src="/images/dlge/dark-tree.svg" />
<img v-else src="/images/dlge/light-tree.svg" />

<p style="text-align: center; font-style: italic;">Figure 4: Tree diagram showing how DLGE containers are connected together (read from left to right)</p>

:::warning
This section is currently a work in progress and will be improved upon in the future.
:::

---

### LOCR
*aka **LocalizedTextResource***

The classic (and possibly the most popular) localization format from the World of Assassination trilogy.

This format defines a [LINE](#glossary) hash to a string per language.
Currently, there is no hash list for these (one is planned for far in the future), so they are all output as CRC32 hashes in hexadecimal.
A language map **is required** for convert as per the [table](#language-maps) whether that be from the version or user supplied.

The JSON format allows for users to put the plain-text version e.g. `UI_LOCATION_PARIS_COUNTRY` instead of the hash, these will be hashed upon rebuilding.

#### JSON Representation

[Schema](/schemas/locr.schema.json)
```json
{
    "hash": "...",
    "languages": {
        "xx": {
            "UI_MY_SUIT_NAME": "**Placeholder** My Suit"
        },
        "en": {
            "UI_MY_SUIT_NAME": "My Epic Suit",
            "DEADBEEF": "..." // Example for a hash in hexadecimal
        },
        ...
    }
}
```

#### API

```cpp
// LOCR + meta.json -> JSON
std::string json = TonyTools::Language::LOCR::Convert(
    Language::Version version,      // game version
    std::vector<char> data,         // raw LOCR data
    std::string metaJson,           // .meta.json string
    std::string langMap = ""        // optional language map
);

// JSON -> LOCR + meta.json
TonyTools::Language::Rebuilt rebuild =
    TonyTools::Langauge::LOCR::Rebuild(
        Language::Version version,  // game version
        std::string jsonString      // HML JSON string
    );
```

---

### RTLV
*aka **RuntimeLocalizedVideo***

This format defines videos (for different languages, `en` is the default for all languages unless explicitly defined otherwise) and subtitles per language.

#### JSON Representation

[Schema](/schemas/rtlv.schema.json)
```json
{
    "hash": "...",
    "videos": {
        "en": "...",
        "jp": "..." // Only used by IOI in H2016, can be used in any version
    },
    "subtitles": {
        "xx": "...",
        "en": "...",
        ...
    }
}
```

#### API

:::warning
The game version is important here!

For convert, it is used to pass to ResourceLib.

For rebuild, it is not only used to pass to ResourceLib but also for language map resolution.
:::

```cpp
// RTLV + meta.json -> JSON
std::string json = TonyTools::Language::RTLV::Convert(
    Language::Version version,      // game version
    std::vector<char> data,         // raw RTLV json
    std::string metaJson            // .meta.json string
);

// JSON -> RTLV + meta.json
TonyTools::Language::Rebuilt rebuild =
    TonyTools::Langauge::RTLV::Rebuild(
        Language::Version version,  // game version
        std::string jsonString,     // HML JSON string
        std::string langMap = ""    // optional language map
    );
```

## Glossary

- **Hash/path** - These terms will be used synonymously, they mean the (truncated) MD5 hash of files, or their full path (if known).
- **Soundtag** - Used in [DLGE](#dlge), defined in [DITL](#ditl). The way a piece of dialogue will be played i.e. in-world, handler, etc.
- **Wwise** - Refers to [Audiokinetic Wwise](https://www.audiokinetic.com/en/products/wwise/), the audio middleware used in the Hitman WoA trilogy.
    - **Event** - Refers to Wwise Events, read more on them [here](https://www.audiokinetic.com/en/library/edge/?source=WwiseFundamentalApproach&id=understanding_events).
        Soundtags specifically use ["Dynamic Dialogue" events](https://www.audiokinetic.com/en/library/edge/?source=Help&id=understanding_dynamic_dialogue_system).
        You can use [this](https://github.com/glacier-modding/G2WwiseDataTool) tool to create them for the game.
    - **Wavs/Wems** - These terms will also be used synonymously. The game uses the wems for WWEM and WWES (functionally the same file type, that being a wem. WWES
        is used in H3 for dialogue). Wems are generated from wavs using Wwise.
- **LINE** - A file that contains a CRC32 hash of a text string e.g. `UI_LOCATION_PARIS_COUNTRY`. LINE files themselves are used for referencing localised lines in bricks, they are not required if it's, for example, being used in UI.
