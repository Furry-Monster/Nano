#version 450

layout(std430, binding = 3) buffer FVisBufferDepth {
    uint mData[];
} VisBufferDepth;

layout(std430, binding = 4) buffer FVisBufferID {
    uint mData[];
} VisBufferID;

layout(location = 0) flat in uvec4 V_PackedData;

void main() {
    ivec2 texcoord = ivec2(gl_FragCoord.xy);
    float z = gl_FragCoord.z;

    uint depthBits = floatBitsToUint(z);
    uint clusterInfo = V_PackedData.x;

    int pixelIndex = texcoord.y * 1280 + texcoord.x;

    uint oldDepth = atomicMin(VisBufferDepth.mData[pixelIndex], depthBits);
    if (depthBits <= oldDepth) {
        VisBufferID.mData[pixelIndex] = clusterInfo;
    }
}
