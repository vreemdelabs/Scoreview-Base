/*
 Scoreview (R)
 Copyright (C) 2015 Patrick Areny
 All Rights Reserved.

 Scoreview is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef MAC
#include <OpenCL/cl.h>
#else
#include <CL/cl.h>
#endif

#include "opencldevice.h"

#define MAXPRG   32
#define OPTIONSZ 4096

cl_program Copencldevice::build_progam(int numfiles, char **filenames,
					     cl_device_id *pdevice, cl_context *pcontext)
{
   /* Program data structures */
   int        i;
   cl_program program;
   FILE       *program_handle;
   char       *program_buffer[MAXPRG];
   char       *program_log;
   size_t     program_size[MAXPRG];
   size_t     log_size;
   cl_int     err;
   char       CompilerOptions[OPTIONSZ];
    size_t    rd;

   /* Read each program file and place content into buffer array */
   for (i = 0; i < numfiles; i++)
     {
       printf("Compiling %s\n", filenames[i]);
       program_handle = fopen(filenames[i], "r");
       if (program_handle == NULL)
	 {
	   perror("Couldn't find the OpenCL program");
	   exit(EXIT_FAILURE);   
	 }
       fseek(program_handle, 0, SEEK_END);
       program_size[i] = ftell(program_handle);
       rewind(program_handle);
       program_buffer[i] = (char*)malloc(program_size[i]+1);
       program_buffer[i][program_size[i]] = '\0';
       rd = fread(program_buffer[i], sizeof(char), program_size[i], program_handle);
       if (rd != program_size[i])
	 printf("Warning reading .cl file failed.\n");
       fclose(program_handle);
     }

   /* Create a program containing all program content */
   program = clCreateProgramWithSource(*pcontext, numfiles, (const char**)program_buffer, program_size, &err);
   if (err < 0)
     {
       perror("Couldn't create the program");
       exit(EXIT_FAILURE);
     }

   // -cl-finite-math-only: assumes that no floats are Not a Number or -+infinite
   //sprintf(CompilerOptions, "-cl-denorms-are-zero -D SAMPLING_FREQUENCY=%d", pKP->sampling_frequency);
   //sprintf(CompilerOptions, "-D SAMPLING_FREQUENCY=%d", pKP->sampling_frequency);
   snprintf(CompilerOptions, OPTIONSZ, " ");

   /* Build program */
   err = clBuildProgram(program, 1, pdevice, CompilerOptions, NULL, NULL);
   if (err < 0)
     {
       /* Find size of log and print to std output */
       clGetProgramBuildInfo(program, *pdevice, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
       program_log = (char*) malloc(log_size + 1);
       program_log[log_size] = '\0';
       clGetProgramBuildInfo(program, *pdevice, CL_PROGRAM_BUILD_LOG, log_size + 1, program_log, NULL);
       printf("%s\n", program_log);
       free(program_log);
       exit(EXIT_FAILURE);
     }

   /* Deallocate resources */
   for (i = 0; i < numfiles; i++)
     {
       free(program_buffer[i]);
     }
   return program;
}

void Copencldevice::print_device_info_string(cl_device_id* device, cl_device_info infID, char *printstring, int j)
{
  size_t valueSize;
  char*  value;
  
  clGetDeviceInfo(*device, infID, 0, NULL, &valueSize);
  value = (char*)malloc(valueSize);
  if (value != NULL)
    {
      if (CL_SUCCESS == clGetDeviceInfo(*device, CL_DEVICE_NAME, valueSize, value, NULL))
        printf(printstring, j+1, value);
      else
        printf(printstring, j+1, (char*)"nc");
      free (value);
    }
}

