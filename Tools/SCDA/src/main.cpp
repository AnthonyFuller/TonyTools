#include "main.h"

struct SCDAHeader
{
    uint32_t type;
    uint32_t numMaterials;
};

struct SCDAMaterialHeader
{
    uint32_t numInstances;
    uint32_t bend;
    float bendConstraint;
    float cutoffDistance;
    float scaleBeginDistance;
};

struct SCDAPackedInstance
{
    uint8_t data[11];
};

struct SCDAMaterial
{
    SCDAMaterialHeader hdr;
    std::vector<SCDAPackedInstance> instances;
};

struct SCDA
{
    SCDAHeader hdr;
    std::vector<SCDAMaterial> materials;
};

int main(int argc, char *argv[])
{
    file_buffer buff;
    buff.load(argv[1]);

    SCDA scda{};

    scda.hdr = buff.read<SCDAHeader>();

    for (int i = 0; i < scda.hdr.numMaterials; i++)
    {
        SCDAMaterial material{
            buff.read<SCDAMaterialHeader>()};

        scda.materials.push_back(material);
    }

    for (int i = 0; i < scda.hdr.numMaterials; i++)
    {
        SCDAMaterial *material = &scda.materials.at(i);

        for (int j = 0; j < material->hdr.numInstances; j++)
        {
            material->instances.push_back(buff.read<SCDAPackedInstance>());
        }
    }

    LOG("Done!");
}
