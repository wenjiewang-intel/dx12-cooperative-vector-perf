#pragma once

typedef enum D3D12_FEATURE {
    ...
    // Contains Cooperative Vector tier.
    // NN tbd when implemented
    D3D12_FEATURE_D3D12_OPTIONSNN;
    D3D12_FEATURE_COOPERATIVE_VECTOR;
};

// This is designed to match the ComponentType enum values but omits data 
// types that are not currently specified to work with this API. The names are chosen
// to more closely match those used by HLSL developers, as opposed to the ComponentType 
// names that align with LLVM IR.

typedef enum D3D12_LINEAR_ALGEBRA_DATATYPE {
  D3D12_LINEAR_ALGEBRA_DATATYPE_SINT16          =  2, // ComponentType::I16
  D3D12_LINEAR_ALGEBRA_DATATYPE_UINT16          =  3, // ComponentType::U16
  D3D12_LINEAR_ALGEBRA_DATATYPE_SINT32          =  4, // ComponentType::I32
  D3D12_LINEAR_ALGEBRA_DATATYPE_UINT32          =  5, // ComponentType::U32
  D3D12_LINEAR_ALGEBRA_DATATYPE_FLOAT16         =  8, // ComponentType::F16
  D3D12_LINEAR_ALGEBRA_DATATYPE_FLOAT32         =  9, // ComponentType::F32
  D3D12_LINEAR_ALGEBRA_DATATYPE_SINT8_T4_PACKED = 17, // ComponentType::PackedS8x32
  D3D12_LINEAR_ALGEBRA_DATATYPE_UINT8_T4_PACKED = 18, // ComponentType::PackedU8x32
  D3D12_LINEAR_ALGEBRA_DATATYPE_UINT8           = 19, // ComponentType::U8
  D3D12_LINEAR_ALGEBRA_DATATYPE_SINT8           = 20, // ComponentType::I8
  D3D12_LINEAR_ALGEBRA_DATATYPE_E4M3            = 21, // ComponentType::F8_E4M3 (1 sign, 4 exp, 3 mantissa bits)
  D3D12_LINEAR_ALGEBRA_DATATYPE_E5M2            = 22, // ComponentType::F8_E5M2 (1 sign, 5 exp, 2 mantissa bits)
};

typedef enum D3D12_COOPERATIVE_VECTOR_TIER
{
    D3D12_COOPERATIVE_VECTOR_TIER_NOT_SUPPORTED,    
    D3D12_COOPERATIVE_VECTOR_TIER_1_0,
    D3D12_COOPERATIVE_VECTOR_TIER_1_1
}

// This struct may be augmented with more capability bits
// as the feature develops
typedef struct D3D12_FEATURE_DATA_D3D12_OPTIONSNN // NN tbd when implemented
{
    Out D3D12_COOPERATIVE_VECTOR_TIER CooperativeVectorTier;
} D3D12_FEATURE_DATA_D3D12_OPTIONSNN;

// Used for MatrixVectorMulAdd intrinsic
typedef struct D3D12_COOPERATIVE_VECTOR_PROPERTIES_MUL
{
    D3D12_LINEAR_ALGEBRA_DATATYPE InputType;
    D3D12_LINEAR_ALGEBRA_DATATYPE InputInterpretation;
    D3D12_LINEAR_ALGEBRA_DATATYPE MatrixInterpretation;
    D3D12_LINEAR_ALGEBRA_DATATYPE BiasInterpretation;
    D3D12_LINEAR_ALGEBRA_DATATYPE OutputType;
    BOOL                          TransposeSupported;
};

// Used for OuterProductAccumulate and VectorAccumulate intrinsics
typedef struct D3D12_COOPERATIVE_VECTOR_PROPERTIES_ACCUMULATE
{
    D3D12_LINEAR_ALGEBRA_DATATYPE InputType;  
    D3D12_LINEAR_ALGEBRA_DATATYPE AccumulationType;
};

// CheckFeatureSupport data struct used with type D3D12_FEATURE_COOPERATIVE_VECTOR:
typedef struct D3D12_FEATURE_DATA_COOPERATIVE_VECTOR
{    
    InOut UINT                                          MatrixVectorMulAddPropCount;
    Out D3D12_COOPERATIVE_VECTOR_PROPERTIES_MUL*        pMatrixVectorMulAddProperties;
    InOut UINT                                          OuterProductAccumulatePropCount;
    Out D3D12_COOPERATIVE_VECTOR_PROPERTIES_ACCUMULATE* pOuterProductAccumulateProperties;
    InOut UINT                                          VectorAccumulatePropCount;
    Out D3D12_COOPERATIVE_VECTOR_PROPERTIES_ACCUMULATE* pVectorAccumulateProperties;
};
