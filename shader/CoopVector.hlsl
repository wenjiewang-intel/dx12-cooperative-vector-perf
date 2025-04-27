#include "linalg.h"

ByteAddressBuffer ROBuffer;
RWByteAddressBuffer RWBuffer0;

[numthreads(256, 1, 1)]
void main() {
  using namespace dx::linalg;

  MatrixRef<DATA_TYPE_FLOAT16, 4, 4, MATRIX_LAYOUT_MUL_OPTIMAL, true> MatrixA =
      {ROBuffer, /*offset=*/128, /*stride=*/0};

  MatrixRef<DATA_TYPE_FLOAT16, 4, 4, MATRIX_LAYOUT_ROW_MAJOR, true>
      MatrixB = {ROBuffer, /*offset=*/128, /*stride=*/16};

  RWMatrixRef<DATA_TYPE_FLOAT16, 128, 256, MATRIX_LAYOUT_OUTER_PRODUCT_OPTIMAL>
      MatrixC = {RWBuffer0, /*offset=*/64, /*stride=*/0};
    
  vector<float16_t, 32> vec1;
}