#version 450

layout(binding = 0) uniform GlobalConstants {
    mat4 mProjectionMatrix;
    mat4 mViewMatrix;
    mat4 mModelMatrix;
    uvec4 mMisc0;
    vec4 mNanite_ViewOrigin;
    vec4 mNanite_ViewForward;
} U_GlobalConstants;

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

    int screenW = int(U_GlobalConstants.mMisc0.z);
    int pixelIndex = texcoord.y * screenW + texcoord.x;

    uint oldDepth = atomicMin(VisBufferDepth.mData[pixelIndex], depthBits);
    if (depthBits <= oldDepth) {
        VisBufferID.mData[pixelIndex] = clusterInfo;
    }
}
