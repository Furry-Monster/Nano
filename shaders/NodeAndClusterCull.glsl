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
    uvec4 mMisc0; // x: MipLevel, y: HZB on, z: screenW, w: screenH
    vec4 mNanite_ViewOrigin; // xyz: camera pos, w: lodScale
    vec4 mNanite_ViewForward; // xyz: view dir, w: lodScaleHW
} U_GlobalConstants;

layout(std430, binding = 6) readonly buffer FNaniteMesh {
    uint mData[];
} NaniteMesh;

layout(binding = 7) uniform sampler2D s_HZB;

uint BitFieldExtractU32(uint Data, uint Size, uint Offset) {
    Size &= 31;
    Offset &= 31;
    return (Data >> Offset) & ((1u << Size) - 1u);
}

struct FHierarchyNodeSlice {
    vec4 LODBounds;
    vec3 BoxBoundsCenter;
    vec3 BoxBoundsExtent;
    float MinLODError;
    float MaxParentLODError;
    uint ChildStartReference;
    uint NumChildren;
    uint StartPageIndex;
    uint NumPages;
    bool bEnabled;
    bool bLoaded;
    bool bLeaf;
};

FHierarchyNodeSlice UnpackHierarchyNodeSlice(
    uvec4 RawData0, uvec4 RawData1, uvec4 RawData2, uint RawData3
) {
    const uvec4 Misc0 = RawData1;
    const uvec4 Misc1 = RawData2;
    const uint Misc2 = RawData3;

    FHierarchyNodeSlice Node;
    Node.LODBounds = uintBitsToFloat(RawData0);
    Node.BoxBoundsCenter = uintBitsToFloat(Misc0.xyz);
    Node.BoxBoundsExtent = uintBitsToFloat(Misc1.xyz);

    vec2 unpacked2Half = unpackHalf2x16(Misc0.w);
    Node.MinLODError = unpacked2Half.x;
    Node.MaxParentLODError = unpacked2Half.y;
    Node.ChildStartReference = Misc1.w;
    Node.bLoaded = (Misc1.w != 0xFFFFFFFFu);

    Node.NumChildren = BitFieldExtractU32(Misc2, NANITE_MAX_CLUSTERS_PER_GROUP_BITS, 0);
    Node.NumPages = BitFieldExtractU32(Misc2, NANITE_MAX_GROUP_PARTS_BITS,
            NANITE_MAX_CLUSTERS_PER_GROUP_BITS);
    Node.StartPageIndex = BitFieldExtractU32(Misc2, NANITE_MAX_RESOURCE_PAGES_BITS,
            NANITE_MAX_CLUSTERS_PER_GROUP_BITS +
                NANITE_MAX_GROUP_PARTS_BITS);
    Node.bEnabled = Misc2 != 0u;
    Node.bLeaf = Misc2 != 0xFFFFFFFFu;

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

// Previous-frame hierarchical Z (max depth per tile). Object is occluded if its
// nearest projected depth exceeds the max depth stored in the HZB region.
bool IsNodeOccludedHZB(FHierarchyNodeSlice slice) {
    vec3 c = slice.BoxBoundsCenter;
    vec3 e = slice.BoxBoundsExtent;
    uvec2 screenSize = U_GlobalConstants.mMisc0.zw;
    if (screenSize.x < 2u || screenSize.y < 2u) {
        return false;
    }

    float minPx = 1e20;
    float maxPx = -1e20;
    float minPy = 1e20;
    float maxPy = -1e20;
    float zObjMin = 1e20;
    bool anyIn = false;

    for (int i = 0; i < 8; i++) {
        vec3 o = vec3(
                (i & 1) != 0 ? e.x : -e.x,
                (i & 2) != 0 ? e.y : -e.y,
                (i & 4) != 0 ? e.z : -e.z);
        vec3 wp = (U_GlobalConstants.mModelMatrix * vec4(c + o, 1.0)).xyz;
        vec3 rel = wp - U_GlobalConstants.mNanite_ViewOrigin.xyz;
        vec4 v = U_GlobalConstants.mViewMatrix * vec4(rel, 1.0);
        vec4 clip = U_GlobalConstants.mProjectionMatrix * v;
        if (clip.w <= 1e-4) {
            continue;
        }
        vec3 ndc = clip.xyz / clip.w;
        if (abs(ndc.x) > 1.5 || abs(ndc.y) > 1.5) {
            return false;
        }
        float sx = (ndc.x * 0.5 + 0.5) * float(screenSize.x - 1u);
        float sy = (ndc.y * 0.5 + 0.5) * float(screenSize.y - 1u);
        minPx = min(minPx, sx);
        maxPx = max(maxPx, sx);
        minPy = min(minPy, sy);
        maxPy = max(maxPy, sy);
        float z01 = ndc.z * 0.5 + 0.5;
        zObjMin = min(zObjMin, z01);
        anyIn = true;
    }

    if (!anyIn) {
        return false;
    }

    minPx = clamp(minPx, 0.0, float(screenSize.x - 1u));
    maxPx = clamp(maxPx, 0.0, float(screenSize.x - 1u));
    minPy = clamp(minPy, 0.0, float(screenSize.y - 1u));
    maxPy = clamp(maxPy, 0.0, float(screenSize.y - 1u));

    float wPix = max(maxPx - minPx, 1.0);
    float hPix = max(maxPy - minPy, 1.0);
    float maxDim = max(wPix, hPix);
    int lod = int(floor(log2(maxDim)));
    const int kHzbMaxMip = 15;
    lod = clamp(lod, 0, kHzbMaxMip);

    float sc = exp2(-float(lod));
    ivec2 tl = ivec2(floor(vec2(minPx, minPy) * sc));
    ivec2 br = ivec2(ceil(vec2(maxPx, maxPy) * sc));
    ivec2 msz = textureSize(s_HZB, lod);
    tl = clamp(tl, ivec2(0), msz - ivec2(1));
    br = clamp(br, ivec2(0), msz - ivec2(1));

    float hzMax = 0.0;
    for (int yy = tl.y; yy <= br.y; yy++) {
        for (int xx = tl.x; xx <= br.x; xx++) {
            hzMax = max(hzMax, texelFetch(s_HZB, ivec2(xx, yy), lod).r);
        }
    }

    const float bias = 0.0005;
    return zObjMin > hzMax + bias;
}

bool ShouldVisitChild(FHierarchyNodeSlice slice) {
    if (U_GlobalConstants.mMisc0.y == 0u) {
        return true;
    }
    return !IsNodeOccludedHZB(slice);
}

void main() {
    uint nodeOffset = CurrentWorkArgs.mData[5];
    uint nodeCount = CurrentWorkArgs.mData[6];

    uint nextNodeOffsetInBuffer = nodeOffset + nodeCount;
    uint nodeOutputOffset = nextNodeOffsetInBuffer;
    uint nextNodeCount = 0;

    uint clusterOutputOffset = CurrentWorkArgs.mData[1];

    for (int nodeIndexOffset = 0; nodeIndexOffset < nodeCount; nodeIndexOffset++) {
        uint currentNodeIndex =
            MainAndPostNodeAndClusterBatches.mData[nodeOffset + nodeIndexOffset];

        for (int i = 0; i < 4; i++) {
            FHierarchyNodeSlice slice = GetHierarchyNodeSlice(currentNodeIndex, i);
            uint currentSliceMipLevel = slice.NumPages;

            if (slice.bEnabled && ShouldVisitChild(slice)) {
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
