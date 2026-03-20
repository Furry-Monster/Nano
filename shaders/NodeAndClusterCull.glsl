#version 450
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

#define NANITE_MAX_GROUP_PARTS_BITS 5
#define NANITE_MAX_GROUP_PARTS_MASK ((1 << NANITE_MAX_GROUP_PARTS_BITS) - 1)
#define NANITE_MAX_GROUP_PARTS (1 << NANITE_MAX_GROUP_PARTS_BITS)

#define NANITE_MAX_RESOURCE_PAGES_BITS 16

#define NANITE_MAX_CLUSTERS_PER_GROUP_BITS 9
#define NANITE_MAX_CLUSTERS_PER_GROUP ((1 << NANITE_MAX_CLUSTERS_PER_GROUP_BITS) - 1)

#define NANITE_MAX_BVH_NODE_FANOUT_BITS 2
#define NANITE_MAX_BVH_NODE_FANOUT (1 << NANITE_MAX_BVH_NODE_FANOUT_BITS)

// 208 bytes per BVH node = 4 children x 13 uints x 4 bytes
#define HIERARCHY_NODE_SLICE_SIZE ((4 + 4 + 4 + 1) * 4 * NANITE_MAX_BVH_NODE_FANOUT)

layout(std430, binding = 0) buffer FBVHBuffer {
    uint mData[];
} BVHBuffer;

layout(std430, binding = 1) buffer FEchoBuffer {
    uint mData[];
} EchoBuffer;

layout(std430, binding = 2) buffer FMainAndPostNodeAndClusterBatches {
    uint mData[];
} MainAndPostNodeAndClusterBatches;

layout(std430, binding = 3) buffer FCurrentWorkArgs {
    uint mData[];
} CurrentWorkArgs;

layout(std430, binding = 4) buffer FNextWorkArgs {
    uint mData[];
} NextWorkArgs;

layout(binding = 5) uniform GlobalConstants {
    mat4 mProjectionMatrix;
    mat4 mViewMatrix;
    mat4 mModelMatrix;
    uvec4 mMisc0;          // x: Manual MipLevel
    vec4 mNanite_ViewOrigin;  // xyz: camera pos, w: lodScale
    vec4 mNanite_ViewForward; // xyz: view dir, w: lodScaleHW
} U_GlobalConstants;

layout(std430, binding = 6) readonly buffer FNaniteMesh {
    uint mData[];
} NaniteMesh;

uint BitFieldExtractU32(uint Data, uint Size, uint Offset) {
    Size &= 31;
    Offset &= 31;
    return (Data >> Offset) & ((1u << Size) - 1u);
}

struct FHierarchyNodeSlice {
    vec4  LODBounds;
    vec3  BoxBoundsCenter;
    vec3  BoxBoundsExtent;
    float MinLODError;
    float MaxParentLODError;
    uint  ChildStartReference;
    uint  NumChildren;
    uint  StartPageIndex;
    uint  NumPages;
    bool  bEnabled;
    bool  bLoaded;
    bool  bLeaf;
};

FHierarchyNodeSlice UnpackHierarchyNodeSlice(
    uvec4 RawData0, uvec4 RawData1, uvec4 RawData2, uint RawData3
) {
    const uvec4 Misc0 = RawData1;
    const uvec4 Misc1 = RawData2;
    const uint  Misc2 = RawData3;

    FHierarchyNodeSlice Node;
    Node.LODBounds           = uintBitsToFloat(RawData0);
    Node.BoxBoundsCenter     = uintBitsToFloat(Misc0.xyz);
    Node.BoxBoundsExtent     = uintBitsToFloat(Misc1.xyz);

    vec2 unpacked2Half       = unpackHalf2x16(Misc0.w);
    Node.MinLODError         = unpacked2Half.x;
    Node.MaxParentLODError   = unpacked2Half.y;
    Node.ChildStartReference = Misc1.w;
    Node.bLoaded             = (Misc1.w != 0xFFFFFFFFu);

    Node.NumChildren   = BitFieldExtractU32(Misc2, NANITE_MAX_CLUSTERS_PER_GROUP_BITS, 0);
    Node.NumPages      = BitFieldExtractU32(Misc2, NANITE_MAX_GROUP_PARTS_BITS,
                                            NANITE_MAX_CLUSTERS_PER_GROUP_BITS);
    Node.StartPageIndex = BitFieldExtractU32(Misc2, NANITE_MAX_RESOURCE_PAGES_BITS,
                                             NANITE_MAX_CLUSTERS_PER_GROUP_BITS +
                                             NANITE_MAX_GROUP_PARTS_BITS);
    Node.bEnabled = Misc2 != 0u;
    Node.bLeaf    = Misc2 != 0xFFFFFFFFu;

    return Node;
}

