#version 450

layout(binding = 0) uniform GlobalConstants {
    mat4 mProjectionMatrix;
    mat4 mViewMatrix;
    mat4 mModelMatrix;
    uvec4 mMisc0;
    vec4 mNanite_ViewOrigin;
    vec4 mNanite_ViewForward;
} U_GlobalConstants;

layout(std430, binding = 1) readonly buffer FClusterPageData {
    uint mData[];
} ClusterPageData;

layout(std430, binding = 2) readonly buffer FVisibleClusterSHWH {
    uint mData[];
} VisibleClusterSHWH;

layout(location = 0) flat out uvec4 V_PackedData;

struct ClusterInfo {
    uint mBaseOffset;
    uint mIndexOffsetLocal;
    uint mIndexCount;
    vec4 mLODBounds;
    float mLODError;
    float mEdgeLength;
};

ClusterInfo GetClusterInfo(uint inPageIndex, uint inClusterIndex) {
    ClusterInfo info;

    uint pageCount = ClusterPageData.mData[0];
    uint pageBaseOffsetInBytes = ClusterPageData.mData[1u + inPageIndex];
    uint pageBaseOffset = pageBaseOffsetInBytes / 4;

    uint clusterCountOnPage = ClusterPageData.mData[pageBaseOffset];
    uint clusterBaseOffsetInBytesLocal =
        ClusterPageData.mData[pageBaseOffset + 1u + inClusterIndex];
    uint clusterBaseOffset =
        pageBaseOffset + 1u + clusterCountOnPage + clusterBaseOffsetInBytesLocal / 4;

    uint clusterIndexDataOffsetLocal = ClusterPageData.mData[clusterBaseOffset] / 4;
    uint clusterIndexCount = ClusterPageData.mData[clusterBaseOffset + 1u];

    uvec4 lodBounds = uvec4(
            ClusterPageData.mData[clusterBaseOffset + 2u],
            ClusterPageData.mData[clusterBaseOffset + 3u],
            ClusterPageData.mData[clusterBaseOffset + 4u],
            ClusterPageData.mData[clusterBaseOffset + 5u]
        );

    uint lodErrorAndEdgeLength = ClusterPageData.mData[clusterBaseOffset + 6u];

    info.mBaseOffset = clusterBaseOffset;
    info.mIndexOffsetLocal = clusterIndexDataOffsetLocal;
    info.mIndexCount = clusterIndexCount;
    info.mLODBounds = uintBitsToFloat(lodBounds);

    vec2 unpacked2Half = unpackHalf2x16(lodErrorAndEdgeLength);
    info.mLODError = unpacked2Half.x;
    info.mEdgeLength = unpacked2Half.y;

    return info;
}

void main() {
    uint clusterIndex = gl_InstanceIndex;
    uint vertexIndex = gl_VertexIndex;

    uint pageIndex = VisibleClusterSHWH.mData[clusterIndex * 2];
    uint clusterIndexOnPage = VisibleClusterSHWH.mData[clusterIndex * 2 + 1];

    ClusterInfo info = GetClusterInfo(pageIndex, clusterIndexOnPage);

    uint currentVertexIndexOffsetBase = info.mBaseOffset + info.mIndexOffsetLocal;
    uint currentVertexIndexOffset = currentVertexIndexOffsetBase + vertexIndex;
    uint currentIndexInCluster = ClusterPageData.mData[currentVertexIndexOffset];

    uint currentClusterPositionDataOffsetBase = info.mBaseOffset + 7u;
    uint currentVertexPositionDataOffset =
        currentClusterPositionDataOffsetBase + currentIndexInCluster * 3u;

    vec3 positionMS = uintBitsToFloat(uvec3(
                ClusterPageData.mData[currentVertexPositionDataOffset],
                ClusterPageData.mData[currentVertexPositionDataOffset + 1],
                ClusterPageData.mData[currentVertexPositionDataOffset + 2]
            ));

    vec4 positionCS = vec4(0.0, 0.0, 0.0, 0.0);
    if (vertexIndex < info.mIndexCount) {
        // Model -> Translated World -> View -> Clip
        vec4 positionWS = U_GlobalConstants.mModelMatrix * vec4(positionMS, 1.0);
        positionWS = vec4(positionWS.xyz - U_GlobalConstants.mNanite_ViewOrigin.xyz, 1.0);
        vec4 positionVS = U_GlobalConstants.mViewMatrix * positionWS;
        positionCS = U_GlobalConstants.mProjectionMatrix * positionVS;
        V_PackedData.x = (pageIndex << 8) | (clusterIndexOnPage + 1);
    }

    gl_Position = positionCS;
}
