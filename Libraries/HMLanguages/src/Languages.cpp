#include "TonyTools/Languages.h"

#include <iostream>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <regex>
#include <unordered_map>

#include <ResourceLib_HM2016.h>
#include <ResourceLib_HM2.h>
#include <ResourceLib_HM3.h>
#include <nlohmann/json.hpp>
#include <hash/md5.h>
#include <hash/crc32.h>
#include <tsl/ordered_map.h>

#include "zip.hpp"
#include "bimap.hpp"
#include "buffer.hpp"

using namespace TonyTools;
using json = nlohmann::ordered_json;

#pragma region Utility Functions
bool is_valid_hash(std::string hash)
{
    const std::string valid_chars = "0123456789ABCDEF";

    if (hash.length() != 16)
        return false;

    return std::all_of(hash.begin(), hash.end(), ::isxdigit);
}

std::string computeHash(std::string str)
{
    MD5 md5;
    str = "00" + md5(str).substr(2, 14);
    std::transform(str.begin(), str.end(), str.begin(), ::toupper);

    return str;
}

std::string generateMeta(std::string hash, uint32_t size, std::string type, tsl::ordered_map<std::string, std::string> depends)
{
    json j = {
        {"hash_value", is_valid_hash(hash) ? hash : computeHash(hash)},
        {"hash_offset", 0x10000000},
        {"hash_size", 0x80000000 + size},
        {"hash_resource_type", type},
        {"hash_reference_table_size", (0x9 * depends.size()) + 4},
        {"hash_reference_table_dummy", 0},
        {"hash_size_final", size},
        {"hash_size_in_memory", ULONG_MAX},
        {"hash_size_in_video_memory", ULONG_MAX},
        {"hash_reference_data", json::array()}
    };

    for (const auto &[hash, flag] : depends)
        j.at("hash_reference_data").push_back({{"hash", hash}, {"flag", flag}});

    return j.dump();
}

ResourceConverter* getConverter(Language::Version version, const char *resourceType)
{
    switch (version)
    {
    case Language::Version::H2016:
    {
        if (!HM2016_IsResourceTypeSupported(resourceType))
        {
            fprintf(stderr, "[LANG] %s for H2016 is not supported in this version of ResourceLib!\n", resourceType);
            return nullptr;
        }

        return HM2016_GetConverterForResource(resourceType);
    };
    case Language::Version::H2:
    {
        if (!HM2_IsResourceTypeSupported(resourceType))
        {
            fprintf(stderr, "[LANG] %s for H2 is not supported in this version of ResourceLib!\n", resourceType);
            return nullptr;
        }

        return HM2_GetConverterForResource(resourceType);
    };
    case Language::Version::H3:
    {
        if (!HM3_IsResourceTypeSupported(resourceType))
        {
            fprintf(stderr, "[LANG] %s for H3 is not supported in this version of ResourceLib!\n", resourceType);
            return nullptr;
        }

        return HM3_GetConverterForResource(resourceType);
    };
    default:
        return nullptr;
    }
}

ResourceGenerator* getGenerator(Language::Version version, const char *resourceType)
{
    switch (version)
    {
    case Language::Version::H2016:
    {
        if (!HM2016_IsResourceTypeSupported(resourceType))
        {
            fprintf(stderr, "[LANG] %s for H2016 is not supported in this version of ResourceLib!\n", resourceType);
            return nullptr;
        }

        return HM2016_GetGeneratorForResource(resourceType);
    };
    case Language::Version::H2:
    {
        if (!HM2_IsResourceTypeSupported(resourceType))
        {
            fprintf(stderr, "[LANG] %s for H2 is not supported in this version of ResourceLib!\n", resourceType);
            return nullptr;
        }

        return HM2_GetGeneratorForResource(resourceType);
    };
    case Language::Version::H3:
    {
        if (!HM3_IsResourceTypeSupported(resourceType))
        {
            fprintf(stderr, "[LANG] %s for H3 is not supported in this version of ResourceLib!\n", resourceType);
            return nullptr;
        }

        return HM3_GetGeneratorForResource(resourceType);
    };
    default:
        return nullptr;
    }
}

std::vector<std::string> split(std::string str)
{
    std::regex regex{R"([,]+)"};
    std::sregex_token_iterator it{str.begin(), str.end(), regex, -1};
    return std::vector<std::string>{it, {}};
}

// From https://github.com/glacier-modding/RPKG-Tool/blob/145d8d7d9711d57f1434489706c3d81b2feeed73/src/crypto.cpp#L3-L41
constexpr uint32_t xteaKeys[4] = {0x53527737, 0x7506499E, 0xBD39AEE3, 0xA59E7268};
constexpr uint32_t xteaDelta = 0x9E3779B9;
constexpr uint32_t xteaRounds = 32;

std::string xteaDecrypt(std::vector<char> data)
{
    for (uint32_t i = 0; i < data.size() / 8; i++)
    {
        uint32_t* v0 = (uint32_t*)(data.data() + (i * 8));
        uint32_t* v1 = (uint32_t*)(data.data() + (i * 8) + 4);

        uint32_t sum = xteaDelta * xteaRounds;

        for (uint32_t j = 0; j < xteaRounds; j++)
        {
            *v1 -= (*v0 << 4 ^ *v0 >> 5) + *v0 ^ sum + xteaKeys[sum >> 11 & 3];
            sum -= xteaDelta;
            *v0 -= (*v1 << 4 ^ *v1 >> 5) + *v1 ^ sum + xteaKeys[sum & 3];
        }
    }

    return std::string(data.begin(), std::find(data.begin(), data.end(), '\0'));
}

std::vector<char> xteaEncrypt(std::string str)
{
    std::vector<char> data(str.begin(), str.end());
    uint32_t paddedSize = data.size() + (data.size() % 8 == 0 ? 0 : 8 - (data.size() % 8));
    data.resize(paddedSize, '\0');

    for (uint32_t i = 0; i < paddedSize / 8; i++)
    {
        uint32_t* v0 = (uint32_t*)(data.data() + (i * 8));
        uint32_t* v1 = (uint32_t*)(data.data() + (i * 8) + 4);

        uint32_t sum = 0;

        for (uint32_t j = 0; j < xteaRounds; j++)
        {
            *v0 += (((*v1 << 4) ^ (*v1 >> 5)) + *v1) ^ (sum + xteaKeys[sum & 3]);
            sum += xteaDelta;
            *v1 += (((*v0 << 4) ^ (*v0 >> 5)) + *v0) ^ (sum + xteaKeys[(sum >> 11) & 3]);
        }
    }

    return data;
}

std::vector<char> symmetricEncrypt(std::string str)
{
    std::vector<char> data(str.begin(), str.end());

    for (char &value : data)
    {
        value ^= 226;
        value = (value & 0x81) | (value & 2) << 1 | (value & 4) << 2 | (value & 8) << 3 | (value & 0x10) >> 3 |
           (value & 0x20) >> 2 | (value & 0x40) >> 1;
    }

    return data;
}

std::string symmetricDecrypt(std::vector<char> data)
{
    for (char &value : data)
    {
        value = (value & 1) | (value & 2) << 3 | (value & 4) >> 1 | (value & 8) << 2 | (value & 16) >> 2 | (value & 32) << 1 |
            (value & 64) >> 3 | (value & 128);
        value ^= 226;
    }
    
    return std::string(data.begin(), data.end());
}

std::string getWavName(std::string path, std::string ffxPath, std::string hash)
{
    if (is_valid_hash(path))
        return hash;

    std::regex r{R"([^\/]*(?=\.wav))"};
    std::regex ffxR{R"([^\/]*(?=\.animset))"};
    std::smatch m;
    std::regex_search(path, m, r);

    if (m.size() != 1)
    {
        std::regex_search(ffxPath, m, ffxR);
        if (m.size() != 1)
            return hash;
    }

    CRC32 crc32;

    std::string hashedName = std::format("{:08X}", crc32(m[0]));

    return hashedName == hash ? m[0] : hash;
}

uint32_t hexStringToNum(std::string string)
{
    CRC32 crc32;

    uint32_t num = std::strtoul(string.c_str(), nullptr, 16);
    if (!std::all_of(string.begin(), string.end(), ::isxdigit))
        return crc32(string);

    return num;
}
#pragma endregion

#pragma region Switch and Tag Maps
// Thanks to Notex for compiling this list
const stde::bimap<uint32_t, std::string> TagMap = {
    {0xB1BC87E2, "In-world_AI_Important"},
    {0xAC9A9481, "In-world_AI_Normal"},
    {0x6B982135, "In-world_Cinematic"},
    {0x1CD54090, "In-world_Megaphoneprocessed"},
    {0x75142478, "In-world_Mission_Escort"},
    {0xCD68BF51, "In-world_Mission_ICAEarpiece"},
    {0x3A388F4F, "In-world_Mission_Sins_Ambrosia"},
    {0x72A1D111, "In-world_Mission_Important"},
    {0x779B3E3D, "In-world_Mission_Handler"},
    {0xE337868C, "In-world_Mission_Dream"},
    {0x23619368, "In-world_Mission_Normal"},
    {0x6FFE9060, "In-world_Mission_ScreenProcessed"},
    {0xB5BC8612, "In-world_Radioprocessed"},
    {0x1AD24ABB, "In-world_Telephoneprocessed"},
    {0xBCA712DB, "In-world_Maincharacter"},
    {0x83820912, "Test_tag"},
    {0x9FFEF3D8, "Voiceover_Cinematic"},
    {0x5C439452, "Voiceover_Menu"},
    {0xACC19C59, "Voiceover_Handler"},
    {0x065E633F, "Voiceover_Handler_AsylumPA"},
    {0x00427D9E, "Voiceover_Handler_Dream"},
    {0x83D45F43, "Voiceover_Handler_Evergreen"},
    {0x80E7657F, "Voiceover_Handler_Sins"},
    {0x4CCED9F3, "Voiceover_Handler_Grey"},
    {0x4BD743DA, "Voiceover_Handler_ICAEarpiece"},
    {0x7DC45414, "Voiceover_Handler_Olivia"},
    {0x1BD3CB0A, "Voiceover_Handler_Memory"},
    {0x2C7378EB, "Voiceover_DevCommentary"},
    {0xF3F67FB6, "Voiceover_Telephoneprocessed"},
    {0xDE6CF204, "In-world_AI_CloseUp"},
    {0x40B8A205, "In-world_AI_HMImportant"},
    {0x3C2916B8, "In-world_Mission_UnImportant"},
    {0x941DFCF1, "In-world_Speaker_Volumetric"},
    {0x81B0931A, "In-world_AI_UnImportant"},
    {0xBD6E5A21, "In-world_PA_Processed_Volumetric"},
    {0x96DEEFB7, "In-world_Crowd_Possession"},
    {0x39488200, "In-world_Mission_Silent"},
    {0xA01D9C8F, "In-world_Mute_LipsyncDummy"},
    {0x156D1DBD, "In-world_RobotProcessed"},
    {0x6552373D, "In-world_Mission_OtherRoom"},
    {0x2BAC7DD1, "In-world_PA_Processed_LongDistance"},
    {0xB7199547, "In-world_AI_VeryLow"},
    {0x6CA89338, "In-world_Sniper_Maincharacter"},
    {0xD533629C, "In-world_Speaker_LoFi_Small"},
    {0xBBD4AD8E, "In-world_Speaker_LoFi_Medium"},
    {0xF91DA131, "In-world_Speaker_LoFi_Large"},
    {0xAFC2BB6C, "In-world_Speaker_HiFi_Small"},
    {0x0613AE4B, "In-world_Speaker_HiFi_Medium"},
    {0x83EC78C1, "In-world_Speaker_HiFi_Large"},
    {0x698F13BC, "In-world_PA_Processed_GrandHall"},
    {0x3699B59F, "In-world_PA_Processed_Robot"}
};