FHierarchyNodeSlice GetHierarchyNodeSlice(uint NodeIndex, uint ChildIndex) {
    const uint BaseAddress = (NodeIndex * HIERARCHY_NODE_SLICE_SIZE) / 4;

    uint i = BaseAddress + 4 * ChildIndex;
    const uvec4 RawData0 = uvec4(
        BVHBuffer.mData[i], BVHBuffer.mData[i + 1],
        BVHBuffer.mData[i + 2], BVHBuffer.mData[i + 3]
    );

    i = BaseAddress + 16 + ChildIndex * 4;
    const uvec4 RawData1 = uvec4(
        BVHBuffer.mData[i], BVHBuffer.mData[i + 1],
        BVHBuffer.mData[i + 2], BVHBuffer.mData[i + 3]
    );

    i = BaseAddress + 32 + ChildIndex * 4;
    const uvec4 RawData2 = uvec4(
        BVHBuffer.mData[i], BVHBuffer.mData[i + 1],
        BVHBuffer.mData[i + 2], BVHBuffer.mData[i + 3]
    );

    i = BaseAddress + 48 + ChildIndex;
    const uint RawData3 = BVHBuffer.mData[i];

    return UnpackHierarchyNodeSlice(RawData0, RawData1, RawData2, RawData3);
}

bool ShouldVisitChild(FHierarchyNodeSlice slice) {
    return true;
}

void main() {
    uint nodeOffset = CurrentWorkArgs.mData[5];
    uint nodeCount  = CurrentWorkArgs.mData[6];

    uint nextNodeOffsetInBuffer = nodeOffset + nodeCount;
    uint nodeOutputOffset       = nextNodeOffsetInBuffer;
    uint nextNodeCount          = 0;

    uint clusterOutputOffset = CurrentWorkArgs.mData[1];

    for (int nodeIndexOffset = 0; nodeIndexOffset < nodeCount; nodeIndexOffset++) {
        uint currentNodeIndex =
            MainAndPostNodeAndClusterBatches.mData[nodeOffset + nodeIndexOffset];

        for (int i = 0; i < 4; i++) {
            FHierarchyNodeSlice slice = GetHierarchyNodeSlice(currentNodeIndex, i);
            uint currentSliceMipLevel = slice.NumPages;

            if (slice.bEnabled) {
                if (!slice.bLeaf) {
                    MainAndPostNodeAndClusterBatches.mData[nodeOutputOffset] =
                        slice.ChildStartReference;
                    nodeOutputOffset++;
                    nextNodeCount++;
                } else {
                    if (currentSliceMipLevel == U_GlobalConstants.mMisc0.x) {
                        uint clusterCountInLeafNode = slice.NumChildren;
                        uint pageIndex = slice.ChildStartReference >> 8;
                        uint clusterOffsetInPage = slice.ChildStartReference & 0xFFu;

                        for (uint j = 0u; j < clusterCountInLeafNode; j++) {
                            MainAndPostNodeAndClusterBatches
                                .mData[1024 + clusterOutputOffset * 2] = pageIndex;
                            MainAndPostNodeAndClusterBatches
                                .mData[1024 + clusterOutputOffset * 2 + 1] =
                                    clusterOffsetInPage + j;
                            clusterOutputOffset++;
                        }
                    }
                }
            }
        }
    }

    NextWorkArgs.mData[5] = nextNodeOffsetInBuffer;
    NextWorkArgs.mData[6] = nextNodeCount;
    NextWorkArgs.mData[1] = clusterOutputOffset;
}
