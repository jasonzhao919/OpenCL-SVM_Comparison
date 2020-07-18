__kernel void SVMhelloworld(__global char* in, __global char* out)
{
	int num = get_global_id(0);
	out[num] = in[num];
}

__kernel void MatMul( const __global int* A,
                      const __global int* B,
                      __global int* C,
                      const int M, const int N, const int K
                      ) {
    
  const int globalRow = get_global_id(0); // Row ID of C (0..M)
  const int globalCol = get_global_id(1); // Col ID of C (0..N)

  // Compute a single element (loop over K)
  int temp = 0;
  for (int k = 0; k < K; k++) {
    temp += A[globalRow * N + k] * B[ k * K + globalCol];
  }

  // Store the result
  C[globalCol * M + globalRow] = temp;
}
