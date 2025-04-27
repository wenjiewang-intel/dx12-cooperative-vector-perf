#pragma once

#include <iostream>
#include <cassert>

enum DataType {
    DATA_TYPE_SINT16 = 2,           // ComponentType::I16
    DATA_TYPE_UINT16 = 3,           // ComponentType::U16
    DATA_TYPE_SINT32 = 4,           // ComponentType::I32
    DATA_TYPE_UINT32 = 5,           // ComponentType::U32
    DATA_TYPE_FLOAT16 = 8,          // ComponentType::F16
    DATA_TYPE_FLOAT32 = 9,          // ComponentType::F32
    DATA_TYPE_SINT8_T4_PACKED = 17, // ComponentType::PackedS8x32
    DATA_TYPE_UINT8_T4_PACKED = 18, // ComponentType::PackedU8x32
    DATA_TYPE_UINT8 = 19,           // ComponentType::U8
    DATA_TYPE_SINT8 = 20,           // ComponentType::I8
    DATA_TYPE_FLOAT8_E4M3 = 21,     // ComponentType::F8_E4M3
                                    // (1 sign, 4 exp, 3 mantissa bits)
    DATA_TYPE_FLOAT8_E5M2 = 22,     // ComponentType::F8_E5M2
                                    // (1 sign, 5 exp, 2 mantissa bits)
};

static void GetFloatExpManBits(DataType dt, uint32_t &expBits, uint32_t &manBits, uint32_t &byteSize)
{
    switch (dt) {
    case DATA_TYPE_FLOAT16:
        expBits = 5;
        manBits = 10;
        byteSize = 2;
        break;
    case DATA_TYPE_FLOAT8_E4M3:
        expBits = 4;
        manBits = 3;
        byteSize = 1;
        break;
    case DATA_TYPE_FLOAT8_E5M2:
        expBits = 5;
        manBits = 2;
        byteSize = 1;
        break;
    default:
        assert(0);
        break;
    }
}

uint32_t SizeofType(DataType dt)
{
    switch (dt)
    {
    case DATA_TYPE_FLOAT32: return 4;
    case DATA_TYPE_FLOAT16: return 2;
    case DATA_TYPE_FLOAT8_E4M3: return 1;
    case DATA_TYPE_FLOAT8_E5M2: return 1;
    default:
        assert(0);
        break;
    }
}

void SetDataFloat(void *ptr, DataType dataType, uint32_t offset, uint32_t index, float value)
{
    uint8_t *p = (uint8_t *)ptr;
    p += offset;

    switch (dataType) {
    case DATA_TYPE_FLOAT32:
        ((float *)p)[index] = value;
        break;
    case DATA_TYPE_FLOAT16:
    case DATA_TYPE_FLOAT8_E4M3:
    case DATA_TYPE_FLOAT8_E5M2:
        {
            uint32_t expBits, manBits, byteSize;
            GetFloatExpManBits(dataType, expBits, manBits, byteSize);
            uint32_t signBit = manBits + expBits;

		    uint32_t intVal = *(uint32_t *)&value;
            uint32_t sign = intVal & 0x80000000;
            int32_t exp = intVal & 0x7F800000;
            uint32_t mantissa = intVal & 0x007FFFFF;
            exp >>= 23;
            exp -= (1<<(8-1)) - 1;
            exp += (1<<(expBits-1)) - 1;
            exp &= (1<<expBits) - 1;
            // RTNE:
            if (mantissa & (1<<(23 - manBits))) {
                mantissa++;
            }
            mantissa += (1<<(22 - manBits)) - 1;
            if (mantissa & (1<<23)) {
                exp++;
                mantissa = 0;
            }
            mantissa >>= 23 - manBits;
            sign >>= 31;
            sign <<= signBit;
            exp <<= manBits;
            uint32_t result = sign | exp | mantissa;
            assert(result < (1ULL << (byteSize*8)));
            if (value == 0) {
                memset(&((uint8_t *)p)[index*byteSize], 0, byteSize);
            } else {
                memcpy(&((uint8_t *)p)[index*byteSize], &result, byteSize);
            }
        }
        break;
    default:
        assert(0);
        break;
    }
}

float GetDataFloat(void const *ptr, DataType dataType, uint32_t offset, uint32_t index)
{
    uint8_t *p = (uint8_t *)ptr;
    p += offset;

    switch (dataType) {
    case DATA_TYPE_FLOAT32:
        return ((float *)p)[index];
    case DATA_TYPE_FLOAT16:
    case DATA_TYPE_FLOAT8_E4M3:
    case DATA_TYPE_FLOAT8_E5M2:
        {
            uint32_t expBits, manBits, byteSize;
            GetFloatExpManBits(dataType, expBits, manBits, byteSize);
		    uint32_t intVal = 0;
            memcpy(&intVal, &((uint8_t *)p)[index*byteSize], byteSize);

            uint32_t signBit = manBits + expBits;
            uint32_t signMask = 1 << signBit;
            uint32_t expMask = ((1 << expBits) - 1) << manBits;

            uint32_t sign = intVal & signMask;
            uint32_t mantissa = intVal & ((1 << manBits) - 1);
            int32_t exp = (intVal & expMask) >> manBits;
            exp -= (1<<(expBits-1)) - 1;
            exp += (1<<(8-1)) - 1;
            exp &= 0xFF;
            exp <<= 23;
            mantissa <<= 23 - manBits;
            sign <<= 31 - signBit;
            uint32_t result = sign | exp | mantissa;
            float ret = (intVal == 0 || intVal == signMask) ? 0.0f : *(float *)&result;
            return ret;
        }
    default:
        assert(0);
        return 0.f;
	}
}

void MatMulAdd(
    DataType dataType,
    void *outputVec,
    void const *matrix,
    void const *inputVec,
    void const *biasVec,
    uint32_t sizeM, uint32_t sizeK,
    uint32_t strideK
)
{
    for (uint32_t m = 0; m < sizeM; ++m) {
        float sum = 0.0f;
        for (uint32_t k = 0; k < sizeK; ++k) {
            float a = GetDataFloat(inputVec, dataType, 0, k);
            float b = GetDataFloat(matrix, dataType, m * strideK, k);
            sum += a * b;
        }
        float bias = GetDataFloat(biasVec, dataType, 0, m);
        sum += bias;
        SetDataFloat(outputVec, dataType, 0, m, sum);
    }
}
