#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(std430, binding = 0) buffer FWorkArgs0 {
    uint mData[];
} WorkArgs0;

layout(std430, binding = 1) buffer FWorkArgs1 {
    uint mData[];
} WorkArgs1;

layout(std430, binding = 2) buffer FMainAndPostNodeAndClusterBatches {
    uint mData[];
} MainAndPostNodeAndClusterBatches;

layout(std430, binding = 3) buffer FVisBufferDepth {
    uint mData[];
} VisBufferDepth;

layout(std430, binding = 4) buffer FVisBufferID {
    uint mData[];
} VisBufferID;

layout(binding = 5) uniform GlobalConstants {
    mat4 mProjectionMatrix;
    mat4 mViewMatrix;
    mat4 mModelMatrix;
    uvec4 mMisc0;
    vec4 mNanite_ViewOrigin;
    vec4 mNanite_ViewForward;
} UBO;

void main() {
    uint screenW = UBO.mMisc0.z;
    uint screenH = UBO.mMisc0.w;
    ivec2 texcoord = ivec2(gl_GlobalInvocationID.xy);
    if (texcoord.x >= int(screenW) || texcoord.y >= int(screenH)) {
        return;
    }

    if (texcoord.x == 0 && texcoord.y == 0) {
        // WorkArgs layout:
        // [0]=vertexCount(384),[1] = clusterOutputOffset,
        // [2..4]=unused,[5] = nodeOffset,[6] = nodeCount
        WorkArgs0.mData[0] = 384u;
        WorkArgs0.mData[1] = 0u;
        WorkArgs0.mData[2] = 0u;
        WorkArgs0.mData[3] = 0u;
        WorkArgs0.mData[5] = 0u;
        WorkArgs0.mData[6] = 1u;

        WorkArgs1.mData[0] = 384u;
        WorkArgs1.mData[1] = 0u;
        WorkArgs1.mData[2] = 0u;
        WorkArgs1.mData[3] = 0u;
        WorkArgs1.mData[5] = 0u;
        WorkArgs1.mData[6] = 1u;

        MainAndPostNodeAndClusterBatches.mData[0] = 0u;
    }

    int pixelIndex = texcoord.y * int(screenW) + texcoord.x;
    VisBufferDepth.mData[pixelIndex] = 0xFFFFFFFFu;
    VisBufferID.mData[pixelIndex] = 0u;
}
