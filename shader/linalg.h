// reference: https://github.com/microsoft/DirectXShaderCompiler/blob/d81fd22622e5f34f991964f3e77852ab7e310c37/tools/clang/lib/Headers/hlsl/dx/linalg.h
// From PR: https://github.com/microsoft/DirectXShaderCompiler/pull/7350/files

// Document: https://github.com/microsoft/hlsl-specs/blob/main/proposals/0031-hlsl-vector-matrix-operations.md

// Header for linear algebra APIs.

#if __spirv__
#error "Cooperative vectors not (yet) supported for SPIRV"
#endif

#if ((__SHADER_TARGET_MAJOR > 6) ||                                            \
     (__SHADER_TARGET_MAJOR == 6 && __SHADER_TARGET_MINOR >= 9)) &&            \
    (__HLSL_VERSION >= 2021)

namespace dx {
namespace linalg {

// NOTE: can't be an enum class because we get this error:
//     error: non-type template argument of type 'dx::linalg::DataType' is not
//     an integral constant expression
//
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

enum MatrixLayout {
  MATRIX_LAYOUT_ROW_MAJOR = 0,
  MATRIX_LAYOUT_COLUMN_MAJOR = 1,
  MATRIX_LAYOUT_MUL_OPTIMAL = 2,
  MATRIX_LAYOUT_OUTER_PRODUCT_OPTIMAL = 3
};

//
// Helper for signedness
//
namespace details {
template <typename T> bool IsUnsigned() { return false; }

#ifdef __HLSL_ENABLE_16_BIT
template <> bool IsUnsigned<uint16_t>() { return true; }
#endif

template <> bool IsUnsigned<uint32_t>() { return true; }
template <> bool IsUnsigned<uint64_t>() { return true; }
} // namespace details

//
// (RW)MatrixRef
//

template <typename BufferTy, DataType DT, uint M, uint K, MatrixLayout ML,
          bool Transpose>
struct MatrixRefImpl {
  BufferTy Buffer;
  uint StartOffset;
  uint Stride;
};

template <DataType DT, uint M, uint K, MatrixLayout ML, bool Transpose = false>
using MatrixRef = MatrixRefImpl<ByteAddressBuffer, DT, M, K, ML, Transpose>;

template <DataType DT, uint M, uint K, MatrixLayout ML, bool Transpose = false>
using RWMatrixRef = MatrixRefImpl<RWByteAddressBuffer, DT, M, K, ML, Transpose>;

//
// (RW)VectorRef
//

template <typename BufferTy, DataType DT> struct VectorRefImpl {
  BufferTy Buffer;
  uint StartOffset;
};

template <DataType DT> using VectorRef = VectorRefImpl<ByteAddressBuffer, DT>;

template <DataType DT>
using RWVectorRef = VectorRefImpl<RWByteAddressBuffer, DT>;

//
// Vector
//

template <typename T, int N, DataType DT> struct InterpretedVector {
  vector<T, N> Data;
};

template <DataType DT, typename T, int N>
InterpretedVector<T, N, DT> MakeInterpretedVector(vector<T, N> Vec) {
  InterpretedVector<T, N, DT> IV = {Vec};
  return IV;
}

//
// Mul
//

template <typename OutputElTy, typename InputElTy, int InputElCount,
          typename MatrixBufferTy, DataType InputDT, DataType MatrixDT,
          uint MatrixM, uint MatrixK, MatrixLayout MatrixLayout,
          bool MatrixTranspose>
vector<OutputElTy, MatrixM>
Mul(MatrixRefImpl<MatrixBufferTy, MatrixDT, MatrixM, MatrixK, MatrixLayout,
                  MatrixTranspose>
        Matrix,
    InterpretedVector<InputElTy, InputElCount, InputDT> InputVector) {

  vector<OutputElTy, MatrixM> OutputVector;

  __builtin_MatVecMul(
      /*out*/ OutputVector, details::IsUnsigned<OutputElTy>(), InputVector.Data,
      details::IsUnsigned<InputElTy>(), InputDT, Matrix.Buffer,
      Matrix.StartOffset, MatrixDT, MatrixM, MatrixK, MatrixLayout,
      MatrixTranspose, Matrix.Stride);

  return OutputVector;
}

//
// MulAdd
//

template <typename OutputElTy, typename InputElTy, int InputElCount,
          typename MatrixBufferTy, DataType InputDT, DataType MatrixDT,
          uint MatrixM, uint MatrixK, MatrixLayout MatrixLayout,
          bool MatrixTranspose, typename BiasVectorBufferTy,
          DataType BiasVectorDT>
vector<OutputElTy, MatrixM>
MulAdd(MatrixRefImpl<MatrixBufferTy, MatrixDT, MatrixM, MatrixK, MatrixLayout,
                     MatrixTranspose>
           Matrix,
       InterpretedVector<InputElTy, InputElCount, InputDT> InputVector,
       VectorRefImpl<BiasVectorBufferTy, BiasVectorDT> BiasVector) {

  vector<OutputElTy, MatrixM> OutputVector;

  __builtin_MatVecMulAdd(
      /*out*/ OutputVector, details::IsUnsigned<OutputElTy>(), InputVector.Data,
      details::IsUnsigned<InputElTy>(), InputDT, Matrix.Buffer,
      Matrix.StartOffset, MatrixDT, MatrixM, MatrixK, MatrixLayout,
      MatrixTranspose, Matrix.Stride, BiasVector.Buffer, BiasVector.StartOffset,
      BiasVectorDT);

  return OutputVector;
}

//
// OuterProductAccumulate
//

template <typename ElTy, int MatrixM, int MatrixN, DataType MatrixDT,
          MatrixLayout MatrixLayout>
void OuterProductAccumulate(
    vector<ElTy, MatrixM> InputVector1, vector<ElTy, MatrixN> InputVector2,
    RWMatrixRef<MatrixDT, MatrixM, MatrixN, MatrixLayout, false> Matrix) {
  __builtin_OuterProductAccumulate(InputVector1, InputVector2, Matrix.Buffer,
                                   Matrix.StartOffset, MatrixDT, MatrixLayout,
                                   Matrix.Stride);
}

//
// VectorAccumulate
//

template <typename ElTy, int ElCount>
void VectorAccumulate(vector<ElTy, ElCount> InputVector,
                      RWByteAddressBuffer Buffer, uint Offset) {
  __builtin_VectorAccumulate(InputVector, Buffer, Offset);
}

} // namespace linalg
} // namespace dx

#endif // SM 6.9 check and HV version check
