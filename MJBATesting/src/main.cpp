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

    Quaternion(QuantizedQuaternion quanQuat) {
        x = ((float)quanQuat.x / 32767) - 1;
        y = ((float)quanQuat.y / 32767) - 1;
        z = ((float)quanQuat.z / 32767) - 1;
        w = ((float)quanQuat.w / 32767) - 1;
    }
};

struct Transform
{
    float x;
    float y;
    float z;
    float w;

    Transform(uint64_t data, Vector3 transScale) {
        // This is ugly as fuck
        x = (float)((int)(data >> 31) >> 11) * transScale.x;
        y = (float)((int)(data >> 10) >> 11) * transScale.y;
        z = (float)((int)((uint32_t)data << 11) >> 11) * transScale.z;
        w = 1.0;
    }
};

struct BoneTransform
{
    Quaternion m_rot;
    Transform m_trans;
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

std::vector<Quaternion> readQuantQuatArr(file_buffer& buff, int numOfQuat)
{
    std::vector<Quaternion> quatArr;
    for (int i = 0; i < numOfQuat; i++)
    {
        QuantizedQuaternion quantQuat = buff.read<QuantizedQuaternion>();
        Quaternion quat(quantQuat);

        quatArr.push_back(quat);
    }

    return quatArr;
}

std::vector<Transform> readQuantTransArr(file_buffer& buff, int numOfTrans, Vector3 transScale)
{
    std::vector<Transform> transArr;
    for (int i = 0; i < numOfTrans; i++)
    {
        uint64_t quantTransData = buff.read<uint64_t>();
        Transform trans(quantTransData, transScale);

        transArr.push_back(trans);
    }

    return transArr;
}

int main(int argc, char* argv[])
{
    file_buffer buff;
    buff.load(argv[1]);

    AnimDataHdr hdr = buff.read<AnimDataHdr>();

    AnimDataOffsets offsets = generateOffsets(hdr);

    buff.index = offsets.constQuats;
    assert(buff.index == offsets.constQuats);
    std::vector<Quaternion> constQuats = readQuantQuatArr(buff, (offsets.quats - offsets.constQuats) / 8);

    assert(buff.index == offsets.quats);
    std::vector<Quaternion> quats = readQuantQuatArr(buff, (offsets.bindPoseQuats - offsets.quats) / 8);

    assert(buff.index == offsets.bindPoseQuats);
    std::vector<Quaternion> bindPoseQuats = readQuantQuatArr(buff, (offsets.constTrans - offsets.bindPoseQuats) / 8);

    assert(buff.index == offsets.constTrans);
    std::vector<Transform> constTrans = readQuantTransArr(buff, (offsets.trans - offsets.constTrans) / 8, hdr.translationScale);

    assert(buff.index == offsets.trans);
    std::vector<Transform> trans = readQuantTransArr(buff, (offsets.bindPoseTrans - offsets.trans) / 8, hdr.translationScale);

    assert(buff.index == offsets.bindPoseTrans);
    std::vector<Transform> bindPoseTrans = readQuantTransArr(buff, (offsets.end - offsets.bindPoseTrans) / 8, hdr.translationScale);

    assert(buff.index == offsets.end);

    LOG("Done!");
}