const stde::bimap<uint32_t, std::string> SwitchMap = {
    {0x7FA5083E, "AI_NPC_ID"},
    {0x3F9FBA5E, "DIALOGUE_NPC_MENDOLA"},
    {0x45B00D96, "DIALOGUE_NPC_INGRAM"},
    {0x1DDAFC58, "DIALOGUE_NPC_SIERRA_KNOX"},
    {0xA4F1E060, "DIALOGUE_NPC_KRUGER"},
    {0xB5490FD1, "DIALOGUE_NPC_VETROVA"},
    {0xBA19F183, "DIALOGUE_NPC_WASHINGTON_Z"},
    {0x501D17B4, "DIALOGUE_NPC_RANGAN"},
    {0xEE62A339, "DIALOGUE_NPC_LAFAYETTE"},
    {0x5C2AFC66, "DIALOGUE_NPC_DOCTOR"},
    {0x31C46FEC, "DIALOGUE_NPC_RITTER"},
    {0x7AE92254, "DIALOGUE_NPC_WILLIAMS"},
    {0xFD99C3A6, "DIALOGUE_NPC_FRANCO"},
    {0x47E3A9BC, "DIALOGUE_NPC_KNIGHT"},
    {0xA2E1F9E3, "DIALOGUE_NPC_NOVIKOV"},
    {0x67E54025, "DIALOGUE_NPC_CARLISLE_EM"},
    {0xA63B9631, "DIALOGUE_NPC_DESANTIS"},
    {0xBDC11BD5, "DIALOGUE_NPC_REYNARD"},
    {0x0762F1BC, "DIALOGUE_NPC_MORGAN"},
    {0xAFD00BC3, "DIALOGUE_NPC_ROSE"},
    {0xA3F96F5F, "DIALOGUE_NPC_FALLBACK"},
    {0xE3F46D9E, "DIALOGUE_NPC_STRANDBERG"},
    {0xC3C54927, "DIALOGUE_NPC_WASHINGTON_S"},
    {0x5D4BD049, "DIALOGUE_NPC_YATESVALENTINA"},
    {0x588D59C7, "DIALOGUE_NPC_KONNY"},
    {0xB4B31C37, "DIALOGUE_NPC_CARLISLE_R"},
    {0xB245D15C, "DIALOGUE_NPC_VARGAS"},
    {0xB2663E75, "DIALOGUE_NPC_SATO"},
    {0x77688BE9, "DIALOGUE_NPC_MARTINEZ"},
    {0x394C7EDC, "DIALOGUE_NPC_CORNELIA_S"},
    {0x7EB7D1EC, "DIALOGUE_NPC_CASSIDY"},
    {0x8DBB3BC3, "DIALOGUE_NPC_ROYCE"},
    {0xF30A3ADC, "DIALOGUE_NPC_WAITER"},
    {0x598AC0C5, "DIALOGUE_NPC_CONSTANT"},
    {0x300D5DE9, "DIALOGUE_NPC_CARLISLE_A"},
    {0x47066204, "DIALOGUE_NPC_CREST"},
    {0x751AA978, "DIALOGUE_NPC_MAELSTROM"},
    {0x024428DA, "DIALOGUE_NPC_VIDAL"},
    {0xCCD125C0, "DIALOGUE_NPC_JANUS"},
    {0xC7ABC9D1, "DIALOGUE_NPC_CHEF"},
    {0xD4F57271, "DIALOGUE_NPC_CROSS"},
    {0x66AB1902, "DIALOGUE_NPC_GRAVES"},
    {0x93A31EE6, "DIALOGUE_NPC_KONG"},
    {0x7B59DDB0, "DIALOGUE_NPC_PARVATI"},
    {0x1466764D, "DIALOGUE_NPC_SHAH"},
    {0x95BCF203, "DIALOGUE_NPC_BRADLEY"},
    {0x920919F6, "DIALOGUE_NPC_CARUSO"},
    {0x0019C089, "DIALOGUE_NPC_BOSCO"},
    {0x73C644A6, "DIALOGUE_NPC_ZAYDAN"},
    {0x5C1FC148, "DIALOGUE_NPC_CLEANER"},
    {0xFF4973B1, "DIALOGUE_NPC_BODYGUARD"},
    {0xFBDC8012, "DIALOGUE_NPC_YAMASAKI"},
    {0x8148F4E0, "DIALOGUE_NPC_ORSON"},
    {0xE2809BCD, "DIALOGUE_NPC_STUYVESANT"},
    {0xD96EF8DC, "DIALOGUE_NPC_CARLISLE_G"},
    {0x5ABD7D1B, "DIALOGUE_NPC_CARLISLE_P"},
    {0x2567B553, "DIALOGUE_NPC_SHEIK"},
    {0xB0E2FC65, "DIALOGUE_NPC_YATES"},
    {0x744561F0, "DIALOGUE_NPC_FALCONE"},
    {0xFF74E623, "DIALOGUE_NPC_HUSH"},
    {0xCDF92C6B, "DIALOGUE_NPC_LEE"},
    {0xB0F082F6, "DIALOGUE_NPC_SAVALAS"},
    {0xE9E772F7, "DIALOGUE_NPC_DASILVA"},
    {0xF4CAD196, "DIALOGUE_NPC_DAHLIA"},
    {0x904E950C, "DIALOGUE_NPC_ROBERT_KNOX"},
    {0xE78D8906, "DIALOGUE_NPC_DELGADO"},
    {0xFC77904A, "DIALOGUE_NPC_ABIATTI"},
    {0x054B89E7, "DIALOGUE_NPC_BERG"},
    {0x6F03072E, "DIALOGUE_NPC_AKKA"},
    {0xB8D68C6C, "DIALOGUE_NPC_FARAH"},
    {0x4020D8B3, "DIALOGUE_NPC_NORFOLK"},
    {0xF00F5A9C, "DIALOGUE_NPC_SANTORO"},
    {0x1E39F881, "DIALOGUE_NPC_CARLISLE_ED"},
    {0xCB72A035, "AI_HMLastKnownDisguise"},
    {0x6B7C8C59, "DIALOGUE_BADDSG_FIXER"},
    {0xB4B1150F, "DIALOGUE_BADDSG_KTCHSTFF"},
    {0x89123A06, "DIALOGUE_BADDSG_GOTY_CLOWN"},
    {0x31F71D72, "DIALOGUE_BADDSG_BANKTELLER"},
    {0xBE7AE4EB, "DIALOGUE_BADDSG_47SUBURBIA"},
    {0x27CCF6A8, "DIALOGUE_BADDSG_GRDNR"},
    {0x69EAB33D, "DIALOGUE_BADDSG_CLUBSTAFF"},
    {0x548FC30F, "DIALOGUE_BADDSG_DJ"},
    {0x56DFFDFC, "DIALOGUE_BADDSG_LIFEGRD"},
    {0x8845DC11, "DIALOGUE_BADDSG_DELUXEDEVIL"},
    {0xEF0A7E3B, "DIALOGUE_BADDSG_FALLBACK"},
    {0xE60F6A0B, "DIALOGUE_BADDSG_MAMBACREW"},
    {0x56C22F12, "DIALOGUE_BADDSG_PARKA"},
    {0xF2CECB98, "DIALOGUE_BADDSG_TERMINUS"},
    {0x430F04A8, "DIALOGUE_BADDSG_STAFF"},
    {0xE4E4A6D3, "DIALOGUE_BADDSG_CLUBOWNER"},
    {0x14EE5CB6, "DIALOGUE_BADDSG_CADDIE"},
    {0x26AF33F3, "DIALOGUE_BADDSG_MOVIEMNSTR"},
    {0xC67F7688, "DIALOGUE_BADDSG_CAMERA"},
    {0xF62509F7, "DIALOGUE_BADDSG_TUX"},
    {0x26E60F7E, "DIALOGUE_BADDSG_CQGUARD"},
    {0xD06A3B5B, "DIALOGUE_BADDSG_MASTER"},
    {0x5D0AB2D7, "DIALOGUE_BADDSG_MECHUS"},
    {0x241B1D63, "DIALOGUE_BADDSG_TATOO"},
    {0x41651500, "DIALOGUE_BADDSG_47COLORADO"},
    {0x82F69EDB, "DIALOGUE_BADDSG_POLITICASST"},
    {0xAE9B6A00, "DIALOGUE_BADDSG_SITEWKR"},
    {0xFBF22B39, "DIALOGUE_BADDSG_LNDRYGRD"},
    {0xB83536E7, "DIALOGUE_BADDSG_47HOKKAIDO"},
    {0x6EBE1596, "DIALOGUE_BADDSG_FOOD"},
    {0xA7E02421, "DIALOGUE_BADDSG_SHEIK"},
    {0x3EADD179, "DIALOGUE_BADDSG_SPECOPS"},
    {0x6A846DE6, "DIALOGUE_BADDSG_XTERMINATOR"},
    {0x62D127EF, "DIALOGUE_BADDSG_WORKER"},
    {0xC7C4193A, "DIALOGUE_BADDSG_HACKER"},
    {0x262531E6, "DIALOGUE_BADDSG_RACEMARSH"},
    {0x71D6C06A, "DIALOGUE_BADDSG_ARKIAN"},
    {0xCD10183E, "DIALOGUE_BADDSG_CQENGNR"},
    {0xE0E2716D, "DIALOGUE_BADDSG_KRNSTDTENGNR"},
    {0xA8243F5A, "DIALOGUE_BADDSG_DRIVERSA"},
    {0x78BFC7E6, "DIALOGUE_BADDSG_RESORTSTAFF"},
    {0xDE82F756, "DIALOGUE_BADDSG_FLRDAMAN"},
    {0x7998936C, "DIALOGUE_BADDSG_MASSEUR"},
    {0x03CF6592, "DIALOGUE_BADDSG_ORSON"},
    {0xDA21E292, "DIALOGUE_BADDSG_NORFOLK"},
    {0xC310DE7F, "DIALOGUE_BADDSG_RESIDENT"},
    {0x0B31D230, "DIALOGUE_BADDSG_SCARECROW"},
    {0x25197A29, "DIALOGUE_BADDSG_MECHIT"},
    {0xAD0425F8, "DIALOGUE_BADDSG_GOTY_DARKSNIPER"},
    {0xDA8FC311, "DIALOGUE_BADDSG_MASCOT"},
    {0xE4CE03D0, "DIALOGUE_BADDSG_MILITIASEC"},
    {0x009768E0, "DIALOGUE_BADDSG_MORGUE"},
    {0x17AB0060, "DIALOGUE_BADDSG_WISEMAN"},
    {0xAE768630, "DIALOGUE_BADDSG_47MUMBAI"},
    {0x96CAB621, "DIALOGUE_BADDSG_CLOTHSALE"},
    {0x48CE7A87, "DIALOGUE_BADDSG_CHEF"},
    {0x7D2CA461, "DIALOGUE_BADDSG_SHAMAN"},
    {0x81F5574C, "DIALOGUE_BADDSG_SNOWTREK"},
    {0x14AC3731, "DIALOGUE_BADDSG_VAMPIRE"},
    {0x1D210A12, "DIALOGUE_BADDSG_RANGANSEC"},
    {0x05FC8800, "DIALOGUE_BADDSG_METALWKR"},
    {0x11826D4B, "DIALOGUE_BADDSG_PSYCH"},
    {0x282F90C2, "DIALOGUE_BADDSG_ARCHITECT"},
    {0x804301E8, "DIALOGUE_BADDSG_PRIEST_VP"},
    {0xF97488C8, "DIALOGUE_BADDSG_47ISLAND"},
    {0xEC05E454, "DIALOGUE_BADDSG_DRIVER"},
    {0x4C7F5024, "DIALOGUE_BADDSG_DANCER"},
    {0xC3205FB7, "DIALOGUE_BADDSG_CHURCHSTAFF"},
    {0xF96A3E3D, "DIALOGUE_BADDSG_KNIGHT"},
    {0x18D591CF, "DIALOGUE_BADDSG_SOLDIER"},
    {0x196E9F58, "DIALOGUE_BADDSG_MECHKRNSTDT"},
    {0xCDD485B7, "DIALOGUE_BADDSG_LEE"},
    {0x7C947C78, "DIALOGUE_BADDSG_SOUNDCREW"},
    {0x08B6CADF, "DIALOGUE_BADDSG_SUBWKR"},
    {0xEE732799, "DIALOGUE_BADDSG_MUMBSEC"},
    {0x4514A1C3, "DIALOGUE_BADDSG_ACTOR"},
    {0x2D09C1F6, "DIALOGUE_BADDSG_PIRATE"},
    {0x5325237F, "DIALOGUE_BADDSG_SUIT"},
    {0x05029869, "DIALOGUE_BADDSG_FILMCREW"},
    {0xA785EA1D, "DIALOGUE_BADDSG_BLAKE"},
    {0x9B5BE080, "DIALOGUE_BADDSG_GAUCHO"},
    {0x2B7968E8, "DIALOGUE_BADDSG_DELUXEHUNTING"},
    {0x18D0D9F4, "DIALOGUE_BADDSG_SNORKEL"},
    {0x0042F434, "DIALOGUE_BADDSG_CQHOMELESS"},
    {0x693D5B80, "DIALOGUE_BADDSG_JOURNAL"},
    {0x55A9E95F, "DIALOGUE_BADDSG_INVESTOR"},
    {0xEF62455E, "DIALOGUE_BADDSG_MUSICIAN"},
    {0x331F30F9, "DIALOGUE_BADDSG_47COLUMBIA"},
    {0x50ED4610, "DIALOGUE_BADDSG_47MARRAKESH"},
    {0x23CDC5B1, "DIALOGUE_BADDSG_TAILOR"},
    {0x9F47FE60, "DIALOGUE_BADDSG_HEADMASTER"},
    {0x9564399B, "DIALOGUE_BADDSG_MENDEZ"},
    {0x42439E5B, "DIALOGUE_BADDSG_TEASERV"},
    {0x288D7DDC, "DIALOGUE_BADDSG_47STARTOUTFIT"},
    {0xCCC9CD1C, "DIALOGUE_BADDSG_HAVSTAFF"},
    {0x009BF3E5, "DIALOGUE_BADDSG_TRAINSERV"},
    {0x6F0E8124, "DIALOGUE_BADDSG_MAILMAN"},
    {0x5A552E05, "DIALOGUE_BADDSG_DEADJANUS"},
    {0xB6A60D88, "DIALOGUE_BADDSG_CLUBTECH"},
    {0x25D6C520, "DIALOGUE_BADDSG_PRINTER"},
    {0xAF24DA00, "DIALOGUE_BADDSG_MECHANIC"},
    {0xC083D789, "DIALOGUE_BADDSG_DIRECTOR"},
    {0x9C8CECFA, "DIALOGUE_BADDSG_LAWYER"},
    {0x8543371A, "DIALOGUE_BADDSG_MEDIC"},
    {0x4E8739BF, "DIALOGUE_BADDSG_YACHTCREW"},
    {0x06B8B48D, "DIALOGUE_BADDSG_CAVEGD"},
    {0x9DE9108B, "DIALOGUE_BADDSG_RANGANGRD"},
    {0xDE772CD8, "DIALOGUE_BADDSG_YOGA"},
    {0x02D16DB8, "DIALOGUE_BADDSG_MAINTENANCE"},
    {0x6135E796, "DIALOGUE_BADDSG_WINEMKR"},
    {0x196A7C7B, "DIALOGUE_BADDSG_FIELDGRD"},
    {0xF8A4E4A8, "DIALOGUE_BADDSG_GOTY_COWBOY"},
    {0x6956CA74, "DIALOGUE_BADDSG_BDYGRD"},
    {0xCE6B7FB4, "DIALOGUE_BADDSG_CASTAWAY"},
    {0x63571DAC, "DIALOGUE_BADDSG_HAZMAT"},
    {0x0DC7E994, "DIALOGUE_BADDSG_DRIVERUS"},
    {0x64C96413, "DIALOGUE_BADDSG_KGB"},
    {0x92D1EA07, "DIALOGUE_BADDSG_BBQ"},
    {0x3FFBF94F, "DIALOGUE_BADDSG_ARKPTRN"},
    {0x3BFF6492, "DIALOGUE_BADDSG_DRUGLABWKR"},
    {0xA781C628, "DIALOGUE_BADDSG_VILLAGEGD"},
    {0x0232679B, "DIALOGUE_BADDSG_GHOST"},
    {0x1284FD2B, "DIALOGUE_BADDSG_BARTENDER"},
    {0xE4E12796, "DIALOGUE_BADDSG_POLITICIAN"},
    {0xC9040148, "DIALOGUE_BADDSG_SANTA"},
    {0xD28D3ED3, "DIALOGUE_BADDSG_EVENTSTFF"},
    {0x8CA34255, "DIALOGUE_BADDSG_REALSTBROKE"},
    {0xC67F2C99, "DIALOGUE_BADDSG_MANSIONGD"},
    {0xF803BC31, "DIALOGUE_BADDSG_MIME"},
    {0x52382033, "DIALOGUE_BADDSG_CIVILIAN"},
    {0xF8E96419, "DIALOGUE_BADDSG_MECHSA"},
    {0x812B3129, "DIALOGUE_BADDSG_BARBER"},
    {0xE98B4368, "DIALOGUE_BADDSG_STYLIST"},
    {0xFF2438C0, "DIALOGUE_BADDSG_DRIVERPALE"},
    {0xE290EC24, "DIALOGUE_BADDSG_FARM"},
    {0xB4667C2D, "DIALOGUE_BADDSG_NITIATE"},
    {0x311C6E40, "DIALOGUE_BADDSG_PHOTOGRAPHER"},
    {0xEAC0A18F, "DIALOGUE_BADDSG_DEALER"},
    {0x07C01A3A, "DIALOGUE_BADDSG_BOATCPTN"},
    {0x8C7FA868, "DIALOGUE_BADDSG_PILOT"},
    {0x63DE52F5, "DIALOGUE_BADDSG_47SAPIENZA"},
    {0x581AC7BB, "DIALOGUE_BADDSG_INTERN"},
    {0xE46CB127, "DIALOGUE_BADDSG_INVESTBANKER"},
    {0xBAD3FB04, "DIALOGUE_BADDSG_PLAGUE"},
    {0x90097B9E, "DIALOGUE_BADDSG_SURGEON"},
    {0xE024DE25, "DIALOGUE_BADDSG_WINTER"},
    {0xD99908DA, "DIALOGUE_BADDSG_KRNSTDTSEC"},
    {0x1B2E8021, "DIALOGUE_BADDSG_IT"},
    {0x43856774, "DIALOGUE_BADDSG_MUMMY"},
    {0xEE81107D, "DIALOGUE_BADDSG_47PREORDER"},
    {0xAB2B5D87, "DIALOGUE_BADDSG_SOMMELIER"},
    {0xE2A36BE7, "DIALOGUE_BADDSG_DOCTOR"},
    {0xC9DFF835, "DIALOGUE_BADDSG_COOK"},
    {0x33B33167, "DIALOGUE_BADDSG_CQANALYST"},
    {0xC4FE9470, "DIALOGUE_BADDSG_SCNTST"},
    {0x1EAB71C2, "DIALOGUE_BADDSG_DRIVERUK"},
    {0xE992E869, "DIALOGUE_BADDSG_DELIVERY"},
    {0x50C7A969, "DIALOGUE_BADDSG_47BANGKOK"},
    {0x9B3A95AF, "DIALOGUE_BADDSG_DRIVERCH"},
    {0x56477E9B, "DIALOGUE_BADDSG_OUTBREAK"},
    {0x8098A04B, "DIALOGUE_BADDSG_CLN"},
    {0x7D99B5FD, "DIALOGUE_BADDSG_RACECOORD"},
    {0x3379FEFD, "DIALOGUE_BADDSG_QUEENSGRD"},
    {0xCEC1F233, "DIALOGUE_BADDSG_47SUIT"},
    {0x809C37B0, "DIALOGUE_BADDSG_BIKER"},
    {0x500FFD59, "DIALOGUE_BADDSG_KILLBILL"},
    {0x82F47736, "DIALOGUE_BADDSG_COUNSELLOR"},
    {0x97328E14, "DIALOGUE_BADDSG_RABBIT"},
    {0x10F08338, "DIALOGUE_BADDSG_MECH_MIAMI"},
    {0x823ED220, "DIALOGUE_BADDSG_CQCNTRLLR"},
    {0x4E662A81, "DIALOGUE_BADDSG_MECHUK"},
    {0x33CF7CD3, "DIALOGUE_BADDSG_CREW"},
    {0x4D83AD5D, "DIALOGUE_BADDSG_WAITER"},
    {0x2AE97317, "DIALOGUE_BADDSG_OFFICER"},
    {0x9D589DCD, "DIALOGUE_BADDSG_HOUSSTFF"},
    {0x8D2C2B92, "DIALOGUE_BADDSG_BASEBALL"},
    {0xD7E15AC2, "DIALOGUE_BADDSG_FORTUNE"},
    {0x56A7D9A6, "DIALOGUE_BADDSG_DELIVERYFOX"},
    {0xFC3A0E0A, "DIALOGUE_BADDSG_ARTIST"},
    {0xB8C53766, "DIALOGUE_BADDSG_THUG"},
    {0x963FBCD0, "DIALOGUE_BADDSG_UNDERTAKER"},
    {0xD6EC692B, "DIALOGUE_BADDSG_ELITE"},
    {0x94A9DBA9, "DIALOGUE_BADDSG_COWBOYHAT"},
    {0xCBF7CEEC, "DIALOGUE_BADDSG_MECHCH"},
    {0x721A0667, "DIALOGUE_BADDSG_CUSTDN"},
    {0x34816F7C, "DIALOGUE_BADDSG_CRITIC"},
    {0x73E648D6, "DIALOGUE_BADDSG_DASILVA"},
    {0x82CD5E36, "DIALOGUE_BADDSG_ENGINEERDUG"},
    {0x73BA4604, "DIALOGUE_BADDSG_QUEENTHUG"},
    {0x71EB18CA, "DIALOGUE_BADDSG_DBBWLL"},
    {0xD31F9A79, "DIALOGUE_BADDSG_NURSE"},
    {0x189B435B, "DIALOGUE_BADDSG_CYCLIST"},
    {0x97D62391, "DIALOGUE_BADDSG_CQSUBJECT"},
    {0x9FD9A912, "DIALOGUE_BADDSG_PRIEST"},
    {0xE0D1016F, "DIALOGUE_BADDSG_BOLLYCREW"},
    {0xE224EF65, "DIALOGUE_BADDSG_47TRAINING"},
    {0x81090804, "DIALOGUE_BADDSG_47STARTCLASSY"},
    {0xF26004CE, "DIALOGUE_BADDSG_HERALD"},
    {0x997E8C1C, "DIALOGUE_BADDSG_KSHMRN"},
    {0x9BCEC571, "DIALOGUE_BADDSG_FAKEMLSTRM"},
    {0x4081454B, "DIALOGUE_BADDSG_NINJA"},
    {0x1B88D1B8, "DIALOGUE_BADDSG_SECURITY"},
    {0x51BACEEB, "DIALOGUE_BADDSG_COP"},
    {0x5FD627C3, "DIALOGUE_BADDSG_CAVEWKR"},
    {0x1A7877E1, "DIALOGUE_BADDSG_KRUGER"},
    {0x4F645BA7, "DIALOGUE_BADDSG_STALKER"},
    {0xC58AF9EE, "DIALOGUE_BADDSG_TECHCREW"},
    {0x8A2E3AB1, "DIALOGUE_BADDSG_BERG"},
    {0x9651F88F, "DIALOGUE_BADDSG_47NEWZEALAND"},
    {0xB9695552, "DIALOGUE_BADDSG_MUMBAISERV"},
    {0x6DD4DA4C, "DIALOGUE_BADDSG_ROBBER"},
    {0xB5C88119, "DIALOGUE_BADDSG_LAWYERBD"},
    {0x64F6A0A0, "DIALOGUE_BADDSG_LAUNDRYWKR"},
    {0xCB7385B6, "DIALOGUE_BADDSG_SHOPKEEP"},
    {0xA03975C5, "DIALOGUE_BADDSG_ATTENDANT"},
    {0x8A0D2CD0, "DIALOGUE_BADDSG_HIPPIE"},
    {0xA3265C81, "DIALOGUE_BADDSG_BUSKER"},
    {0xB7B4A985, "DIALOGUE_BADDSG_BUTCHER"},
    {0xF2F1E59C, "AI_HMArm"},
    {0xF5BF8030, "DIALOGUE_SYRINGE"},
    {0xD22765B3, "DIALOGUE_FISH"},
    {0x2F408478, "DIALOGUE_SCISSORS"},
    {0x798A7A77, "DIALOGUE_SCRWDRVR"},
    {0x2D414CB7, "DIALOGUE_COCONUT"},
    {0xE7C11FAB, "DIALOGUE_SHOTGUN"},
    {0x4CDE37C6, "DIALOGUE_SWORD"},
    {0x96FE075B, "DIALOGUE_PAN"},
    {0xF2C9428E, "DIALOGUE_PIPE"},
    {0x6DEF0A1C, "DIALOGUE_RIFLE"},
    {0x955BB884, "DIALOGUE_CASE"},
    {0xF080C8E1, "DIALOGUE_BAG"},
    {0x6BE64824, "DIALOGUE_BATON"},
    {0x9C779D48, "DIALOGUE_FALLBACK"},
    {0x07F5EB1A, "DIALOGUE_HAMMER"},
    {0x6C7B68E1, "DIALOGUE_KNIFE"},
    {0x87C8BE8C, "DIALOGUE_AXE"},
    {0x772829E7, "DIALOGUE_SNIPER"},
    {0x8BE9047A, "DIALOGUE_AUTO"},
    {0x0C834487, "DIALOGUE_CROWBAR"},
    {0xA13965FB, "DIALOGUE_GUN"},
    {0x6D5C5897, "DIALOGUE_BOMB"},
    {0x2381C24A, "DIALOGUE_SHOVEL"},
    {0x55BD19F2, "DIALOGUE_CLUB"},
    {0x9523C11C, "DIALOGUE_IRON"},
    {0x743E893F, "DIALOGUE_BAT"},
    {0xEE3CEF29, "DIALOGUE_BOLTCUTTER"},
    {0xCDA01F51, "DIALOGUE_TOOL"},
    {0x996287D3, "AI_HMLastKnownGtag"},
    {0x6ED40AA9, "Bridge"},
    {0x1197AC8D, "Rehab"},
    {0x7EAC03DF, "Entrance"},
    {0xEE4FB996, "Barn"},
    {0x09D8CD48, "LivingRoom"},
    {0x2909269D, "MasterBed"},
    {0x03BDBC6D, "Stage"},
    {0xE82A8AE5, "RageRoom"},
    {0xAE9D3294, "Research"},
    {0xCF01EC91, "TellerHall"},
    {0xE7592619, "Showroom"},
    {0x3C39DFB8, "Stonecirc"},
    {0xA54DC27F, "Cave"},
    {0x9761D873, "Church"},
    {0xB551A722, "DiningRoom"},
    {0xE69D4DE8, "Penthouse"},
    {0x476A9D0D, "Gym"},
    {0x2FA88503, "Corridor"},
    {0x0FA306B8, "Pool"},
    {0x64EA1CDB, "Wrkshp"},
    {0x8DDD3F9B, "WaitingArea"},
    {0xC899442B, "Confroom"},
    {0x23F0AF20, "CarCover"},
    {0x3BD8C15E, "Observatory"},
    {0xA4C811EF, "Restaurant"},
    {0xAD64AD18, "Ruins"},
    {0x19A62B4A, "ArtInstall"},
    {0xA05B134B, "Bow"},
    {0x0A6280E0, "Channels"},
    {0xAC458B1C, "Beach"},
    {0x255E4BC4, "Expo"},
    {0x84E83DFC, "Hut"},
    {0xFA58F55B, "Morgue"},
    {0x716D7BFE, "Crowd"},
    {0x4CAA4213, "Laundry"},
    {0x8B0FE91B, "TrophyRoom"},
    {0x5706BB77, "Orchard"},
    {0x60A20160, "Dancefloor"},
    {0x7EA3A385, "WineCellar"},
    {0xEAEE8965, "Elevator"},
    {0x3BA51DDC, "Garden"},
    {0x73FD6E34, "Office"},
    {0x9335F2CB, "Greenhouse"},
    {0x78F165DC, "Cliff"},
    {0x34274965, "Podium"},
    {0x988A643D, "Garage"},
    {0x8B911714, "Surgery"},
    {0xD20CAC30, "Shrine"},
    {0xA7EFB2B5, "ITRoom"},
    {0x52375AB5, "MasterBath"},
    {0x81ED2C8B, "DepositBox"},
    {0x749091AF, "Attic"},
    {0x15E448F4, "Canteen"},
    {0xF047260C, "Dungeon"},
    {0x889641A6, "Gallery"},
    {0x2CA84FF5, "Bushes"},
    {0x551D40AE, "Trees"},
    {0xF9FEAC48, "Medbay"},
    {0xB77E6897, "ServerRm"},
    {0x4B2D9634, "Backyard"},
    {0x839D4A3D, "Vineyard"},
    {0xA217603C, "Hangar"},
    {0xE3A3F2F2, "Port"},
    {0xEC6DDF6B, "Shed"},
    {0x251EF7A8, "Kitchen"},
    {0x5E90AE8F, "Freezer"},
    {0x4971A074, "Bathroom"},
    {0x0BCEAFE3, "Pier"},
    {0x6E42095F, "Warehouse"},
    {0x0788C9D7, "Foliage"},
    {0xFA210703, "Krnspaddock"},
    {0xDA2FF75B, "VRRoom"},
    {0xEF8FD02E, "Cafe"},
    {0xA9FA6F3C, "Sauna"},
    {0x89828016, "Yard"},
    {0x504D58FF, "BarrelRoom"},
    {0x8BA08F23, "Catwalk"},
    {0xD4FDCB22, "Suite"},
    {0x6AF65B2F, "Terrace"},
    {0x85677AB0, "Tanks"},
    {0xD4790FE3, "Basement"},
    {0x6A90DFEE, "PitGarage"},
    {0x9FCB8864, "Tower"},
    {0x6E3DA120, "Library"},
    {0x3EC4659C, "Stands"},
    {0x93458B98, "Playground"},
    {0xA4D32B2D, "Stern"},
    {0xB8FD303C, "HotSprings"},
    {0xC29271DD, "Hotel"},
    {0x2D8A5D26, "Parkinglot"},
    {0xC686B680, "UpperFloor"},
    {0x692DA6DC, "Balcony"},
    {0x254C0ED3, "SecretRoom"},
    {0xA5C73896, "Catacombs"},
    {0x39BE773C, "Fountain"},
    {0x2D0AA016, "ProdRoom"},
    {0x2E38CC47, "Overpass"},
    {0x60F2E4E5, "Railing"},
    {0x65364E35, "TradingFloor"},
    {0x181937AA, "Gate"},
    {0x29A87ACD, "Bedroom"},
    {0x40E339F0, "MainFloor"},
    {0xBBF3DDAB, "Lockerroom"},
    {0x9A346A5C, "Field"},
    {0xC86F1814, "Fallback"},
    {0xA3B6E182, "Mausoleum"},
    {0x7093C1B6, "Starboard"},
    {0x3A522FB3, "Den"},
    {0x457F272D, "Roof"},
    {0x3E0B200B, "Chillout"},
    {0x53261F38, "SecBuild"},
    {0xC8E6986D, "SlurryPit"},
    {0xF4C17C06, "Courtyard"},
    {0xAB3C1DDC, "TrainYard"},
    {0x5246754D, "Range"},
    {0x4EB2CA4A, "Bar"},
    {0x3EF16625, "Vault"},
    {0x8A496239, "UGFacility"},
    {0xCB8CB23E, "Classroom"},
    {0x3008F26F, "ExecOffice"},
    {0x599BF724, "Lab"},
    {0x9EEF368B, "Atrium"},
    {0xC7D70825, "VIP_Area"},
    {0x3422482F, "River"},
    {0x0D257AF3, "Lobby"},
    {0x60977170, "Cemetery"},
    {0x3309510B, "Staffroom"},
    {0x9BC722A8, "Storage"},
    {0xD32F0182, "Cinema"},
    {0xA2DFAA61, "Alley"},
    {0x84C3F7F9, "Paddocks"},
    {0x75BEABD4, "Toilet"},
    {0x6435D20D, "Park"},
    {0xBEFA7024, "BelowDeck"},
    {0x8F75618C, "Foyer"},
    {0xC024B29D, "Platform"},
    {0x69CCB0F7, "Grass"},
    {0x88DAFD86, "TastingRoom"},
    {0xCEF78E46, "Delivery"},
    {0xE6D3F9B3, "GreenRoom"},
    {0x2713C6CE, "Stairs"},
    {0xD61F529F, "Backstage"},
    {0xF5D584E2, "Marsh"},
    {0xCA95CFBB, "Helipad"},
    {0xB575F43A, "Skywalk"},
    {0x0C58E39C, "Shop"},
    {0x8E0242C9, "Walls"},
    {0x4D870280, "Studio"},
    {0xF742D6EE, "Street"},
    {0x4CC20BFF, "Jungle"},
    {0xB90B21D6, "TechRoom"},
    {0xD227078C, "Reception"},
    {0x4FC9325A, "Lockers"},
    {0xA6141699, "House"},
    {0x70686D79, "CnstSite"},
    {0x843161C3, "Spa"},
    {0xCA4F6D9F, "Square"},
    {0x0B9E1CF5, "Laundromat"},
    {0x279AB8DB, "Barge"},
    {0xD2D15076, "Tunnel"},
    {0x35363CE5, "Controlroom"},
    {0x88DA571C, "AI_Sentry_ItemRequest"},
    {0x67586A25, "TICKET"},
    {0x24DCD7A5, "VIPTICKET"},
    {0x371AD751, "INVITE"},
    {0x849AC4E8, "AI_PQ"},
    {0xE1BFBD72, "AI_PQ47EntAck"},
    {0x192547E0, "AI_PQNPCEntAck"},
    {0x5E817350, "EV_DianaIDAcc"},
    {0x8630BAEF, "DIALOGUE_WATCH"},
    {0xFFCD169C, "DIALOGUE_GLASSES"},
    {0xD4EE2D43, "DIALOGUE_TATTOO"},
    {0xD1DDB1A8, "DIALOGUE_ACCFALLBACK"},
    {0x79A90CE9, "DIALOGUE_HAT"},
    {0xFFCC961F, "DIALOGUE_NECKLACE"},
    {0x5FA94DCD, "DIALOGUE_EARRINGS"},
    {0x14BF9240, "EV_DianaWelcome"},
    {0xC8695D7A, "DIALOGUE_PINOCHLE"},
    {0x003DFCF3, "DIALOGUE_BINGO"},
    {0x3341CEF6, "DIALOGUE_LUDO"},
    {0x147385C6, "DIALOGUE_HEX"},
    {0x00751C21, "DIALOGUE_LOCFALLBACK"},
    {0x0DA0EAA6, "DIALOGUE_SHOGI"},
    {0x61C689BD, "DIALOGUE_HILO"},
    {0x016413D1, "DIALOGUE_SPADES"},
    {0x0EBA55C6, "DIALOGUE_SAPO"},
    {0x76082EFE, "DIALOGUE_SOLITAIRE"},
    {0xAA1B86EE, "DIALOGUE_TROMPO"},
    {0x281225D7, "DIALOGUE_SICBO"},
    {0xA09B9A39, "DIALOGUE_CRAPS"},
    {0xB31BD6DD, "DIALOGUE_CHESS"},
    {0x688DAB7D, "DIALOGUE_POKER"},
    {0x2B3077CA, "DIALOGUE_MAHJONG"},
    {0x08B3275D, "DIALOGUE_MANCALA"},
    {0xB1058B58, "DIALOGUE_WHIST"},
    {0x0CB3E574, "DIALOGUE_BLACKJACK"},
    {0xB0786EB3, "DIALOGUE_BRIDGE"},
    {0xF6DD1480, "Ev_Tells"},
    {0x023A57A2, "DIALOGUE_NERVOUS"},
    {0xF5CD3431, "DIALOGUE_SWEETTOOTH"},
    {0x1FE6AD7E, "DIALOGUE_HUNGRY"},
    {0x09AA3473, "DIALOGUE_THIRSTY"},
    {0x0590407A, "DIALOGUE_SMOKER"},
    {0x53B7C0DD, "DIALOGUE_TLSFALLBACK"},
    {0xAB554AD3, "DIALOGUE_BOOKWORM"},
    {0x5AD5DC75, "DIALOGUE_ALLERGIC"},
    {0x835F5C74, "EV_DianaIDHair"},
    {0x5FBE4AE1, "DIALOGUE_BLACK"},
    {0x70310437, "DIALOGUE_BLOND"},
    {0xBB22649D, "DIALOGUE_BROWN"},
    {0x0CC0DB62, "DIALOGUE_GREY"},
    {0xC5D1A62A, "DIALOGUE_HAIRFALLBACK"},
    {0xFCEDF03C, "DIALOGUE_NOHAIR"},
    {0x11C3FF2F, "DIALOGUE_RED"},
    {0xB1937ADF, "EV_Meeting"},
    {0x85A289C9, "DIALOGUE_SECRET"},
    {0xDBFD782B, "DIALOGUE_AGFALLBACK"},
    {0x2D7707DE, "DIALOGUE_HANDOVER"},
    {0xA5BD8932, "DIALOGUE_BUSINESS"}
};
#pragma endregion

