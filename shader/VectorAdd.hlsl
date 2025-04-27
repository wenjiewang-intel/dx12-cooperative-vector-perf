// HLSL kernel for vector addition
cbuffer Constants : register(b0) {
    uint vectorSize;
};

StructuredBuffer<float> InputA : register(t0);
StructuredBuffer<float> InputB : register(t1);
RWStructuredBuffer<float> Output : register(u0);

[numthreads(256, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID) {
    uint id = DTid.x;
    if (id < vectorSize) {
        Output[id] = InputA[id] + InputB[id];
        // Output[id] = InputA[id];
    }
    // Output[id] = 3;
}