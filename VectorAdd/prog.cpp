/**********************************************************************
Copyright ©2014 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

	Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
	Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/

// For clarity,error checking has been omitted.

#include <CL/cl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <exception>

#define SUCCESS 0
#define FAILURE 1

using namespace std;

const int SIZE = 100000000;
    
/* convert the kernel file into a string */
int convertToString(const char *filename, std::string& s)
{
	size_t size;
	char*  str;
	std::fstream f(filename, (std::fstream::in | std::fstream::binary));

	if(f.is_open())
	{
		size_t fileSize;
		f.seekg(0, std::fstream::end);
		size = fileSize = (size_t)f.tellg();
		f.seekg(0, std::fstream::beg);
		str = new char[size+1];
		if(!str)
		{
			f.close();
			return 0;
		}

		f.read(str, fileSize);
		f.close();
		str[size] = '\0';
		s = str;
		delete[] str;
		return 0;
	}
	cout<<"Error: failed to open file\n:"<<filename<<endl;
	return FAILURE;
}

void vector_add_svm();
void vector_add_non_svm();


int main(int argc, char* argv[])
{

  std::cout << "SVM \n------------------------------ \n" << std::endl;
  {
    vector_add_svm();
  }
  
  
  std::cout << "\n\n" << "Non-SVM \n------------------------------ " << std::endl;
  {
    vector_add_non_svm();
  }
}



void vector_add_svm(){

  const float ELEMENTS = 100000000;
	const float DATA_SIZE = ELEMENTS * sizeof(float);
 
/*Step1: Getting platforms and choose an available one.*/
	cl_uint numPlatforms;	//the NO. of platforms
	cl_platform_id platform = NULL;	//the chosen platform
	cl_int	status = clGetPlatformIDs(0, NULL, &numPlatforms);
	if (status != CL_SUCCESS)
	{
		cout << "Error: Getting platforms!" << endl;
	}

	/*For clarity, choose the first available platform. */
	if(numPlatforms > 0)
	{
		cl_platform_id* platforms = (cl_platform_id* )malloc(numPlatforms* sizeof(cl_platform_id));
		status = clGetPlatformIDs(numPlatforms, platforms, NULL);
		platform = platforms[0];
		free(platforms);
	}

/*Step 2:Query the platform and choose the first GPU device if has one.Otherwise use the CPU as device.*/
	cl_uint				numDevices = 0;
	cl_device_id        *devices;
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);	
	if (numDevices == 0)	//no GPU available.
	{
		cout << "No GPU device available." << endl;
		cout << "Choose CPU as default device." << endl;
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 0, NULL, &numDevices);	
		devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, numDevices, devices, NULL);
	}
	else
	{
		devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
	}
	

/*Step 3: Create context.*/
	cl_context context = clCreateContext(NULL,1, devices,NULL,NULL,NULL);
	
/*Step 4: Creating command queue associate with the context.*/
	/*old func. code
		cl_command_queue commandQueue = clCreateCommandQueue(context, devices[0], 0, NULL);
    	*/
	/*new func.
    	*	cl_command_queue clCreateCommandQueueWithProperties(
			cl_context context, 
			cl_device_id device, 
			const cl_queue_properties *properties, 
			cl_int *errcode_ret)
	*/
	//cl_command_queue commandQueue = clCreateCommandQueueWithProperties(context, devices[0], 0, NULL);
	cl_command_queue commandQueue = clCreateCommandQueue(context, devices[0], CL_QUEUE_PROFILING_ENABLE, NULL);

/*Step 5: Create program object */
	const char *filename = "Kernel.cl";
	string sourceStr;
	status = convertToString(filename, sourceStr);
	const char *source = sourceStr.c_str();
	size_t sourceSize[] = {strlen(source)};
	cl_program program = clCreateProgramWithSource(context, 1, &source, sourceSize, NULL);
	
/*Step 6: Build program. */
	const char options[] = "-cl-std=CL2.0";
	status = clBuildProgram(program, 1,devices, options,NULL,NULL);

/*Step 7: Create kernel object */
	cl_kernel kernel = clCreateKernel(program, "vector_add", NULL);
	