#pragma region RTLV
std::string Language::RTLV::Convert(Language::Version version, std::vector<char> data, std::string metaJson)
{
    ResourceConverter *converter = getConverter(version, "RTLV");
    if (!converter)
        return "";

    JsonString *converted = converter->FromMemoryToJsonString(data.data(), data.size());
    if (!converted)
        return "";

    json j = {
        {"$schema", "https://tonytools.win/schemas/rtlv.schema.json"},
        {"hash", ""},
        {"videos", json::object()},
        {"subtitles", json::object()}
    };

    try
    {
        json jConv = json::parse(converted->JsonData);
        converter->FreeJsonString(converted);
        converted = nullptr;

        if (jConv.at("AudioLanguages").size() != jConv.at("VideoRidsPerAudioLanguage").size())
        {
            fprintf(stderr, "[LANG//RTLV] Mismatch in languages and resource IDs in RL JSON!\n");
            return "";
        }

        for (const auto &[lang, id] : c9::zip(jConv.at("AudioLanguages"), jConv.at("VideoRidsPerAudioLanguage")))
            j.at("videos").push_back({lang, std::format("{:08X}{:08X}", id.at("m_IDHigh").get<uint32_t>(), id.at("m_IDLow").get<uint32_t>())});

        if (jConv.at("SubtitleLanguages").size() != jConv.at("SubtitleMarkupsPerLanguage").size())
        {
            fprintf(stderr, "[LANG//RTLV] Mismatch in subtitle languages and content in RL JSON!\n");
            return "";
        }

        for (const auto &[lang, text] : c9::zip(jConv.at("SubtitleLanguages"), jConv.at("SubtitleMarkupsPerLanguage")))
            j.at("subtitles").push_back({lang, text});

        json meta = json::parse(metaJson);
        j.at("hash") = meta["hash_path"].is_null() ? meta.at("hash_value") : meta.at("hash_path");

        return j.dump();
    }
    catch (json::exception err)
    {
        if (converted)
            converter->FreeJsonString(converted);

        fprintf(stderr, "[LANG//RTLV] JSON error:\n"
                        "\t%s\n", err.what());
    }
    return "";
}

