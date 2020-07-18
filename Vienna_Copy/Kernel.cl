
__kernel void av_cpu( 
  __global float * vec1, 
  uint4 size1, 
  float fac2, 
  __global const float * vec2, 
  uint4 size2
  ){ 

  float alpha = fac2; 

  for (unsigned int i = get_global_id(0); i < size1.z; i += get_global_size(0)){
      vec1[i] = vec2[i] * alpha ; 
  }

} 