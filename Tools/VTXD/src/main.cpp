#include "main.h"

struct VTXDHeader
{
    uint32_t numSubMeshes;
};

struct VTXDSubMeshHeader
{
    uint32_t id;
    uint32_t numVerts;
};

struct Colour
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
};

struct VTXDSubMesh
{
    VTXDSubMeshHeader hdr;
    std::vector<Colour> colours;
};

struct VTXD
{
    VTXDHeader hdr;
    std::vector<VTXDSubMesh> subMeshes;
};

int main(int argc, char *argv[])
{
    file_buffer buff;
    buff.load(argv[1]);

    VTXD vtxd{};

    vtxd.hdr = buff.read<VTXDHeader>();

    for (int i = 0; i < vtxd.hdr.numSubMeshes; i++)
    {
        VTXDSubMesh submesh{
            buff.read<VTXDSubMeshHeader>()};

        for (int j = 0; j < submesh.hdr.numVerts; j++)
        {
            submesh.colours.push_back(buff.read<Colour>());
        }

        vtxd.subMeshes.push_back(submesh);
    }
}