Language::Rebuilt Language::RTLV::Rebuild(Language::Version version, std::string jsonString, std::string langMap)
{
    ResourceGenerator *generator = getGenerator(version, "RTLV");
    if (!generator)
        return {};

    Language::Rebuilt out{};
    tsl::ordered_map<std::string, std::string> depends{};

    std::unordered_map<std::string, uint32_t> languages;
    if (!langMap.empty())
    {
        std::vector<std::string> langs = split(langMap);
        for (int i = 0; i < langs.size(); i++)
            languages[langs.at(i)] = i;
    }
    else if (version == Version::H3)
        languages = {{"xx", 0}, {"en", 1}, {"fr", 2}, {"it", 3}, {"de", 4}, {"es", 5}, {"ru", 6}, {"cn", 7}, {"tc", 8}, {"jp", 9}};
    else
        languages = {{"xx", 0}, {"en", 1}, {"fr", 2}, {"it", 3}, {"de", 4}, {"es", 5}, {"ru", 6}, {"mx", 7}, {"br", 8}, {"pl", 9}, {"cn", 10}, {"jp", 11}, {"tc", 12}};

    try
    {
        json jSrc = json::parse(jsonString);

        if (jSrc.at("videos").size() < 1 || jSrc.at("subtitles").size() < 1)
        {
            fprintf(stderr, "[LANG//RTLV] Videos and/or subtitles object is empty!\n");
            return {};
        }

        // The langmap property overrides any argument passed languages maps.
        // This property ensures easy compat with tools like SMF.
        if (jSrc.contains("langmap"))
        {
            languages.clear();
            std::vector<std::string> langs = split(jSrc.at("langmap").get<std::string>());
            for (int i = 0; i < langs.size(); i++)
                languages[langs.at(i)] = i;
        }

        json j = {
            {"AudioLanguages", {}},
            {"VideoRidsPerAudioLanguage", {}},
            {"SubtitleLanguages", {}},
            {"SubtitleMarkupsPerLanguage", {}}
        };

        for (const auto &[lang, video] : jSrc.at("videos").items())
        {
            if (!languages.contains(lang))
            {
                fprintf(stderr, "[LANG//RTLV] Language map does not contain language \"%s\".\n", lang.c_str());
                return {};
            }

            j.at("AudioLanguages").push_back(lang);

            std::string vidHash = video.get<std::string>();
            if (!is_valid_hash(vidHash))
                vidHash = computeHash(vidHash);

            uint64_t id = std::strtoull(vidHash.c_str(), nullptr, 16);

            j.at("VideoRidsPerAudioLanguage").push_back(json::object({{"m_IDHigh", id >> 32}, {"m_IDLow", id & ULONG_MAX}}));

            depends[vidHash] = std::format("{:2X}", 0x80 + languages[lang]);
        }

        for (const auto &[lang, text] : jSrc.at("subtitles").items())
        {
            j.at("SubtitleLanguages").push_back(lang);
            j.at("SubtitleMarkupsPerLanguage").push_back(text);
        }

        std::string rlJson = j.dump();
        ResourceMem *generated = generator->FromJsonStringToResourceMem(rlJson.c_str(), rlJson.size(), false);
        if (!generated)
            return {};

        out.file.resize(generated->DataSize);
        std::memcpy(out.file.data(), generated->ResourceData, generated->DataSize);
        generator->FreeResourceMem(generated);

        out.meta = generateMeta(jSrc.at("hash"), out.file.size(), "RTLV", depends);

        return out;
    }
    catch (json::exception err)
    {
        fprintf(stderr, "[LANG//RTLV] JSON error:\n"
                        "\t%s\n", err.what());
    }
    return {};
}
#pragma endregion