// Looks for a GPU
//device_type = CL_DEVICE_TYPE_ALL;
//device_type = CL_DEVICE_TYPE_GPU;
//device_type = CL_DEVICE_TYPE_CPU;
int Copencldevice::find_opencl_device(cl_device_type  device_type)
{
  unsigned int    i, j;
  cl_uint         platformCount;
  cl_platform_id  *platforms;
  cl_uint         deviceCount;
  cl_device_id    *devices;
  cl_uint         maxComputeUnits;
  cl_ulong        globalmemsize;
  cl_int          err;

  // get all platforms
  err = clGetPlatformIDs(0, NULL, &platformCount);
  if (err != CL_SUCCESS)
    {
      perror("Getting Opencl platform count failed");
      exit(EXIT_FAILURE);   
    }
  printf("OpenCl platform count=%d.", platformCount);
  platforms = new cl_platform_id[platformCount];
  //platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id) * platformCount);
  err = clGetPlatformIDs(platformCount, platforms, NULL);
  /* if (err != CL_SUCCESS)
    {
      perror("Getting Opencl platforms failed");
      printf("Install the nvidia or AMD OpenCL drivers\n");
      exit(EXIT_FAILURE);
    }
  */
  for (i = 0; i < platformCount; i++)
    {
      // Get all devices on the platform
      err = clGetDeviceIDs(platforms[i], device_type, 0, NULL, &deviceCount);
      /*
      if (err != CL_SUCCESS)
	{
	  perror("Getting Opencl devices IDs count failed");
	  exit(EXIT_FAILURE);
	}
      */
      devices = (cl_device_id*)malloc(sizeof(cl_device_id) * deviceCount);
      err = clGetDeviceIDs(platforms[i], device_type, deviceCount, devices, NULL);
      /*
      if (err != CL_SUCCESS)
	{
	  perror("Getting Opencl devices IDs failed");
	  exit(EXIT_FAILURE);   
	}
      */
      // For each device print critical attributes
      for (j = 0; j < deviceCount; j++)
	{
	  print_device_info_string(&devices[j],  CL_DEVICE_NAME, (char*)"%d. Device: %s\n", j);	             // Print device name
	  print_device_info_string(&devices[j],  CL_DEVICE_VERSION, (char*)"%d. Hardware version: %s\n", j); // Print hardware device version
	  print_device_info_string(&devices[j],  CL_DRIVER_VERSION, (char*)"%d. Software version: %s\n", j); // print software driver version
	  // print c version supported by compiler for device
	  print_device_info_string(&devices[j],  CL_DEVICE_OPENCL_C_VERSION, (char*)"%d. OpenCL version: %s\n", j);
	  // print parallel compute units
	  clGetDeviceInfo(devices[j], CL_DEVICE_MAX_COMPUTE_UNITS,
			  sizeof(maxComputeUnits), &maxComputeUnits, NULL);
	  printf(" %d.%d Parallel compute units: %d\n", j + 1, 4, maxComputeUnits);
	  // Get the global memory size
	  clGetDeviceInfo(devices[j], CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(globalmemsize), &globalmemsize, NULL);
	  printf(" %d.%d Global memory size: %d   %dMB\n", j + 1, 5, (int)globalmemsize, (int)globalmemsize / (1024 * 1024));
	  // Max work dimensions
	  cl_uint dim;
	  clGetDeviceInfo(devices[j], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(dim), &dim, NULL);
	  printf(" Dimensions: %d\n", (int)dim);
	  // Dimensions sizes
	  size_t dimsizes[3];
	  clGetDeviceInfo(devices[j], CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(dimsizes), &dimsizes, NULL);
	  printf(" Dimensions sizes: %d %d %d\n", (int)dimsizes[0], (int)dimsizes[1], (int)dimsizes[2]);
        }
      free(devices);
    }
  // Get all devices on the first platform
  clGetDeviceIDs(platforms[0], device_type, 0, NULL, &deviceCount);
  if (deviceCount <= 0)
    return (1);
  devices = (cl_device_id*)malloc(sizeof(cl_device_id) * deviceCount);
  clGetDeviceIDs(platforms[0], device_type, deviceCount, devices, NULL);
  m_device = devices[0];
  free(devices);
  delete[] platforms;
  return (0);
}

void Copencldevice::print_enqueNDrKernel_error(cl_int err)
{
  switch (err)
    {
    case CL_INVALID_PROGRAM_EXECUTABLE:
      printf("there is no successfully built program executable available for device associated with command_queue.\n");
      break;
    case CL_INVALID_COMMAND_QUEUE:
      printf("command_queue is not a valid command-queue\n");
      break;
    case CL_INVALID_KERNEL:
      printf("kernel is not a valid kernel object\n");
      break;
    case CL_INVALID_CONTEXT:
      printf("context associated with command_queue and kernel is not the same or if the context associated with command_queue and events in event_wait_list are not the same\n");
      break;
    case CL_INVALID_KERNEL_ARGS:
      printf("the kernel argument values have not been specified\n");
      break;
    case CL_INVALID_WORK_DIMENSION:
      printf("work_dim is not a valid value (i.e. a value between 1 and 3)\n");
      break;
    case CL_INVALID_WORK_GROUP_SIZE:
      printf("CL_INVALID_WORK_GROUP_SIZE\n");
      break;
    case CL_INVALID_WORK_ITEM_SIZE: 
      printf("the number of work-items specified in any of local_work_size[0], ... local_work_size[work_dim - 1] is greater than the corresponding values specified by CL_DEVICE_MAX_WORK_ITEM_SIZES[0],...\n");
      break;
    case CL_INVALID_GLOBAL_OFFSET:
      printf("global_work_offset is not NULL\n");
      break;
    case CL_OUT_OF_RESOURCES:
      printf("CL_OUT_OF_RESOURCES\n");
      break;
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:
      printf("here is a failure to allocate memory for data store associated with image or buffer objects specified as arguments to kernel\n");
      break;
    case CL_INVALID_EVENT_WAIT_LIST:
      printf("event_wait_list is NULL and num_events_in_wait_list > 0, or event_wait_list is not NULL and num_events_in_wait_list is 0, or if event objects in event_wait_list are not valid events\n");
      break;
    case CL_OUT_OF_HOST_MEMORY:
      printf("there is a failure to allocate resources required by the OpenCL implementation on the host\n");
      break;
    }
}

void Copencldevice::print_enqueReadBuffer_error(cl_int err)
{
  switch (err)
    {
    case CL_INVALID_COMMAND_QUEUE:
      printf("command_queue is not a valid command-queue\n");
      break;
    case CL_INVALID_CONTEXT:
      printf("context associated with command_queue or events and buffer are not the same\n");
      break;
    case CL_INVALID_MEM_OBJECT:
      printf("buffer is not a valid buffer object\n");
      break;
    case CL_INVALID_VALUE:
      printf("the region being read specified by (offset, cb) is out of bounds or if ptr is a NULL value\n");
      break;
    case CL_INVALID_EVENT_WAIT_LIST:
      printf("event_wait_list is NULL and num_events_in_wait_list greater than 0\n");
      break;
    case CL_MEM_OBJECT_ALLOCATION_FAILURE:
      printf("there is a failure to allocate memory for data store associated with buffer\n");
      break;
    case CL_OUT_OF_HOST_MEMORY:
      printf("there is a failure to allocate resources required by the OpenCL implementation on the host\n");
      break;
    };
}

