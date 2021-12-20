#include "main.h"

struct ZNewTrajectory
{
    uint32_t frames;
    uint32_t sampleFreq;
};

struct Vector3
{
    float x;
    float y;
    float z;
};

struct QuantizedQuaternion
{
    uint16_t x;
    uint16_t y;
    uint16_t z;
    uint16_t w;
};

struct Quaternion
{
    float x;
    float y;
    float z;
    float w;

    Quaternion(uint16_t quan_x, uint16_t quan_y, uint16_t quan_z, uint16_t quan_w) {
        x = ((float)quan_x / 32767) - 1;
        y = ((float)quan_y / 32767) - 1;
        z = ((float)quan_z / 32767) - 1;
        w = ((float)quan_w / 32767) - 1;
    }
};

struct AnimDataHdr
{
    float duration;
    uint32_t boneCount;
    ZNewTrajectory trajectory;
    uint32_t frames;
    float fps;
    uint32_t keyframes;
    uint32_t pad;
    uint32_t frameCount;
    uint16_t constBoneQuat;
    uint16_t constBoneTrans;
    Vector3 translationScale;
    bool hasBindPose;
};

struct AnimDataOffsets
{
    size_t quatBitArray;
    size_t transBitArray;
    size_t retargetBitArray;
    size_t constQuats;
    size_t quats;
    size_t bindPoseQuats;
    size_t constTrans;
    size_t trans;
    size_t bindPoseTrans;
    size_t end;
};

AnimDataOffsets generateOffsets(AnimDataHdr hdr)
{
    AnimDataOffsets offsets{};

    size_t bindPoseBoneCount{};
    if (hdr.hasBindPose)
        bindPoseBoneCount = 2 * hdr.boneCount;
    else
        bindPoseBoneCount = 0;

    offsets.quatBitArray = 0x38;
    offsets.transBitArray = (8 * ((hdr.boneCount + 63) >> 6)) + 0x38;
    offsets.retargetBitArray = offsets.transBitArray + (offsets.transBitArray - 0x38);
    offsets.constQuats = offsets.retargetBitArray + 8 * ((uint32_t)(bindPoseBoneCount + 63) >> 6);
    offsets.quats = offsets.constQuats + 8 * hdr.constBoneQuat;
    offsets.bindPoseQuats = offsets.quats + 8 * hdr.frameCount * (hdr.boneCount - hdr.constBoneQuat);
    offsets.constTrans = offsets.bindPoseQuats + 8 * bindPoseBoneCount;
    offsets.trans = offsets.constTrans + 8 * hdr.constBoneTrans;
    offsets.bindPoseTrans = offsets.trans + 8 * hdr.frameCount * (hdr.boneCount - hdr.constBoneTrans);
    offsets.end = offsets.bindPoseTrans + (8 * bindPoseBoneCount);

    return offsets;
}

int main(int argc, char* argv[])
{
    // Load file data into vector
    std::ifstream inputFile(argv[1], std::ifstream::binary);
    std::vector<char> inputData((std::istreambuf_iterator<char>(inputFile)), std::istreambuf_iterator<char>());
    inputFile.close();

    uint32_t curIndex = 0;

    AnimDataHdr animDataHeader{};
    memcpy(&animDataHeader, &inputData[0], sizeof(AnimDataHdr));
    curIndex += sizeof(AnimDataHdr);

    AnimDataOffsets offsets = generateOffsets(animDataHeader);
    curIndex = offsets.constQuats;

    //for (int i = 0; i > (offsets.quats - offsets.constQuats) / 8; i++)
    //{
        QuantizedQuaternion quantQuat{};
        memcpy(&quantQuat, &inputData[curIndex], sizeof(QuantizedQuaternion));
        curIndex += sizeof(QuantizedQuaternion);

        Quaternion quat(quantQuat.x, quantQuat.y, quantQuat.z, quantQuat.w);

        LOG("Done 1");
    //}

    LOG("Done!");
}