#pragma region LOCR
std::string Language::LOCR::Convert(Language::Version version, std::vector<char> data, std::string metaJson, std::string langMap, bool symmetric)
{
    buffer buff(data);

    bool isLOCRv2 = false;
    if (version != Version::H2016)
    {
        buff.read<uint8_t>();
        isLOCRv2 = true;
    }

    json j = {
        {"$schema", "https://tonytools.win/schemas/locr.schema.json"},
        {"hash", ""},
        {"symmetric", true},
        {"languages", json::object()}
    };

    if (!symmetric || version != Version::H2016)
        j.erase("symmetric");

    uint32_t numLanguages = (buff.read<uint32_t>() - isLOCRv2) / 4;
    buff.index -= 4;
    std::vector<std::string> languages = {"xx", "en", "fr", "it", "de", "es", "ru", "mx", "br", "pl", "cn", "jp", "tc"};;
    if (!langMap.empty())
        languages = split(langMap);
    else if (version == Version::H3)
        languages = {"xx", "en", "fr", "it", "de", "es", "ru", "cn", "tc", "jp"};

    if (numLanguages > languages.size())
    {
        fprintf(stderr, "[LANG//LOCR] Language map is smaller than the number of languages in the file!\n");
        return "";
    }

    size_t oldIndex = buff.index;
    for (int i = 0; i < numLanguages; i++)
    {
        j.at("languages").push_back({languages.at(i), json::object()});

        uint32_t oldOffset = buff.index;
        uint32_t offset;
        buff.index = oldIndex;
        if ((offset = buff.read<uint32_t>()) == ULONG_MAX)
        {
            buff.index = oldOffset;
            oldIndex += 4;
            continue;
        }
        buff.index = offset;
        oldIndex += 4;

        uint32_t numStrings = buff.read<uint32_t>();
        for (int k = 0; k < numStrings; k++)
        {
            uint32_t hash = buff.read<uint32_t>();
            std::string str = (symmetric && version == Version::H2016)
                                ? symmetricDecrypt(buff.read<std::vector<char>>())
                                : xteaDecrypt(buff.read<std::vector<char>>());
            buff.index += 1;

            j.at("languages").at(languages.at(i)).push_back({std::format("{:X}", hash), str});
        }
    }

    if (buff.index != buff.size())
    {
        fprintf(stderr, "[LANG//LOCR] Did not read to end of file! Report this!\n");
        return "";
    }

    try
    {
        json meta = json::parse(metaJson);
        j["hash"] = meta["hash_path"].is_null() ? meta.at("hash_value") : meta.at("hash_path");

        return j.dump();
    }
    catch (json::exception err)
    {
        fprintf(stderr, "[LANG//LOCR] JSON error:\n"
                        "\t%s%s\n", err.what(),
                        version == Version::H2016
                        ? "\nIf this is an older H2016 LOCR file, this may be due to a symmetric cipher being used." : "");
    }
    return "";
}

