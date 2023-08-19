#include <iostream>
#include <cstdint>
#include <vector>
#include <format>

#include "buffer.hpp"
#include "glob.h"
#include <assert.h>

#pragma pack(push, 1)
enum FXShaderType : uint32_t {
    FX_SHADER_TYPE_VERTEX_SHADER = 0,
    FX_SHADER_TYPE_PIXEL_SHADER,
    FX_SHADER_TYPE_GEOMETRY_SHADER,
    FX_SHADER_TYPE_DOMAIN_SHADER,
    FX_SHADER_TYPE_HULL_SHADER,
    FX_SHADER_TYPE_COMPUTE_SHADER,
    FX_SHADER_TYPE_SIZE
};

struct Header {
    uint32_t effectOffset;
    uint32_t effectSize;
    uint64_t reserved; // Space reserved for future use
};

struct FX2Header {
    uint32_t nShaders;
    uint32_t nShadersOffset;
    uint32_t nTechniques;
    uint32_t nTechniquesOffset;
    uint32_t nTextureStatesOffset;
};

struct FX3Header {
    uint32_t unk1; // Always 0x01, possibly version
    uint32_t unk2; // Always 0x20
    uint32_t nShaders;
    uint32_t nShadersOffset;
    uint32_t nTechniques;
    uint32_t nTechniquesOffset;
    uint32_t nTextureStates;
    uint32_t nTextureStatesOffset;
};

struct FXTechniqueHeader {
    uint32_t nNameOffset;
    uint32_t nPasses;
    uint32_t nPassStartOffset;
};

// This is a class as the nTags value is sometimes not present.
class FX2ProgramHeader {
public:
    uint32_t nMagicStart;
    uint32_t nNameOffset;
    FXShaderType eProgramType;
    uint32_t nTags = 0; // Not present in some versions, if they are present, always 0, some cases have 1.
    uint32_t nProgramOffset;
    uint32_t nProgramSize;
    uint32_t nConstantDescOffset;
    uint32_t nNumConstants;
    uint32_t nTextureDescOffset;
    uint32_t nNumTextures;
    uint32_t nConstSize;
    uint8_t nConstBufferBindMask;
    uint8_t nLastTextureStream;
    uint8_t unk1; // Presumed to be unused
    uint8_t unk2; // Presumed to be unused
    uint32_t nMagicEnd;

    FX2ProgramHeader(buffer& buff, bool tags = false) {
        hasTags = tags;

        nMagicStart = buff.read<uint32_t>();
        nNameOffset = buff.read<uint32_t>();
        eProgramType = buff.read<FXShaderType>();
        if (tags)
            nTags = buff.read<uint32_t>();
        nProgramOffset = buff.read<uint32_t>();
        nProgramSize = buff.read<uint32_t>();
        nConstantDescOffset = buff.read<uint32_t>();
        nNumConstants = buff.read<uint32_t>();
        nTextureDescOffset = buff.read<uint32_t>();
        nNumTextures = buff.read<uint32_t>();
        nConstSize = buff.read<uint32_t>();
        nConstBufferBindMask = buff.read<uint8_t>();
        nLastTextureStream = buff.read<uint8_t>();
        unk1 = buff.read<uint8_t>();
        unk2 = buff.read<uint8_t>();
        nMagicEnd = buff.read<uint32_t>();
    }

private:
    bool hasTags = false;
};

// This is a class as we read it in manually to detect
// what "version" of the header this is.
class FX3ProgramHeader {
public:
    uint32_t nMagicStart;
    uint32_t nNameOffset;
    FXShaderType eProgramType;
    uint32_t nTags;
    uint32_t nProgramOffset;
    uint32_t nProgramSize;
    uint32_t nConstantDescOffset;
    uint32_t nNumConstants;
    uint32_t nTextureDescOffset;
    uint32_t nNumTextures;
    uint32_t unk1; // Presumed to be unused (always 0xFFFFFFFF), but works like the above offsets.
    uint32_t unk2; // Presumed to be unused (always 0), but works like nNumX above. 
    uint32_t nConstSize;
    uint8_t nConstBufferBindMask;
    uint8_t nLastTextureStream;
    uint8_t unk3; // Not always 0, otherwise a value from 1-9
    uint8_t unk4; // Not always 0, otherwise always 98, from patch2+ always 0xFF
    uint64_t unk5;
    uint64_t unk6 = 0; // Only in patch2+ // Only in patch2+
    uint32_t nMagicEnd;

