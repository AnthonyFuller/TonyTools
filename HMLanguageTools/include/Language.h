#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <unordered_map>

#include "buffer.hpp"

namespace Language
{
    enum class Version : uint8_t
    {
        ALPHA = 0,
        H2016 = 1,
        H2 = 2,
        H3 = 3,
        NONE = 255
    };

    struct Rebuilt
    {
        std::vector<char> file;
        std::string meta;
    };

    namespace DLGE
    {
        // Thanks to Notex for compiling this list
        const std::unordered_map<uint32_t, std::string> TagMap = {
            { 0xE287BCB1, "In-world_AI_Important" },
            { 0x81949AAC, "In-world_AI_Normal" },
            { 0x3521986B, "In-world_Cinematic" },
            { 0x9040D51C, "In-world_Megaphoneprocessed" },
            { 0x78241475, "In-world_Mission_Escort" },
            { 0x51BF68CD, "In-world_Mission_ICAEarpiece" },
            { 0x4F8F383A, "In-world_Mission_Sins_Ambrosia" },
            { 0x11D1A172, "In-world_Mission_Important" },
            { 0x3D3E9B77, "In-world_Mission_Handler" },
            { 0x8C8637E3, "In-world_Mission_Dream" },
            { 0x68936123, "In-world_Mission_Normal" },
            { 0x6090FE6F, "In-world_Mission_ScreenProcessed" },
            { 0x1286BCB5, "In-world_Radioprocessed" },
            { 0xBB4AD21A, "In-world_Telephoneprocessed" },
            { 0xDB12A7BC, "In-world_Maincharacter" },
            { 0x12098283, "Test_tag" },
            { 0xD8F3FE9F, "Voiceover_Cinematic" },
            { 0x5294435C, "Voiceover_Menu" },
            { 0x599CC1AC, "Voiceover_Handler" },
            { 0x3F635E06, "Voiceover_Handler_AsylumPA" },
            { 0x9E7D4200, "Voiceover_Handler_Dream" },
            { 0x435FD483, "Voiceover_Handler_Evergreen" },
            { 0x7F65E780, "Voiceover_Handler_Sins" },
            { 0xF3D9CE4C, "Voiceover_Handler_Grey" },
            { 0xDA43D74B, "Voiceover_Handler_ICAEarpiece" },
            { 0x1454C47D, "Voiceover_Handler_Olivia" },
            { 0x0ACBD31B, "Voiceover_Handler_Memory" },
            { 0xEB78732C, "Voiceover_DevCommentary" },
            { 0xB67FF6F3, "Voiceover_Telephoneprocessed" },
            { 0x04F26CDE, "In-world_AI_CloseUp" },
            { 0x05A2B840, "In-world_AI_HMImportant" },
            { 0xB816293C, "In-world_Mission_UnImportant" },
            { 0xF1FC1D94, "In-world_Speaker_Volumetric" },
            { 0x1A93B081, "In-world_AI_UnImportant" },
            { 0x215A6EBD, "In-world_PA_Processed_Volumetric" },
            { 0xB7EFDE96, "In-world_Crowd_Possession" },
            { 0x00824839, "In-world_Mission_Silent" },
            { 0x8F9C1DA0, "In-world_Mute_LipsyncDummy" },
            { 0xBD1D6D15, "In-world_RobotProcessed" },
            { 0x3D375265, "In-world_Mission_OtherRoom" },
            { 0xD17DAC2B, "In-world_PA_Processed_LongDistance" },
            { 0x479519B7, "In-world_AI_VeryLow" },
            { 0x3893A86C, "In-world_Sniper_Maincharacter" },
            { 0x9C6233D5, "In-world_Speaker_LoFi_Small" },
            { 0x8EADD4BB, "In-world_Speaker_LoFi_Medium" },
            { 0x31A11DF9, "In-world_Speaker_LoFi_Large" },
            { 0x6CBBC2AF, "In-world_Speaker_HiFi_Small" },
            { 0x4BAE1306, "In-world_Speaker_HiFi_Medium" },
            { 0xC178EC83, "In-world_Speaker_HiFi_Large" },
            { 0xBC138F69, "In-world_PA_Processed_GrandHall" },
            { 0x9FB59936, "In-world_PA_Processed_Robot" }
        };

        struct Metadata {
            uint16_t typeIndex; // >> 12 for type -- & 0xFFF for index
            // This is actually a u32 count, then X amount of u32s but our buffer
            // stream reader, when reading a vector, reads a u32 of size first.
            std::vector<uint32_t> SwitchHashes;
        };

        class Container {
        public:
            uint8_t type;
            uint32_t SwitchGroupHash;
            uint32_t DefaultSwitchHash;
            std::vector<Metadata> metadata; 

            Container(buffer& buff) {
                type = buff.read<uint8_t>();
                SwitchGroupHash = buff.read<uint32_t>();
                DefaultSwitchHash = buff.read<uint32_t>();

                uint32_t count = buff.read<uint32_t>();
                for (uint32_t i = 0; i < count; i++) {
                    Metadata data = {
                        buff.read<uint16_t>(),
                        buff.read<std::vector<uint32_t>>()
                    };

                    metadata.push_back(data);
                }
            }
        };

        std::string Convert(Language::Version version, std::vector<char> data, std::string metaJson, std::string langMap = "");
        Rebuilt Rebuild(Language::Version version, std::string jsonString);
    } // namespace DLGE

    namespace LOCR
    {
        std::string Convert(Language::Version version, std::vector<char> data, std::string metaJson, std::string langMap = "");
        Rebuilt Rebuild(Language::Version version, std::string jsonString);
    } // namespace LOCR

    namespace RTLV
    {
        std::string Convert(Version version, std::vector<char> data, std::string metaJson);
        Rebuilt Rebuild(Version version, std::string jsonString);
    } // namespace RTLV
} // namespace Language