Language::Rebuilt Language::LOCR::Rebuild(Language::Version version, std::string jsonString, bool symmetric)
{
    Language::Rebuilt out{};

    try
    {
        json jSrc = json::parse(jsonString);

        buffer buff;

        if (!jSrc["symmetric"].is_null() && jSrc["symmetric"].get<bool>() && version == Version::H2016)
            symmetric = true;

        if (version != Version::H2016)
            buff.write<char>('\0');

        uint32_t curOffset = buff.index;
        buff.insert(jSrc.at("languages").size() * 4);

        for (const auto &[lang, strings] : jSrc.at("languages").items())
        {
            if (!strings.size())
            {
                uint32_t temp = buff.index;
                buff.index = curOffset;
                buff.write<uint32_t>(ULONG_MAX);
                curOffset += 4;
                buff.index = temp;
                continue;
            }

            uint32_t temp = buff.index;
            buff.index = curOffset;
            buff.write<uint32_t>(temp);
            curOffset += 4;
            buff.index = temp;

            buff.write<uint32_t>(strings.size());
            for (const auto &[strHash, string] : strings.items())
            {
                buff.write<uint32_t>(hexStringToNum(strHash));
                buff.write<std::vector<char>>((symmetric && version == Version::H2016)
                                                ? symmetricEncrypt(string.get<std::string>())
                                                : xteaEncrypt(string.get<std::string>())
                                            );
                buff.write<char>('\0');
            }
        }

        out.file = buff.data();
        out.meta = generateMeta(jSrc.at("hash"), out.file.size(), "LOCR", {});

        return out;
    }
    catch (json::exception err)
    {
        fprintf(stderr, "[LANG//LOCR] JSON error:\n"
                        "\t%s\n", err.what());
    }
    return {};
}
#pragma endregion

#pragma region DITL
std::string Language::DITL::Convert(std::vector<char> data, std::string metaJson)
{
    buffer buff(data);

    json j = {
        {"$schema", "https://tonytools.win/schemas/ditl.schema.json"},
        {"hash", ""},
        {"soundtags", json::object()}
    };

    try
    {
        json meta = json::parse(metaJson);
        j["hash"] = meta["hash_path"].is_null() ? meta.at("hash_value") : meta.at("hash_path");

        uint32_t count = buff.read<uint32_t>();
        for (uint32_t i = 0; i < count; i++)
        {
            std::string depend = meta.at("hash_reference_data").at(buff.read<uint32_t>()).at("hash");
            uint32_t soundtagHash = buff.read<uint32_t>();

            j.at("soundtags").push_back({
                TagMap.has_key(soundtagHash) ? TagMap.get_value(soundtagHash) : std::format("{:08X}", soundtagHash),
                depend
            });
        }

        // Sanity check
        if (buff.index != buff.size())
        {
            fprintf(stderr, "[LANG//DITL] Did not read to the end of the file! Report this!\n");
            return "";
        }

        return j.dump();
    }
    catch (json::exception err)
    {
        fprintf(stderr, "[LANG//DITL] JSON error:\n"
                        "\t%s\n", err.what());
    }
    return "";
}

Language::Rebuilt Language::DITL::Rebuild(std::string jsonString)
{
    Language::Rebuilt out{};
    tsl::ordered_map<std::string, std::string> depends{};

    try
    {
        json jSrc = json::parse(jsonString);

        buffer buff;

        buff.write<uint32_t>(jSrc.at("soundtags").size());

        for (const auto &[tagName, hash] : jSrc.at("soundtags").items())
        {
            if (depends.contains(hash))
                buff.write<uint32_t>(depends.find(hash) - depends.begin());
            else
            {
                buff.write<uint32_t>(depends.size());
                depends[hash] = "1F";
            }

            buff.write<uint32_t>(TagMap.has_value(tagName) ? TagMap.get_key(tagName) : hexStringToNum(tagName));
        }

        out.file = buff.data();
        out.meta = generateMeta(jSrc.at("hash"), out.file.size(), "DITL", depends);

        return out;
    }
    catch (json::exception err)
    {
        fprintf(stderr, "[LANG//DITL] JSON error:\n"
                        "\t%s\n", err.what());
    }
    return {};
}
#pragma endregion

#pragma region CLNG
std::string Language::CLNG::Convert(Language::Version version, std::vector<char> data, std::string metaJson, std::string langMap)
{
    buffer buff(data);

    json j = {
        {"$schema", "https://tonytools.win/schemas/clng.schema.json"},
        {"hash", ""},
        {"languages", json::object()}
    };

    std::vector<std::string> languages = {"xx", "en", "fr", "it", "de", "es", "ru", "mx", "br", "pl", "cn", "jp", "tc"};

    if (!langMap.empty())
        languages = split(langMap);
    else if (version == Version::H3)
        languages = {"xx", "en", "fr", "it", "de", "es", "ru", "cn", "tc", "jp"};

    uint32_t i = 0;
    while (buff.index != buff.size())
    {
        if (i >= languages.size())
        {
            fprintf(stderr, "[LANG//CLNG] Language map is smaller than the number of languages in the file!\n");
            return "";
        }

        j.at("languages").push_back({languages.at(i++), buff.read<bool>()});
    }

    try
    {
        json meta = json::parse(metaJson);
        j["hash"] = meta["hash_path"].is_null() ? meta.at("hash_value") : meta.at("hash_path");

        return j.dump();
    }
    catch (json::exception err)
    {
        fprintf(stderr, "[LANG//CLNG] JSON error:\n"
                        "\t%s\n", err.what());
    }
    return "";
}

Language::Rebuilt Language::CLNG::Rebuild(std::string jsonString)
{
    Language::Rebuilt out{};

    try
    {
        json jSrc = json::parse(jsonString);

        buffer buff;

        for (const auto &[language, value] : jSrc.at("languages").items())
            buff.write<bool>(value.get<bool>());

        out.file = buff.data();
        out.meta = generateMeta(jSrc.at("hash").get<std::string>(), out.file.size(), "CLNG", {});

        return out;
    }
    catch (json::exception err)
    {
        fprintf(stderr, "[LANG//CLNG] JSON error:\n"
                        "\t%s\n", err.what());
    }
    return {};
    
}
#pragma endregion

#pragma region DLGE
struct DLGE_Metadata
{
    uint16_t typeIndex; // >> 12 for type -- & 0xFFF for index
    // This is actually a u32 count, then X amount of u32s but our buffer
    // stream reader, when reading a vector, reads a u32 of size first.
    std::vector<uint32_t> SwitchHashes;
};

enum class DLGE_Type : uint8_t
{
    eDEIT_WavFile = 0x01,
    eDEIT_RandomContainer,
    eDEIT_SwitchContainer,
    eDEIT_SequenceContainer,
    eDEIT_Invalid = 0x15
};

NLOHMANN_JSON_SERIALIZE_ENUM(DLGE_Type, {
    {DLGE_Type::eDEIT_Invalid, "Invalid"},
    {DLGE_Type::eDEIT_WavFile, "WavFile"},
    {DLGE_Type::eDEIT_RandomContainer, "Random"},
    {DLGE_Type::eDEIT_SwitchContainer, "Switch"},
    {DLGE_Type::eDEIT_SequenceContainer, "Sequence"}
});

class DLGE_Container
{
public:
    uint8_t type;
    uint32_t SwitchGroupHash;
    uint32_t DefaultSwitchHash;
    std::vector<DLGE_Metadata> metadata;

    DLGE_Container(buffer &buff) 
    {
        type = buff.read<uint8_t>();
        SwitchGroupHash = buff.read<uint32_t>();
        DefaultSwitchHash = buff.read<uint32_t>();

        uint32_t count = buff.read<uint32_t>();
        DLGE_Metadata data;
        for (uint32_t i = 0; i < count; i++)
        {
            data = {
                buff.read<uint16_t>(),
                buff.read<std::vector<uint32_t>>()
            };

            metadata.push_back(data);
        }
    };

    DLGE_Container(uint8_t pType, uint32_t sgh, uint32_t dsh)
    {
        type = pType;
        SwitchGroupHash = sgh;
        DefaultSwitchHash = dsh;
        metadata = {};
    };

    void addMetadata(uint16_t typeIndex, std::vector<uint32_t> entries)
    {
        metadata.push_back({
            typeIndex,
            entries
        });
    };

    void write(buffer &buff)
    {
        buff.write<uint8_t>(type);
        buff.write<uint32_t>(SwitchGroupHash);
        buff.write<uint32_t>(DefaultSwitchHash);
        
        buff.write<uint32_t>(metadata.size());
        for (const DLGE_Metadata &metadata : metadata)
        {
            buff.write<uint16_t>(metadata.typeIndex);
            buff.write<std::vector<uint32_t>>(metadata.SwitchHashes);
        }
    };
};

