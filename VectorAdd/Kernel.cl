kernel void vector_add(__global float *a, __global float *b, __global float *res, uint vector_size){
    for ( uint i = get_global_id(0); i < vector_size; i += get_global_size(0))
        res[i] = a[i] + b[i]; 
}
