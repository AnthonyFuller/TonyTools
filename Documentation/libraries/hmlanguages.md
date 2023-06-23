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

There is a [tool](/tools/hmlanguagetools) which implements this library in full. The [source](https://github.com/AnthonyFuller/TonyTools/blob/master/Tools/HMLanguageTools) can be used as a reference implementation.

This page will go into detail about the formats, naming conventions used by the tool, and more. Technical details, such as structure of the raw files, will be in collapsible sections (but these have yet to be added).

:::warning
ResourceLib is required for this library. For more information, see the [installation](/general/installation#resourcelib) page.
:::

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

:::info Note
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

#### JSON Representation {#clng-json}

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

#### API {#clng-api}

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

#### JSON Representation {#ditl-json}

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

#### API {#ditl-api}

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

Now, this format is **complex**, and that's putting it lightly. This is the longest section for a reason, so strap in, and make sure you know your [definitions](#glossary).

This format, in its simplest terms, defines dialogue. These could include subtitles (optional), [Wavs (WWES/M)](#glossary), and FaceFX animations (FXAS).

Going more in-depth, this format comprises different containers. The 4 container types are: **WavFile**, **Random**, **Switch**, and **Sequence**.

An overview of them can be found in the table below, we go more in-depth in the following section.

{#dlge-container-table}
| Container Name 	| Description                                                                                                                                    	|
|----------------	|------------------------------------------------------------------------------------------------------------------------------------------------	|
| WavFile        	| The "leaf" of the tree. Defines (optional) subtitles, wavs, FaceFX animations, and soundtags.                                                  	|
| Random         	| Contains WavFiles with an added random "weight" (0 to 1). Randomly chooses one of the contained WavFiles.                                     	|
| Switch         	| Works like programming switch cases. Contains Random containers and WavFiles with added "cases" array. Has a "sound group" and "default" case. 	|
| Sequence       	| Can contain all the containers (apart from itself). Goes through the child containers **in-order**.                                            	|

<p style="text-align: center; font-style: italic;">Figure 3: Table of DLGE containers and a brief description of them</p>

A basic diagram (showcasing all containers) can be seen below.

{#dlge-container-tree}

<img v-if="isDark" src="/images/dlge/dark-tree.svg" />
<img v-else src="/images/dlge/light-tree.svg" />

<p style="text-align: center; font-style: italic;">Figure 4: Tree diagram showing how DLGE containers are connected together (read from left to right)</p>

#### Containers

This section will go into more depth regarding the containers including their individual JSON representations before going onto the main format representation.

##### WavFile
*The simplest container, no fancy logic behind this one.*

As said in the [table](#dlge-container-table) above, they are the "leaf" of the DLGE tree. Unless a container is empty (which is allowed), this is what all containers boil down to.

They are very simple containers and can mostly be explained through the JSON representation seen below.
All fields are required unless otherwise specified.

```json
{
    "type": "WavFile",        // important, cannot differ from this
    "wavName": "...",         // CRC32 of the "wav name", retrieved from the
                              //    path, otherwise it is the hash in hex
    "cases": [...],           // array of switch cases, see the Switch section
    "weight": ...,            // a decimal (or hexadecimal string) random weight
                              //    see the Random section
    "soundtag": "...",        // soundtag name (or hash in hex)
    "defaultWav": "...",      // the path of the "default" wav (usually english)
    "defaultFfx": "...",      // like defaultWav, but for the FaceFX animation
    "languages": {            // this can be empty or be missing languages
                              //   (i.e. xx)
        "en": "...",          // subtitle for this language
        ...
        "jp": {               // this is optional, IOI use this in H2016
            "wav": "...",     // required, the path of the wav for thislanguage
            "ffx": "...",     // required, the path of the FaceFX animation for
                              //   this language
            "subtitle": "..." // optional, subtitle for this language
        }
    }
}
```

:::info Note
The `cases` and `weight` properties are only required if the WavFile itself is a child of their respective containers ([Switch](#switch) and [Random](#random)).
Otherwise, they will be ignored. A WavFile can never actually have the two.
:::

##### Random
*Not actually that Random.*

The second-simplest container, they only ever contain WavFiles with the added `weight` property as shown above and one is picked, at random, with respect to this weight.

The only extra thing you really need to know about how [weighting](https://en.wikipedia.org/wiki/Weighting) works in relation to randomness.
If a WavFile has a higher weight, it means that it is more likely to be chosen when the Random container is invoked.
These weight values should only ever add up to `1`, the library currently does not check this for you.

HMLanguages gives you two options for representation of the weight value:
1. Decimal, this means it is a number between `0` and `1` (inclusive).
2. Hexadecimal string, a hex number between `000000` and `FFFFFF` (inclusive).

The default is decimal, and during convert, a bool needs to be set. When rebuilding, the tool can easily distinguish the two, so the bool is not required.

To convert between these two representations, you can just divide the hex weight value by `FFFFFF` this obviously implies that `000000` is 0, and `FFFFFF` is 1.

Converting weights as hex allows for more precision, but decimal is easier for most people to read. For example, for something that will occur with a 1 in 4 chance, `0.24999995529651375` (`3FFFFF`) will be output instead of `0.25` as it doesn't perfectly divide since `FFFFFF` is an odd number.

This container's JSON representation can be seen below with both decimal and hexadecimal weights.

```json
{
    "type": "Random",
    "cases": [...],             // this only occurs when it is
                                //   a child of a Switch container
    "containers": [
        {
            "type": "WavFile",
            ...
            "weight": 0.25,
            ...
        },
        {
            "type": "WavFile",
            ...
            "weight": "3FFFFF", // this is a 1 in 4 chance
            ...
        }
        {
            "type": "WavFile",
            ...
            "weight": 0.5,
            ...
        }
    ]
}
```

:::info IOI Moment
Sometimes IOI just use these containers in a Switch container with a single WavFile inside and give it a weight of 1 (and sometimes they're empty).

It's unclear as to why they do this, but HMLanguages allows WavFiles to be referenced inside Switch containers.
:::

##### Switch
*No, you can't turn it off and on again.*

Switch containers, as previously mentioned, work like switch cases from many programming languages with a slight difference.

First, Hitman uses a "switch key" to reference switches, the game will also pass a "case value" to this.

There are a few standard ones for these built into the engine itself, but we can also create our own.
An exhaustive list (of the switch keys at least) can be seen below of the switches used in the game (as of June 2023):
:::details Switches
- `AI_NPC_ID` - `DIALOGUE_NPC_*` i.e. `DIALOGUE_NPC_DAHLIA`
    - Used for body identification lines.
- `AI_HMLastKnownDisguise` - `DIALOGUE_BADDSG_*` i.e. `DIALOGUE_BADDSG_GOTY_CLOWN`
    - A bit obvious but used for when an NPC is reporting what HM is wearing to a guard.
- `AI_Sentry_ItemRequest` - `INVITE`, `TICKET`, `VIPTICKET`
    - Used for when asking HM for an item (this list is exhaustive).
- `AI_HMArm` - `DIALOGUE_*` i.e. `DIALOGUE_SNIPER`
    - Used for when an NPC is reporting what illegal item HM was using to a guard.
- `AI_HMLastKnownGtag` - loads of variations i.e. `Dungeon`, `Laundry`
    - Used for when an NPC is reporting where HM was to a guard.
- `AI_PQ` - `AI_PQ47EntAck` and `AI_PQNPCEntAck`
    - Used for when HM or an NPC is detected in a "private group".
- There are a few Evergreen ones which are added differently to these as they aren't in-engine, we will go into these now.
:::

It is possible to add custom switch keys and groups to the game via use of [WSWB](#glossary) (aka [DSWB](#glossary)) and [WSWT](#glossary). The WSWT file is empty, and used when invoking them from an entity.
The WSWB contains all the switch cases and the key itself. These are mostly used when the game needs to change what is said depending on other variables i.e. tells, looks, location, etc.

You can create these manually using [ResourceTool](https://github.com/OrfeasZ/ZHMTools) or using [G2WwiseDataTool](https://github.com/glacier-modding/G2WwiseDataTool) when integrated into a Wwise project.

Switch containers also contain a `default` property, which points to one of the cases (it sometimes doesn't even exist in any of the cases, we call that an "IOI moment"), normally this is a fallback line and will be used if a Switch container gets passed a case it does not have explicit handling for.

This container's JSON representation can be seen below:

```json
{
    "type": "Switch",
    "switchKey": "AI_Sentry_ItemRequest",
    "default": "INVITE",
    "containers": [
        {
            "type": "Random",
            "cases": [
                "INVITE"        // although this is an array, it's perfectly
                                //   fine (and normal) to just have one
            ],
            ...
        },
        {
            "type": "WavFile",
            "cases": [
                "TICKET",       // but you can have more than one
                "VIPTICKET"
            ],
            ...
        }
    ]
}
```

##### Sequence
*Don't tell me what to do!*

Sequence containers are the one container that can house all other containers.

They're actually quite simple. Whatever order the containers are in, is the order that they will be played out.

If used, these will **always** be the root container.

You can see the JSON representation below:

```json
{
    "type": "Sequence",
    "containers": [
        {
            "type": "Random",
            ...
        },
        {
            "type": "Switch",
            ...
        },
        {
            "type": "WavFile",
            ...
        }
    ]
}
```

And that's that! You now know about all the individual containers in the DLGE format. Now onto the main format representation and then the API!

#### JSON Representation {#dlge-json}

This will be *heavily* simplified as we go into more depth in the previous sections.

This layout is meant to mimic the one you saw right at the start of this section with the [tree](#dlge-container-tree).

[Schema](/schemas/dlge.schema.json)
```json
{
    "hash": "...",
    "DITL": "...",      // the hash where soundtags will be resolved
    "CLNG": "...",      // the hash for the CLNG file, the default one
                        //   in chunk0 should really be used
    "rootContainer": {
        "type": "Sequence",
        "containers": [
            {
                "type": "Random",
                ...
            },
            {
                "type": "Switch",
                ...
            },
            {
                "type": "WavFile",
                ...
            }
        ]
    }
}
```

#### API {#dlge-api}

```cpp
// DLGE + meta.json -> DLGE
std::string json = TonyTools::Language::DLGE::Convert(
    Language::Version version,              // game version
    std::vector<char> data,                 // raw DLGE data
    std::string metaJson,                   // .meta.json string
    std::string defaultLocale = "en",       // optional default locale
    bool hexPrecision = false,              // should random weights be
                                            //   output as hex?
    std::string langMap = ""                // optional language map
                                            //   (must be exact!)
);

// JSON -> DLGE + meta.json
TonyTools::Language::Rebuilt rebuild =
    TonyTools::Langauge::DLGE::Rebuild(
        Language::Version version,          // game version
        std::string jsonString,             // HML JSON string
        std::string defaultLocale = "en",   // optional default locale
        std::string langMap = ""            // optional language map
                                            //   (must be exact!)
    );
```

And we're all done, take a breather, go get a drink, and revel in the fact that you know all about a specific file type from a niche game!

---

### LOCR
*aka **LocalizedTextResource***

The classic (and possibly the most popular) localization format from the World of Assassination trilogy.

This format defines a [LINE](#glossary) hash to a string per language.
Currently, there is no hash list for these (one is planned for far in the future), so they are all output as CRC32 hashes in hexadecimal.
A language map **is required** for convert as per the [table](#language-maps) whether that be from the version or user supplied.

The JSON format allows for users to put the plain-text version e.g. `UI_LOCATION_PARIS_COUNTRY` instead of the hash, these will be hashed upon rebuilding.

:::info Note
For early H2016 LOCR files, the tool may fail to convert by default. This is due to a different cipher being used. You can fix this by passing `true` for the final parameter ([see below](#locr-api)).
:::

#### JSON Representation {#locr-json}

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

#### API {#locr-api}

```cpp
// LOCR + meta.json -> JSON
std::string json = TonyTools::Language::LOCR::Convert(
    Language::Version version,      // game version
    std::vector<char> data,         // raw LOCR data
    std::string metaJson,           // .meta.json string
    std::string langMap = "",       // optional language map
    bool symmetric = false          // whether a symmetric cipher should
                                    //   be used 
);

// JSON -> LOCR + meta.json
TonyTools::Language::Rebuilt rebuild =
    TonyTools::Langauge::LOCR::Rebuild(
        Language::Version version,  // game version
        std::string jsonString,     // HML JSON string
        bool symmetric = false      // whether a symmetric cipher should
                                    //   be used
    );
```

---

### RTLV
*aka **RuntimeLocalizedVideo***

This format defines videos (for different languages, `en` is the default for all languages unless explicitly defined otherwise) and subtitles per language.

#### JSON Representation {#rtlv-json}

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

#### API {#rtlv-api}

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
    - **WSWT** - Wwise Audio Switch Template, empty file used for the template reference of switch cases.
    - **WSWB/DSWB** - Wwise Audio Switch Blueprint, used for referencing switch cases as events inside entities and contains the actual switch data, explained in more depth in the [Switch section](#switch) of DLGE.
- **LINE** - A file that contains a CRC32 hash of a text string e.g. `UI_LOCATION_PARIS_COUNTRY`. LINE files themselves are used for referencing localised lines in bricks, they are not required if it's, for example, being used in UI.