std::string Language::DLGE::Convert(Language::Version version, std::vector<char> data, std::string metaJson, std::string defaultLocale, bool hexPrecision, std::string langMap)
{
    buffer buff(data);

    json j = {
        {"$schema", "https://tonytools.win/schemas/dlge.schema.json"},
        {"hash", ""},
        {"DITL", ""},
        {"CLNG", ""}
    };

    // These are H2 languages by default.
    std::vector<std::string> languages = {"xx", "en", "fr", "it", "de", "es", "ru", "mx", "br", "pl", "cn", "jp", "tc"};
    if (!langMap.empty())
    {
        languages = split(langMap);
        j.push_back({"langmap", langMap});
    }
    else if (version == Version::H2016)
        // Late versions of H2016 share the same langmap as H2, but without tc, so we remove it.
        languages.pop_back();
    else if (version == Version::H3)
        languages = {"xx", "en", "fr", "it", "de", "es", "ru", "cn", "tc", "jp"};

    j.push_back({"rootContainer", nullptr});

    try
    {
        json meta = json::parse(metaJson);
        j["hash"] = meta["hash_path"].is_null() ? meta.at("hash_value") : meta.at("hash_path");
        j["DITL"] = meta.at("hash_reference_data").at(buff.read<uint32_t>()).at("hash");
        j["CLNG"] = meta.at("hash_reference_data").at(buff.read<uint32_t>()).at("hash");

        // We setup these maps to store the various types of containers and the latest index for final construction later.
        std::map<uint8_t, tsl::ordered_map<uint32_t, json>> containerMap = {
            {1, {}},
            {2, {}},
            {3, {}},
            {4, {}}
        };
        std::map<uint8_t, uint32_t> indexMap = {
            {1, 0},
            {2, 0},
            {3, 0},
            {4, 0}
        };

        // Weirdly, sequences reference by some "global id" for certain types so we store this here.
        uint32_t globalIndex = -1;
        std::unordered_map<uint32_t, uint32_t> globalMap = {};
        
        // Read everything but the root typedIndex as we read that above.
        while (buff.index != (buff.size() - 2))
        {
            switch (buff.peek<uint8_t>())
            {
            case 0x01: // eDEIT_WavFile
            {
                buff.index += 1; // We don't need to record the type.
                uint32_t soundTagHash = buff.read<uint32_t>();
                uint32_t wavNameHash = buff.read<uint32_t>();

                // H2016 has this at the start of every language in a wav file container, we check this later.
                if (version != Version::H2016)
                    buff.read<uint32_t>();

                json wav = json::object({
                    {"type", DLGE_Type::eDEIT_WavFile},
                    {"wavName", std::format("{:08X}", wavNameHash)},
                    {"cases", nullptr},
                    {"weight", nullptr},
                    {"soundtag", TagMap.has_key(soundTagHash) ? TagMap.get_value(soundTagHash) : std::format("{:X}", soundTagHash)},
                    {"defaultWav", nullptr},
                    {"defaultFfx", nullptr},
                    {"languages", json::object()}
                });

                for (std::string const &language : languages)
                {
                    if (version == Version::H2016)
                        buff.read<uint32_t>();

                    uint32_t wavIndex = buff.read<uint32_t>(); // WWES/WWEM depend index
                    uint32_t ffxIndex = buff.read<uint32_t>(); // FaceFX depend index

                    json subtitleJson = nullptr;

                    if (wavIndex != ULONG_MAX && ffxIndex != ULONG_MAX)
                    {
                        if (language == defaultLocale)
                        {
                            wav.at("defaultWav") = meta.at("hash_reference_data").at(wavIndex).at("hash");
                            wav.at("defaultFfx") = meta.at("hash_reference_data").at(ffxIndex).at("hash");

                            // As we are most likely to have the english (default locale unless specified) hash, we get the wav hash from here.
                            wav.at("wavName") = getWavName(wav.at("defaultWav"), wav.at("defaultFfx"), std::format("{:08X}", wavNameHash));
                        }
                        else
                        {
                            subtitleJson = json::object({
                                {"wav", meta.at("hash_reference_data").at(wavIndex).at("hash")},
                                {"ffx", meta.at("hash_reference_data").at(ffxIndex).at("hash")}
                            });
                        }
                    }

                    if (buff.peek<uint32_t>() != 0)
                        if (subtitleJson.is_null())
                            subtitleJson = xteaDecrypt(buff.read<std::vector<char>>());
                        else
                            subtitleJson.push_back({"subtitle", xteaDecrypt(buff.read<std::vector<char>>())});
                    else
                        // We do this as we only peeked the size.
                        buff.index += 4;

                    if (!subtitleJson.is_null())
                        wav.at("languages").push_back({language, subtitleJson});
                }

                containerMap.at(0x01)[indexMap.at(0x01)++] = wav;
                break;
            }
            case 0x02: // eDEIT_RandomContainer
            {
                DLGE_Container container(buff);

                json cjson = json::object({
                    {"type", DLGE_Type::eDEIT_RandomContainer},
                    {"cases", nullptr},
                    {"containers", json::array()}
                });

                for (const DLGE_Metadata &metadata : container.metadata)
                {
                    // Random containers will ONLY EVER CONTAIN references to wav files.
                    // They will also only ever contain one "SwitchHashes" entry with the weight. 
                    // This has been verified across all games. This also makes sense when considering
                    // the purpose of the different containers. It makes no sense for a switch group or sequence to be randomised.

                    uint8_t type = metadata.typeIndex >> 12;
                    uint32_t index = metadata.typeIndex & 0xFFF;

                    // Referencing anything other than WavFiles in a random container is illogical.
                    if (type != 0x01) {
                        fprintf(stderr, "[LANG//DLGE] Bad random container reference [0x%02X].\n", type);
                        return "";
                    }

                    // Remove the switch property as it will be unused.
                    containerMap.at(type).at(index).erase("cases");

                    // Cannot use ternary here due to conflicting types.
                    if (hexPrecision) 
                        containerMap.at(type).at(index).at("weight") = std::format("{:06X}", metadata.SwitchHashes[0]);
                    else
                        containerMap.at(type).at(index).at("weight") = (double)metadata.SwitchHashes[0] / (double)0xFFFFFF;

                    cjson.at("containers").push_back(containerMap.at(type).at(index));
                    containerMap.at(type).erase(index);
                }

                containerMap.at(0x02)[indexMap.at(0x02)++] = cjson;
                globalMap[++globalIndex] = indexMap.at(0x02) - 1;
                break;
            }
            case 0x03: // eDEIT_SwitchContainer
            {
                DLGE_Container container(buff);

                json cjson = json::object({
                    {"type", DLGE_Type::eDEIT_SwitchContainer},
                    {
                        "switchKey", SwitchMap.has_key(container.SwitchGroupHash)
                            ? SwitchMap.get_value(container.SwitchGroupHash)
                            : std::format("{:08X}", container.SwitchGroupHash)
                    },
                    {
                        "default", SwitchMap.has_key(container.DefaultSwitchHash)
                            ? SwitchMap.get_value(container.DefaultSwitchHash)
                            : std::format("{:08X}", container.DefaultSwitchHash)
                    },
                    {"containers", json::array()}
                });

                for (const DLGE_Metadata &metadata : container.metadata) {
                    // Switch containers will ONLY EVER CONTAIN references to random containers. And there will only ever be 1 per DLGE.
                    // But, they may contain more than one entry (or no entries) in the "SwitchHashes" array.
                    // This has been verified across all games. This, again, makes sense when considering the purposes of each container.
                    // But, we allow WavFile references in HMLT as they make sense, but currently it's unknown if the game allows for this.

                    uint8_t type = metadata.typeIndex >> 12;
                    uint32_t index = metadata.typeIndex & 0xFFF;

                    // Referencing anything other than WavFiles and random containers in a switch container is illogical.
                    if (type != 0x01 && type != 0x02) {
                        fprintf(stderr, "[LANG//DLGE] Bad switch container reference [0x%02X].\n", type);
                        return "";
                    }

                    // Remove the weight value as it isn't used.
                    containerMap.at(type).at(index).erase("weight");

                    json caseArray = json::array();
                    for (auto &hash : metadata.SwitchHashes)
                        caseArray.push_back(SwitchMap.has_key(hash) ? SwitchMap.get_value(hash) : std::format("{:08X}", hash));

                    containerMap.at(type).at(index).at("cases") = caseArray;
                    cjson.at("containers").push_back(containerMap.at(type).at(index));
                    containerMap.at(type).erase(index);
                }

                containerMap.at(0x03)[indexMap.at(0x03)++] = cjson;
                globalMap[++globalIndex] = indexMap.at(0x03) - 1;
                break;
            }
            case 0x04: // eDEIT_SequenceContainer
            {
                // Sequence containers can contain any of the containers apart from sequence containers of course.
                // Unsure if this is a hard limitation, or if they've just not used any.
                // Further testing required. (Although if it is a limitation, this is logical).

                DLGE_Container container(buff);

                json cjson = json::object({
                    {"type", DLGE_Type::eDEIT_SequenceContainer},
                    {"containers", json::array()}
                });
                
                for (const DLGE_Metadata &metadata : container.metadata)
                {
                    uint8_t type = metadata.typeIndex >> 12;
                    uint32_t index = (type == 0x02 || type == 0x03) ? globalMap.at(metadata.typeIndex & 0xFFF) : metadata.typeIndex & 0xFFF;

                    if (type == 0x04) {
                        fprintf(stderr, "[LANG//DLGE] A sequence container cannot contain a sequence.\n");
                        return "";
                    }

                    // Remove cases and weight property as it won't be used.
                    switch(type) {
                        case 0x01:
                            containerMap.at(type).at(index).erase("weight");
                        case 0x02:
                            containerMap.at(type).at(index).erase("cases");
                    }

                    // We can do this as there is only one switch container per DLGE.
                    cjson.at("containers").push_back(type == 0x03 ? (--containerMap.at(type).end())->second : containerMap.at(type).at(index));
                    if (type == 0x03)
                        containerMap.at(type).clear();
                    else
                        containerMap.at(type).erase(index);
                }

                containerMap.at(0x04)[indexMap.at(0x04)++] = cjson;
                break;
            }
            case 0x15: // eDEIT_Invalid
            {
                fprintf(stderr, "[LANG//DLGE] Invalid section found. Report this!\n");
                return "";
            }
            default: // Just in case
            {
                fprintf(stderr, "[LANG//DLGE] Unknown section found [0x%02X]. Report this!\n", buff.read<uint8_t>());
                return "";
            }
            }
        }
        
        bool set = false;
        for (const auto [type, typedContainerMap] : containerMap) {
            if(!typedContainerMap.size()) continue;
            if(typedContainerMap.size() != 1 || set)
            {
                fprintf(stderr, "[LANG//DLGE] More than one container left over. Report this!\n");
                return "";
            }

            j.at("rootContainer") = typedContainerMap.rbegin()->second;

            switch (type) {
                case 0x01:
                    j.at("rootContainer").erase("weight");
                case 0x02:
                    j.at("rootContainer").erase("cases");
            }
            set = true;
        }

        if (!set)
        {
            fprintf(stderr, "[LANG//DLGE] No root container found. Report this!\n");
            return "";
        }

        return j.dump();
    }
    catch (json::exception err)
    {
        fprintf(stderr, "[LANG//DLGE] JSON error:\n"
                        "\t%s\n", err.what());
        fprintf(stdout, "This could be due to your language maps not matching up to the number of languages in the file (older file?).\n");
        return "";
    }
}

// Avoids code duplication
void addDepend(
    buffer &buff,
    std::string hash,
    std::string flag,
    tsl::ordered_map<std::string, std::string> &depends
)
{
    if (depends.contains(hash))
        buff.write<uint32_t>(depends.find(hash) - depends.begin());
    else
    {
        buff.write<uint32_t>(depends.size());
        depends[hash] = flag;
    }
}