    FX3ProgramHeader(buffer& buff) {
        nMagicStart = buff.read<uint32_t>();
        nNameOffset = buff.read<uint32_t>();
        eProgramType = buff.read<FXShaderType>();
        nTags = buff.read<uint32_t>();
        nProgramOffset = buff.read<uint32_t>();
        nProgramSize = buff.read<uint32_t>();
        nConstantDescOffset = buff.read<uint32_t>();
        nNumConstants = buff.read<uint32_t>();
        nTextureDescOffset = buff.read<uint32_t>();
        nNumTextures = buff.read<uint32_t>();
        unk1 = buff.read<uint32_t>();
        unk2 = buff.read<uint32_t>();
        nConstSize = buff.read<uint32_t>();
        nConstBufferBindMask = buff.read<uint8_t>();
        nLastTextureStream = buff.read<uint8_t>();
        unk3 = buff.read<uint8_t>();
        unk4 = buff.read<uint8_t>();
        unk5 = buff.read<uint64_t>();
        nMagicEnd = buff.read<uint32_t>();

        if (nMagicEnd != 0xF4F5F6F7) {
            isNewVer = true;

            buff.index -= 4;
            unk6 = buff.read<uint64_t>();
            nMagicEnd = buff.read<uint32_t>();
        }
    }

private:
    bool isNewVer = false;
};

enum RenderConstBufferType : uint32_t {
    Vector1D = 1,
    Vector2D,
    Vector3D,
    Transform2D,
    Matrix44 = 8,
    Texture2D,
    Texture3D,
    TextureCube,
    TextureCubeArray = 13,
    Buffer = 15,
    Texture2DArray
};

struct FX2ConstantDesc {
    uint32_t nNameOffset;
    RenderConstBufferType nType;
    uint32_t nOffset;
    uint32_t nSize;
};

class FX2Desc {
public:
    std::string name;
    RenderConstBufferType nType;
    uint32_t nOffset;
    uint32_t nSize;

    FX2Desc(buffer& buff) {
        FX2ConstantDesc desc = buff.read<FX2ConstantDesc>();
        name = buff.read<std::string>(desc.nNameOffset);
        nType = desc.nType;
        nOffset = desc.nOffset;
        nSize = desc.nSize;
    }
};

// This is only present in early versions of the 2016 shader format.
// Future ones use the header seen below.
struct FX2PassDesc {
    uint32_t nNameOffset;
    uint32_t pShader[6];
};

struct FXPassHeader {
    uint32_t nNameOffset;
    uint32_t nNumShaders;
};

class FXPass {
public:
    std::string name;
    std::vector<uint32_t> shaders;

    FXPass(buffer& buff, bool isNewPass = false) {
        if (isNewPass) {
            isNewVer = true;

            // Technically we should read the header and then go from there,
            // but our buffer library allows us to read a u32 first, then a vector
            // of that amount.
            name = buff.read<std::string>(buff.read<uint32_t>());
            shaders = buff.read<std::vector<uint32_t>>();
        } else {
            FX2PassDesc bin = buff.read<FX2PassDesc>();
            name = buff.read<std::string>(bin.nNameOffset);
            for (uint32_t shader : bin.pShader) {
                if (shader != -1)
                    shaders.push_back(shader);
            }
        }
    }

private:
    bool isNewVer = false;
};

class FX2Program {
public:
    std::string name;
    FXShaderType type;
    std::vector<char> bytecode;
    std::vector<FX2Desc> constants;
    std::vector<FX2Desc> textures;

    FX2Program(buffer& buff, bool tags = false) {
        FX2ProgramHeader hdr = FX2ProgramHeader(buff, tags);
        
        buff.save();

        name = buff.read<std::string>(hdr.nNameOffset);
        type = hdr.eProgramType;
        bytecode = buff.read<std::vector<char>>(hdr.nProgramOffset, hdr.nProgramSize);

        if (hdr.nNumConstants) {
            buff.index = hdr.nConstantDescOffset;

            for (int i = 0; i < hdr.nNumConstants; i++)
                constants.push_back(FX2Desc(buff));
        }

        if (hdr.nNumTextures) {
            buff.index = hdr.nTextureDescOffset;

            for (int i = 0; i < hdr.nNumTextures; i++)
                textures.push_back(FX2Desc(buff));
        }

        buff.load();
    }
};

class FXTechnique {
public:
    std::string name;
    std::vector<FXPass> passes;

