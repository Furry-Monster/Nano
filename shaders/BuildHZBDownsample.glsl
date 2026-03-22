#version 450

layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(binding = 0, r32f) uniform readonly image2D SrcMip;
layout(binding = 1, r32f) uniform writeonly image2D DstMip;

void main() {
    ivec2 dst = ivec2(gl_GlobalInvocationID.xy);
    ivec2 dstDim = imageSize(DstMip);
    if (dst.x >= dstDim.x || dst.y >= dstDim.y) {
        return;
    }

    ivec2 srcCoord = dst * 2;
    ivec2 srcDim = imageSize(SrcMip);
    float m = 0.0;
    for (int dy = 0; dy < 2; dy++) {
        for (int dx = 0; dx < 2; dx++) {
            ivec2 s = srcCoord + ivec2(dx, dy);
            if (s.x < srcDim.x && s.y < srcDim.y) {
                m = max(m, imageLoad(SrcMip, s).r);
            }
        }
    }
    imageStore(DstMip, dst, vec4(m, 0.0, 0.0, 0.0));
}
