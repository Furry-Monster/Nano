#version 450
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0) uniform GlobalConstants {
    mat4 mProjectionMatrix;
    mat4 mViewMatrix;
    mat4 mModelMatrix;
    uvec4 mMisc0;
    vec4 mNanite_ViewOrigin;
    vec4 mNanite_ViewForward;
};

layout(std430, binding = 1) buffer FMainAndPostNodeAndClusterBatches {
    uint mData[];
} MainAndPostNodeAndClusterBatches;

layout(std430, binding = 2) buffer FVisibleClusterSHWH {
    uint mData[];
} VisibleClusterSHWH;

void main() {
    // Pass-through: copy visible clusters from batches to output buffer
    for (int i = 0; i < 932; i++) {
        VisibleClusterSHWH.mData[i * 2] =
            MainAndPostNodeAndClusterBatches.mData[1024 + i * 2];
        VisibleClusterSHWH.mData[i * 2 + 1] =
            MainAndPostNodeAndClusterBatches.mData[1024 + i * 2 + 1];
    }
}
