#version 450
#extension GL_ARB_gpu_shader_int64 : enable

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(std430, binding = 0) readonly buffer FVisBuffer64 {
    uint64_t mData[];
} VisBuffer64;

layout(binding = 1, r32f) uniform writeonly image2D HZBMip0;

layout(binding = 2) uniform GlobalConstants {
    mat4 mProjectionMatrix;
    mat4 mViewMatrix;
    mat4 mModelMatrix;
    uvec4 mMisc0;
    vec4 mNanite_ViewOrigin;
    vec4 mNanite_ViewForward;
} UBO;

void main() {
    uvec2 dim = UBO.mMisc0.zw;
    if (dim.x == 0u || dim.y == 0u) {
        return;
    }

    ivec2 p = ivec2(gl_GlobalInvocationID.xy);
    if (p.x >= int(dim.x) || p.y >= int(dim.y)) {
        return;
    }

    int idx = p.y * int(dim.x) + p.x;
    uint64_t v = VisBuffer64.mData[idx];
    uint hi = uint(v >> 32u);
    float z;
    if (hi == 0xFFFFFFFFu) {
        z = 1.0;
    } else {
        z = uintBitsToFloat(hi);
    }
    imageStore(HZBMip0, p, vec4(z, 0.0, 0.0, 0.0));
}
