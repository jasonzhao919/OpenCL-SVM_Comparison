__kernel void GEMV( const __global int* A,
                      const __global int* B,
                      __global int* C,
                      const int M, const int N
                      ) {
    
  const int globalRow = get_global_id(0); // Row ID of C (0..M)

  // Compute a single element (loop over K)
  int temp = 0;
  for (int k = 0; k < N; k++) {
    temp += A[globalRow * N + k] * B[k];
  }

  // Store the result
  C[globalRow] = temp;
}