/*Step 8: Initial input,output for the host and create SVM buffer*/
  auto start_time = chrono::high_resolution_clock::now();
  
  int szA = SIZE;
  int szB = SIZE;
  int szC = SIZE;
	
  float *A = (float *)clSVMAlloc( context, CL_MEM_READ_WRITE, szA * sizeof(float), 0 );
  float *B = (float *)clSVMAlloc( context, CL_MEM_READ_WRITE, szB * sizeof(float), 0 );
	float *C = (float *)clSVMAlloc( context, CL_MEM_READ_WRITE, szC * sizeof(float), 0 );

 
	status = clEnqueueSVMMap(commandQueue, CL_TRUE, CL_MAP_WRITE_INVALIDATE_REGION, A, DATA_SIZE, 0, NULL, NULL);
  status = clEnqueueSVMMap(commandQueue, CL_TRUE, CL_MAP_WRITE_INVALIDATE_REGION, B, DATA_SIZE, 0, NULL, NULL);
  
	status = clEnqueueSVMUnmap(commandQueue, A, 0, NULL, NULL);
  status = clEnqueueSVMUnmap(commandQueue, B, 0, NULL, NULL);
 
/*Step 9: Sets Kernel arguments.*/
	status = clSetKernelArgSVMPointer(kernel, 0, A);
 	
  status = clSetKernelArgSVMPointer(kernel, 1, B);

  status = clSetKernelArgSVMPointer(kernel, 2, C);
 	
  status = clSetKernelArg(kernel, 3, sizeof(cl_uint), &SIZE);
   
  
 
/*Step 10: Running the kernel.*/
	size_t global_work_size[1] = {SIZE};
  
	status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, NULL);
  
  clFinish(commandQueue);
 
  status = clEnqueueSVMMap(commandQueue, CL_TRUE, CL_MAP_WRITE_INVALIDATE_REGION, C, szC * sizeof(int), 0, NULL, NULL);
  
  auto end_time = chrono::high_resolution_clock::now();
  chrono::duration<double> time_duration = end_time-start_time;
  cout << "SVM vector_add takes: " << time_duration.count() << " s" <<endl;

/*Step 11: Test if kernel works and char. are all assign to outputBuffer*/
/*	
	cout << "\nA :" << endl;
  for(int i = 0; i < szA; i++){
    cout << setw(4) << (int)A[i] << " ";
    
    if( ((i+1) % Ndim) == 0 ){
      cout << endl;
    }    
  }
    
  cout << "\nB :" << endl;
  for(int i = 0; i < szB; i++){

    cout << setw(4) << (int)B[i] << " ";
    if( ((i+1) % Pdim) == 0 ){
      cout << endl;
    }    
  }

	cout << "\nC :" << endl;
  for(int i = 0; i < szC; i++){

    cout << setw(4) << c[i] << " ";
    if( ((i+1) % Pdim) == 0 ){
      cout << endl;
    }   
  }
*/
  status = clEnqueueSVMUnmap(commandQueue, C, 0, NULL, NULL);

/*Step 12: Clean the resources.*/
	clSVMFree(context, A);  
  clSVMFree(context, B); 
	clSVMFree(context, C);  
	status = clReleaseKernel(kernel);				//Release kernel.
	status = clReleaseProgram(program);				//Release the program object.
	status = clReleaseCommandQueue(commandQueue);	//Release  Command queue.
	status = clReleaseContext(context);				//Release context.

	if (devices != NULL)
	{
		free(devices);
		devices = NULL;
	}

	cout<<"Program passed!\n";
}