bool processContainer(
    Language::Version version,
    buffer &buff,
    json container,
    std::unordered_map<uint32_t, uint32_t> &indexMap,
    std::vector<std::pair<std::string, uint32_t>> languages,
    tsl::ordered_map<std::string, std::string> &depends,
    std::string defLocale
)
{
    DLGE_Type type = container.at("type").get<DLGE_Type>();

    switch (type) {
        case DLGE_Type::eDEIT_WavFile: {
            buff.write<uint8_t>(0x01);

            std::string soundTag = container.at("soundtag").get<std::string>();
            buff.write<uint32_t>(TagMap.has_value(soundTag) ? TagMap.get_key(soundTag) : hexStringToNum(soundTag));
            buff.write<uint32_t>(hexStringToNum(container.at("wavName").get<std::string>()));

            if (version != Language::Version::H2016)
                buff.write<uint32_t>(0x00);

            for (const auto &[language, index] : languages)
            {
                if (version == Language::Version::H2016)
                    buff.write<uint32_t>(0x00);

                if (defLocale == language)
                {
                    if (!container.at("defaultWav").is_null() && !container.at("defaultFfx").is_null())
                    {
                        addDepend(
                            buff,
                            container.at("defaultWav").get<std::string>(),
                            std::format("{:02X}", 0x80 + index),
                            depends
                        );

                        addDepend(
                            buff,
                            container.at("defaultFfx").get<std::string>(),
                            std::format("{:02X}", 0x80 + index),
                            depends
                        );
                    }
                    else
                        buff.write<uint64_t>(ULLONG_MAX);

                    if (container.at("languages").contains(language))
                        if (container.at("languages").at(language).size() == 0)
                            buff.write<uint32_t>(0x00);
                        else
                            buff.write<std::vector<char>>(xteaEncrypt(container.at("languages").at(language).get<std::string>()));
                    else
                        buff.write<uint32_t>(0x00);
                }
                else
                {
                    if (!container.at("languages").contains(language))
                    {
                        buff.write<uint64_t>(ULLONG_MAX);
                        buff.write<uint32_t>(0x00);

                        continue;
                    }

                    if (container.at("languages").at(language).is_object())
                    {
                        // This language has wav and ffx (and possibly subtitles).
                        addDepend(
                            buff,
                            container.at("languages").at(language).at("wav").get<std::string>(),
                            std::format("{:02X}", 0x80 + index),
                            depends
                        );

                        addDepend(
                            buff,
                            container.at("languages").at(language).at("ffx").get<std::string>(),
                            std::format("{:02X}", 0x80 + index),
                            depends
                        );

                        if (container.at("languages").at(language).contains("subtitle"))
                            buff.write<std::vector<char>>(xteaEncrypt(container.at("languages").at(language).at("subtitle").get<std::string>()));
                        else
                            buff.write<uint32_t>(0x00);

                        continue;
                    }

                    buff.write<uint64_t>(ULLONG_MAX);

                    if (container.at("languages").at(language).size() == 0)
                        buff.write<uint32_t>(0x00);
                    else
                        buff.write<std::vector<char>>(xteaEncrypt(container.at("languages").at(language).get<std::string>()));
                }
            }

            indexMap.at(0x01)++;
            break;
        }
        case DLGE_Type::eDEIT_RandomContainer: {
            DLGE_Container rawContainer(0x02, 0, 0);

            // We need to write the wav files first.
            for (const json &childContainer : container.at("containers"))
            {
                if (childContainer.at("type").get<DLGE_Type>() != DLGE_Type::eDEIT_WavFile)
                {
                    fprintf(stderr, "[LANG//DLGE] Invalid type found in Random container!\n");
                    return false;
                }

                if (!childContainer.contains("weight"))
                {
                    fprintf(stderr, "[LANG//DLGE] Missing weight in Random container child.\n");
                    return false;
                }

                if (!processContainer(
                    version,
                    buff,
                    childContainer,
                    indexMap,
                    languages,
                    depends,
                    defLocale
                ))
                {
                    fprintf(stderr, "[LANG//DLGE] Failed to process a Random container child.\n");
                    return false;
                }
            
                uint32_t weight;

                if (childContainer.at("weight").is_string())
                {
                    std::string weightStr = childContainer.at("weight").get<std::string>();
                    // Hex precision was enabled on convert.
                    weight = std::strtoul(childContainer.at("weight").get<std::string>().c_str(), NULL, 16);
                    if (!std::all_of(weightStr.begin(), weightStr.end(), ::isxdigit))
                    {
                        fprintf(stderr, "[LANG//DLGE] Invalid weight found in Random container child.\n");
                        return false;
                    }
                }
                else
                    // It must be a double.
                    weight = round(childContainer.at("weight").get<double>() * 0xFFFFFF);

                // We only ever have WavFiles in a random container.
                rawContainer.addMetadata(
                    (0x01 << 12) | (indexMap.at(0x01) & 0xFFF),
                    std::vector<uint32_t>({
                        weight
                    })
                );
            }

            rawContainer.write(buff);
            indexMap.at(0x00)++;
            indexMap.at(0x02)++;
            break;
        }
        case DLGE_Type::eDEIT_SwitchContainer: {
            // This allows us to ensure that there's only one switch container per DLGE.
            if (indexMap.at(0x03) != -1)
            {
                fprintf(stderr, "[LANG//DLGE] Multiple Switch containers found in DLGE!\n");
                return false;
            }

            if (!container.contains("switchKey") || !container.contains("default"))
            {
                fprintf(stderr, "[LANG//DLGE] Switch container is missing \"switchKey\" or \"default\" property.\n");
                return false;
            }

            std::string switchKey = container.at("switchKey").get<std::string>();
            std::string defGroup = container.at("default").get<std::string>();
            DLGE_Container rawContainer(
                0x03,
                SwitchMap.has_value(switchKey) ? SwitchMap.get_key(switchKey) : hexStringToNum(switchKey),
                SwitchMap.has_value(defGroup) ? SwitchMap.get_key(defGroup) : hexStringToNum(defGroup)
            );

            for (const json &childContainer : container.at("containers"))
            {
                DLGE_Type cType = childContainer.at("type").get<DLGE_Type>();

                if (cType != DLGE_Type::eDEIT_WavFile && cType != DLGE_Type::eDEIT_RandomContainer)
                {
                    fprintf(stderr, "[LANG//DLGE] Invalid type found in Switch container.\n");
                    return false;
                }

                if (!childContainer.contains("cases"))
                {
                    fprintf(stderr, "[LANG//DLGE] Cases array missing from Switch container child.\n");
                    return false;
                }

                if (!processContainer(
                    version,
                    buff,
                    childContainer,
                    indexMap,
                    languages,
                    depends,
                    defLocale
                ))
                {
                    fprintf(stderr, "[LANG//DLGE] Failed to process a Switch container child.\n");
                    return false;
                }

                std::vector<uint32_t> switchCases{};

                for (const json &sCase : childContainer.at("cases"))
                {
                    std::string caseStr = sCase.get<std::string>();
                    switchCases.push_back(
                        SwitchMap.has_value(caseStr) ? SwitchMap.get_key(caseStr) : hexStringToNum(caseStr)
                    );
                }

                rawContainer.addMetadata(
                    ((uint8_t)cType << 12) | (indexMap.at((uint8_t)cType) & 0xFFF),
                    switchCases
                );

                indexMap.at(0x03)++;
            }

            rawContainer.write(buff);
            indexMap.at(0x00)++;
            indexMap.at(0x03)++;
            break;
        }
        case DLGE_Type::eDEIT_SequenceContainer: {
            // This allows us to ensure that there's only one sequence container per DLGE.
            if (indexMap.at(0x04) != -1)
            {
                fprintf(stderr, "[LANG//DLGE] Multiple Sequence containers found in DLGE!\n");
                return false;
            }

            DLGE_Container rawContainer(0x04, 0, 0);

            for (const json &childContainer : container.at("containers"))
            {
                DLGE_Type cType = childContainer.at("type").get<DLGE_Type>();

                if (!processContainer(
                    version,
                    buff,
                    childContainer,
                    indexMap,
                    languages,
                    depends,
                    defLocale
                ))
                {
                    fprintf(stderr, "[LANG//DLGE] Failed to process a Sequence container child.\n");
                    return false;
                }

                rawContainer.addMetadata(
                    ((uint8_t)cType << 12) | (indexMap.at((uint8_t)cType == 0x01 ? 0x01 : 0x00) & 0xFFF),
                    {}
                );

                indexMap.at(0x04)++;
            }

            rawContainer.write(buff);
            indexMap.at(0x00)++;
            indexMap.at(0x04)++;
            break;
        }
        case DLGE_Type::eDEIT_Invalid: {
            fprintf(stderr, "[LANG//DLGE] Invalid type found in JSON.\n");
            return false;
        }
    }

    if (container.contains("isRoot"))
        buff.write<uint16_t>(((uint8_t)type << 12) | (indexMap.at((uint8_t)type == 0x01 ? 0x01 : 0x00) & 0xFFF));

    return true;
}

Language::Rebuilt Language::DLGE::Rebuild(Language::Version version, std::string jsonString, std::string defaultLocale, std::string langMap)
{
    Language::Rebuilt out{};
    tsl::ordered_map<std::string, std::string> depends{};

    // We require it to be ordered. These are, like usual, the H2 languages.
    std::vector<std::pair<std::string, uint32_t>> languages = {
        {"xx", 0}, {"en", 1}, {"fr", 2}, {"it", 3}, {"de", 4}, {"es", 5}, {"ru", 6}, {"mx", 7}, {"br", 8}, {"pl", 9}, {"cn", 10}, {"jp", 11}, {"tc", 12}
    };
    if (!langMap.empty())
    {
        languages.clear();
        std::vector<std::string> langs = split(langMap);
        for (int i = 0; i < langs.size(); i++)
            languages.push_back({ langs.at(i), i });
    }
    else if (version == Version::H2016)
        // Late versions of H2016 share the same langmap as H2, but without tc, so we remove it.
        languages.pop_back();
    else if (version == Version::H3)
        languages = {{"xx", 0}, {"en", 1}, {"fr", 2}, {"it", 3}, {"de", 4}, {"es", 5}, {"ru", 6}, {"cn", 7}, {"tc", 8}, {"jp", 9}};

    try
    {
        json jSrc = json::parse(jsonString);

        buffer buff;

        // The langmap property overrides any argument passed languages maps.
        // This property ensures easy compat with tools like SMF.
        if (jSrc.contains("langmap"))
        {
            languages.clear();
            std::vector<std::string> langs = split(jSrc.at("langmap").get<std::string>());
            for (int i = 0; i < langs.size(); i++)
                languages.push_back({ langs.at(i), i });
        }
        
        buff.write<uint32_t>(0x00);
        depends[jSrc.at("DITL").get<std::string>()] = "1F";
        buff.write<uint32_t>(0x01);
        depends[jSrc.at("CLNG").get<std::string>()] = "1F";

        // So the processContainer function knows what the root container is.
        jSrc.at("rootContainer").push_back({ "isRoot", true });

        // 0 is the "global" index
        std::unordered_map<uint32_t, uint32_t> indexMap = {
            {0, -1},
            {1, -1},
            {2, -1},
            {3, -1},
            {4, -1}
        };

        // This function will process all containers recursively.
        if (!processContainer(
            version,
            buff,
            jSrc.at("rootContainer"),
            indexMap,
            languages,
            depends,
            defaultLocale
        ))
        {
            fprintf(stderr, "[LANG//DLGE] Failed to process containers!\n");
            return {};
        }

        out.file = buff.data();
        out.meta = generateMeta(jSrc.at("hash"), out.file.size(), "DLGE", depends);

        return out;
    }
    catch (json::exception err)
    {
        fprintf(stderr, "[LANG//DLGE] JSON error:\n"
                        "\t%s\n", err.what());
    }
    return {};
}
#pragma endregion
