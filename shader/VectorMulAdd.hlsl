ByteAddressBuffer input_vector_buffer : register(t0);
ByteAddressBuffer matrix_buffer : register(t1);
ByteAddressBuffer bias_buffer : register(t2);
RWByteAddressBuffer output_vector_buffer : register(u0);

// StructuredBuffer<float> input_vector_buffer : register(t0);
// StructuredBuffer<float> matrix_buffer : register(t1);
// StructuredBuffer<float> bias_buffer : register(t2);
// RWStructuredBuffer<float> output_vector_buffer : register(u0);

#define BYTES_OF_ITY 4
#define BYTES_OF_OTY 4
#define M 8
#define K 8
#define STRIDE_K 32

[numthreads(4, 1, 1)]
void main(uint3 DTid : SV_DispatchThreadID)
{
    uint m = DTid.x;
    // for (uint32_t m = 0; m < M; m++) {
        float sum = 0.0f;
        for (uint32_t k = 0; k < K; k++) {
            float v1 = asfloat(matrix_buffer.Load(m * STRIDE_K + k * BYTES_OF_ITY));
            float v2 = asfloat(input_vector_buffer.Load(k * BYTES_OF_ITY));
            sum += v1 * v2;
        }
        float bias = asfloat(bias_buffer.Load(m * BYTES_OF_ITY));
        sum += bias;
        output_vector_buffer.Store(m * BYTES_OF_OTY, sum);
    // }
}