void vector_add_non_svm(){

/*Step1: Getting platforms and choose an available one.*/
	cl_uint numPlatforms; //the NO. of platforms
	cl_platform_id platform = NULL; //the chosen platform
	cl_int status = clGetPlatformIDs(0, NULL, &numPlatforms);
	if (status != CL_SUCCESS)
	{
		cout << "Error: Getting platforms!" << endl;
	}

	/*For clarity, choose the first available platform. */
	if (numPlatforms > 0)
	{
		cl_platform_id* platforms = 
                     (cl_platform_id*)malloc(numPlatforms * sizeof(cl_platform_id));
		status = clGetPlatformIDs(numPlatforms, platforms, NULL);
		platform = platforms[0];
		free(platforms);
	}

	/*Step 2:Query the platform and choose the first GPU device if has one.Otherwise use the CPU as device.*/
	cl_uint numDevices = 0;
	cl_device_id        *devices;
	status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 0, NULL, &numDevices);
	if (numDevices == 0) //no GPU available.
	{
		cout << "No GPU device available." << endl;
		cout << "Choose CPU as default device." << endl;
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, 0, NULL, &numDevices);
		devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_CPU, numDevices, devices, NULL);
	}
	else
	{
		devices = (cl_device_id*)malloc(numDevices * sizeof(cl_device_id));
		status = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, numDevices, devices, NULL);
	}


	/*Step 3: Create context.*/
	cl_context context = clCreateContext(NULL, 1, devices, NULL, NULL, NULL);

	/*Step 4: Creating command queue associate with the context.*/
	cl_command_queue commandQueue = clCreateCommandQueue(context, devices[0], CL_QUEUE_PROFILING_ENABLE, NULL);

	/*Step 5: Create program object */
	const char *filename = "Kernel.cl";
	string sourceStr;
	status = convertToString(filename, sourceStr);
	const char *source = sourceStr.c_str();
	size_t sourceSize[] = { strlen(source) };
	cl_program program = clCreateProgramWithSource(context, 1, &source, sourceSize, NULL);

	/*Step 6: Build program. */
	status = clBuildProgram(program, 1, devices, NULL, NULL, NULL);

	/*Step 7: Initial input,output for the host and create memory objects for the kernel*/
	int szA = SIZE;
  int szB = SIZE;
  int szC = SIZE;

  int *A;
  int *B;
  int *C;

  A = (int *)malloc(szA * sizeof(int));
  B = (int *)malloc(szB * sizeof(int));
  C = (int *)malloc(szC * sizeof(int));
  
  auto start_time = chrono::high_resolution_clock::now();
  
	cl_mem Buffer_A = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                             szA * sizeof(int), A, NULL);
  
  cl_mem Buffer_B = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
                             szB * sizeof(int), B, NULL);
                             
	cl_mem Buffer_C = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, 
                             szC * sizeof(int), C, NULL);

	/*Step 8: Create kernel object */
	cl_kernel kernel = clCreateKernel(program, "vector_add", NULL);

	/*Step 9: Sets Kernel arguments.*/
	status = clSetKernelArg(kernel, 0, sizeof(cl_mem), &Buffer_A);
	status = clSetKernelArg(kernel, 1, sizeof(cl_mem), &Buffer_B);
  status = clSetKernelArg(kernel, 2, sizeof(cl_mem), &Buffer_C);
  status = clSetKernelArg(kernel, 3, sizeof(int), &SIZE);

  
	/*Step 10: Running the kernel.*/
	size_t global_work_size[1] = { SIZE };
 
	status = clEnqueueNDRangeKernel(commandQueue, kernel, 1, NULL, 
                                        global_work_size, NULL, 0, NULL, NULL);
  
	/*Step 11: Read the cout put back to host memory.*/
  
	status = clEnqueueReadBuffer(commandQueue, Buffer_C, CL_TRUE, 0, szC * sizeof(int), C, 0, NULL, NULL);
  
  auto end_time = chrono::high_resolution_clock::now();
  chrono::duration<double> time_duration = end_time-start_time;
  cout << "Non-SVM vector_add takes: " << time_duration.count() << " s" <<endl;
  
/*
  cout << "\nA :" << endl;
  for(int i = 0; i < szA; i++){

    cout << setw(4) << A[i] << " ";
    if( ((i+1) % Ndim) == 0 ){
      cout << endl;
    }
    
  }
    
  cout << "\nB :" << endl;
  for(int i = 0; i < szB; i++){

    cout << setw(4) << B[i] << " ";
    if( ((i+1) % Pdim) == 0 ){
      cout << endl;
    }
    
  }

	cout << "\nC :" << endl;
  for(int i = 0; i < szC; i++){

    cout << setw(4) << C[i] << " ";
    if( ((i+1) % Pdim) == 0 ){
      cout << endl;
    }
    
  }
*/  

	/*Step 12: Clean the resources.*/
	status = clReleaseKernel(kernel); //Release kernel.
	status = clReleaseProgram(program); //Release the program object.
	status = clReleaseMemObject(Buffer_A); //Release mem object.
  status = clReleaseMemObject(Buffer_B);
	status = clReleaseMemObject(Buffer_C);
	status = clReleaseCommandQueue(commandQueue); //Release  Command queue.
	status = clReleaseContext(context); //Release context.

	if (C != NULL)
	{
		free(C);
		C = NULL;
	}

	if (devices != NULL)
	{
		free(devices);
		devices = NULL;
	}

	std::cout << "\nPassed!\n";
 
}