#version 450
// Parallel copy: one invocation per cluster pair (2 uints)
layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;

#define NANITE_MAX_VISIBLE_CLUSTERS 932

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
    uint i = gl_GlobalInvocationID.x;
    if (i >= NANITE_MAX_VISIBLE_CLUSTERS)
        return;
    VisibleClusterSHWH.mData[i * 2] =
        MainAndPostNodeAndClusterBatches.mData[1024 + i * 2];
    VisibleClusterSHWH.mData[i * 2 + 1] =
        MainAndPostNodeAndClusterBatches.mData[1024 + i * 2 + 1];
}