    FXTechnique(buffer& buff, bool isNewPass = false) {
        FXTechniqueHeader hdr = buff.read<FXTechniqueHeader>();
        
        buff.save();

        name = buff.read<std::string>(hdr.nNameOffset);
        
        buff.index = hdr.nPassStartOffset;

        for (int i = 0; i < hdr.nPasses; i++)
            passes.push_back(FXPass(buff, isNewPass));
    
        buff.load();
    }
};

class FX3Program {
public:
    std::string name;
    FXShaderType type;
    std::vector<char> bytecode;
    std::vector<FX2Desc> constants;
    std::vector<FX2Desc> textures;

    FX3Program(buffer& buff) {
        FX3ProgramHeader hdr(buff);

        // We have to skip 4 bytes otherwise the next read will be wrong.
        // This is probably down to alignment of some sort as the value is always 0.
        buff.index += 4;
        
        buff.save();

        name = buff.read<std::string>(hdr.nNameOffset);

        if (hdr.eProgramType > 6) {
            printf("%s %d\n", "New shader type found!", hdr.eProgramType);
            assert(false);
        }

        type = hdr.eProgramType;
        bytecode = buff.read<std::vector<char>>(hdr.nProgramOffset, hdr.nProgramSize);

        if (hdr.nNumConstants) {
            buff.index = hdr.nConstantDescOffset;

            for (int i = 0; i < hdr.nNumConstants; i++)
                constants.push_back(FX2Desc(buff));
        }

        if (hdr.nNumTextures) {
            buff.index = hdr.nTextureDescOffset;

            for (int i = 0; i < hdr.nNumTextures; i++)
                textures.push_back(FX2Desc(buff));
        }

        buff.load();
    }
};

class FX2Shader {
public:
    std::vector<FX2Program> programs;
    std::vector<FXTechnique> techniques;
    uint32_t texStatesOffset;

    // The tags variable is equivalent to the use of the new pass format.
    FX2Shader(buffer& buff, bool tags = false) {
        FX2Header hdr = buff.read<FX2Header>();
        
        texStatesOffset = hdr.nTextureStatesOffset;

        if (hdr.nTechniques) {
            buff.index = hdr.nTechniquesOffset;
            for (int i = 0; i < hdr.nTechniques; i++)
                techniques.push_back(FXTechnique(buff, tags));
        }

        if (hdr.nShaders) {
            buff.index = hdr.nShadersOffset;
            for (int i = 0; i < hdr.nShaders; i++)
                programs.push_back(FX2Program(buff, tags));
        }
    }
};

class FX3Shader {
public:
    std::vector<FX3Program> programs;
    std::vector<FXTechnique> techniques;
    std::vector<std::string> textureStates;

    FX3Shader(buffer& buff) {
        FX3Header hdr = buff.read<FX3Header>();

        if (hdr.nShaders) {
            buff.index = hdr.nShadersOffset;
            for (int i = 0; i < hdr.nShaders; i++)
                programs.push_back(FX3Program(buff));
        }

        if (hdr.nTechniques) {
            buff.index = hdr.nTechniquesOffset;
            for (int i = 0; i < hdr.nTechniques; i++)
                // All H3 shaders use the "new pass".
                techniques.push_back(FXTechnique(buff, true));
        }

        if (hdr.nTextureStates) {
            buff.index = hdr.nTextureStatesOffset;
            for (int i = 0; i < hdr.nTextureStates; i++)
                textureStates.push_back(buff.read<std::string>(buff.read<uint32_t>()));
        }
    }
};

struct NamesHeader {
    uint32_t magic; // If 0x33362626, skip to offset.
    uint32_t offset;
    uint32_t unk1; // Always 0x01
    uint32_t nNames;
    uint32_t nNamesOffset;
};
#pragma pack(pop)

int main(int argc, char *argv[])
{
    glob::glob glob(argv[1]);
    glob.use_full_paths(true);
    uint32_t shdrCount = 0;
    while (glob) {
        file_buffer buff;
        std::string path = glob.current_match();
        buff.loadf(path);

        Header hdr = buff.read<Header>();

        buff.rebase(hdr.effectOffset);
        buff.resize(hdr.effectSize);

        FX3Shader shdr(buff);

        // For H2016/2 shaders
        /*bool tags = false;
        std::vector<std::string> names;
        if (buff.peek<uint32_t>() == 0x33362626) {
            NamesHeader hdr = buff.read<NamesHeader>();
            buff.index = hdr.offset;
            tags = true;
        }

        FX2Shader shdr(buff, tags);*/

        glob.next();
    }
}
