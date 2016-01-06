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

#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <assert.h>

#include <SDL2/SDL.h>
#include <CL/cl.h>

#include "spectrometre.h"
#include "colorscale.h"
#include "hann_window.h"
#include "opencldevice.h"
#include "opencldft.h"

#define STRSIZ 1024
#define KERNELPROGRAMNAME    "./dft.cl"
#define FIRKERNELPROGRAMNAME "./filterfir.cl"

Copenclspectrometer::Copenclspectrometer(int samplingf, int wx, int wy,
					 float fbase, float fmax): 
  m_sampling_frequency(samplingf),
  m_fbase(fbase),
  m_fmax(fmax),
  m_wx(wx),
  m_wy(wy),
  m_maxdbvalue(1.),
  m_bhalfband(false)
{
  m_output_buffer_size = m_wx * m_wy;
  m_outputbuffer = new float[m_wx * m_wy];
  set_kernel_prameters(0, 0, 0, m_fmax);
  m_maxwindowsize = 4 * m_sampling_frequency;
  m_windowdata = new float[m_maxwindowsize];
  m_pattack = new float[m_wx];
  m_pdecay  = new float[m_wx];
  for (int i = 0; i < m_wx; i++)
    {
      m_pattack[i] = 0;
      m_pdecay[i] = 0;
    }
}

Copenclspectrometer::~Copenclspectrometer()
{
  release_opencl_dft();
  delete[] m_pattack;
  delete[] m_pdecay;
  delete[] m_windowdata;
  delete[] m_outputbuffer;
}

cl_device_id Copenclspectrometer::get_opencl_deviceid()
{
  return m_device;
}

float Copenclspectrometer::get_analysis_interval_T(float maxdispf)
{
  float T;
  float fresolution;

  // The minimum interval needed depends on the frequency resolution. T = 1 / frequres
  // frequres = abs(fmax - fbase) / spectrogram_height
  // for a C1 at 37hz Cd1 is at 34hz the resolution needed at this zoom factor is below 0.5hz
  // (50hz - 20hz) / 512 = 30 / 512 = 0.058hz
  // T = 1 / 0.058 = 17 seconds... Keep it at 1 second max
  // 
  fresolution = (maxdispf - m_fbase) / (float)m_wy;
  T = (1 / fresolution);
  // Get below the ideal frequency resolution, the time variation will be more visible
  T /= 2;
  // Stop abusing the machine for once, 0.5hz is the maximum resolution
  if (T > 2.)
    T = 2.;
  //printf("fresolution=%f T=%f\n", fresolution, T);
  return T;
}

// Number of samples inside the discrete transform interval
int Copenclspectrometer::get_N(int sampling_frequency)
{
  int N;

  N = (int)((float)sampling_frequency * get_analysis_interval_T(m_kparams.maxfreq));
  return N;
}

void Copenclspectrometer::set_kernel_prameters(int tracksize, int startsample, int stopsample, float fmax)
{
  // Does not move during execution
  m_kparams.sampling_frequency = m_sampling_frequency;
  // Changes
  m_kparams.maxfreq = fmax;
  m_kparams.minfreq = m_fbase;
  m_kparams.N = get_N(m_kparams.sampling_frequency);
  m_kparams.tracksize = tracksize;
  m_kparams.start_sample = startsample;
  m_kparams.stop_sample = stopsample;
}

void Copenclspectrometer::create_kernels()
{
   int        numfiles = 2;
   char       filename0[STRSIZ];
   char       filename1[STRSIZ];
   char       *filenames[4];
   cl_int     err;

   /* Build the program */
   strcpy(filename0, KERNELPROGRAMNAME);
   strcpy(filename1, FIRKERNELPROGRAMNAME);
   filenames[0] = filename0;
   filenames[1] = filename1;
   m_program = build_progam(numfiles, filenames, &m_device, &m_context);
   /* Create the fir kernel */
   m_firkernel = clCreateKernel(m_program, "fir", &err);
   if (err < 0)
   {
      perror("Couldn't create the kernel");
      exit(EXIT_FAILURE);
   }
   /* Create the dft kernel */
   m_kernel = clCreateKernel(m_program, "dft", &err);
   if (err < 0)
   {
      perror("Couldn't create the kernel");
      exit(EXIT_FAILURE);
   }
   
}

void Copenclspectrometer::calibration()
{
  float  *psinebuffer;
  double time;
  int    samples;
  int    i, j;
  float  f;
  float  sr;
  unsigned long exectime;
  int    updatewidth;
  int    start, stop;
  float  max;

  time = 8.;
  samples = time * m_sampling_frequency;
  psinebuffer = new float[samples];
  f = 440.;
  sr = (float)m_sampling_frequency;
  for (i = 0; i < samples; i++)
    {
      psinebuffer[i] = sin((2. * M_PI * f * (float)i) / sr);
    }
  // Calculate the chromagram
  updatewidth = m_wx / 4;
  start = 4 * m_sampling_frequency;
  stop = 5 * m_sampling_frequency;
  f = m_sampling_frequency;
  opencl_dft(psinebuffer, samples, start, stop, updatewidth, f, &exectime);
  // Find the hightest value in the chromagram
  for (i = 0, max = 0; i < updatewidth; i++)
    {
      for (j = 0; j < m_wy; j++)
	{
	  f = m_outputbuffer[i * m_wy + j]; 
	  if (f > max)
	    max = f;
	}
    }
  m_maxdbvalue = max; 
  m_dbvalueN = get_N(m_sampling_frequency);
  delete[] psinebuffer;
  printf("The spectrogram maximum audio level is %f\n", m_maxdbvalue);
}

int Copenclspectrometer::init_opencl_dft()
{
  cl_int   err;

  //-----------------------------------------------------------------------------
  // Open cl devices
  //-----------------------------------------------------------------------------
  if (find_opencl_device(CL_DEVICE_TYPE_GPU))
    return 1;
  // Create a context
  m_context = clCreateContext(NULL, 1, &m_device, NULL, NULL, &err);
  if (err < 0)
    {
      perror("Couldn't create a context");
      exit(EXIT_FAILURE);   
    }
  //-----------------------------------------------------------------------------
  // Create programs
  // Create the kernels
  //-----------------------------------------------------------------------------
  create_kernels();
  //-----------------------------------------------------------------------------
  // Buffers
  //-----------------------------------------------------------------------------
#ifdef USEFIR_KERNEL
  allocate_fir_buffers();
#endif
  // Input samples to the dft
  m_samples = clCreateBuffer(m_context,
	                     CL_MEM_READ_WRITE,
			     (MAX_SPECTROMETER_LENGTH_SECONDS + 2) * m_sampling_frequency * sizeof(float), NULL, &err);
  if (err < 0)
    {
      perror("Couldn't create the GPU track buffer");
      exit(EXIT_FAILURE);
    }

  // Create the kernel parameters buffer
  m_kernelcfg = clCreateBuffer(m_context,
			       CL_MEM_READ_ONLY,
			       sizeof(m_kparams), NULL, &err);
  if (err < 0)
    {
      perror("Couldn't create the GPU track buffer");
      exit(EXIT_FAILURE);
    }

  // Create the hann window buffer
  m_hannwindow = clCreateBuffer(m_context,
		                CL_MEM_READ_ONLY,
				m_maxwindowsize * sizeof(float), NULL, &err);
  if (err < 0)
    {
      perror("Couldn't create the GPU hann window buffer");
      exit(EXIT_FAILURE);
    }

  // Create the output buffer (image format)
  m_spectre_img = clCreateBuffer(m_context,
	                     CL_MEM_WRITE_ONLY,
			     m_output_buffer_size * sizeof(float), NULL, &err);
  if (err < 0)
    {
      perror("Couldn't create the GPU spectrum buffer");
      exit(EXIT_FAILURE);
    }

  //-----------------------------------------------------------------------------
  // Create the command queue
  //-----------------------------------------------------------------------------
  m_queue = clCreateCommandQueue(m_context, m_device, CL_QUEUE_PROFILING_ENABLE, &err); // FIXME works better on nvidia cards if the Queue is only allocated once
  if (err < 0)
    {
      perror("Couldn't create the command queue");
      exit(EXIT_FAILURE);
    }
  return 0;
}

void Copenclspectrometer::release_opencl_dft()
{
  clReleaseCommandQueue(m_queue); // FIXME works better on nvidia cards if the Queue is only allocated once
  clReleaseKernel(m_firkernel);
  clReleaseKernel(m_kernel);
  clReleaseProgram(m_program);
  clReleaseContext(m_context);
}

void Copenclspectrometer::img_float_to_RGB32(float *fbuffer, unsigned int *pimg, int width, int height, int updatewidth, int circularindex, bool bdecibelview)
{
  int   i, j;
  int   index;
  int   start;
  static float max = 0;
  float newmax;

  for (i = 0; i < updatewidth; i++)
    {
      index = i * height;
      if (!bdecibelview)
	{
	  for (j = 0, newmax = 0.; j < height; j++)
	    {
	      if (fbuffer[index + j] > newmax)
		newmax = fbuffer[index + j];
	    }
	  max = newmax;
	}
      if (bdecibelview)
	max = m_maxdbvalue;
      start = (circularindex + i) % width;
      for (j = 0; j < height; j++)
	{
	  //const int scp = 1000;
	  float f = fbuffer[index + j];
	  
	  //pimg[(height - j - 1) * width + start] = value_to_color_888(10. * log(scp * f / max), 40);
	  pimg[(height - j - 1) * width + start] = value_to_color_888(f, max);
	}
    }
    img_float_to_attack(fbuffer, width, height, updatewidth, circularindex);
//#define SHOW_ATACK
#ifdef SHOW_ATACK
    int y, e;

    for (i = 0; i < updatewidth; i++)
      {
	start = (circularindex + i) % width;
	y = m_pattack[start] - m_pdecay[start];
	if (y  < 0)
	  y = 0;
	if (y > height)
	  y = height;
	for (e = 1; e < y; e++)
	  pimg[(height - e) * width + start] = 0xEBD233;//0xFE0000;
	//printf("v is %f\n", m_pfreqbalance[start]);
      }
#endif
}

void Copenclspectrometer::img_float_to_attack(float *fbuffer, int width, int height, int updatewidth, int circularindex)
{
  int   i, j;
  int   index;
  int   previndex;
  int   start;

  for (i = 1; i < updatewidth; i++)
    {
      index = i * height;
      previndex = (i - 1) * height;
      start = (circularindex + i) % width;
      m_pattack[start] = 0;
      m_pdecay[start] = 0;
      for (j = 0; j < height; j++)
	{
	  float v = (fbuffer[index + j] - fbuffer[previndex + j]);
	  if (v > 0)
	    m_pattack[start] += v;
	  else
	    m_pdecay[start] -= v;
	}
      //     printf("v is %f\n", pattack[start]);
    }
  start = (circularindex) % width;
  m_pattack[start] = m_pattack[(start + 1) % width];
  m_pdecay[start] = m_pdecay[(start + 1) % width];
}

void Copenclspectrometer::reset_attack(float *pattackbuffer, int width)
{
  int   i;

  for (i = 0; i < width && i < m_wx; i++)
    {
      m_pattack[i] = m_pdecay[i] = 0;
      pattackbuffer[i] = 0;
    }
}

void Copenclspectrometer::get_attack(float *pattackbuffer, int width, int circularindex)
{
  int   start;
  int   i;

  for (i = 0; i < width && i < m_wx; i++)
    {
      start = (circularindex + i) % width;
      pattackbuffer[i] = m_pattack[start] - m_pdecay[start];
    }
}

void Copenclspectrometer::han_window_setup()
{
  int n;
  int N;

  N = m_kparams.N;
  for (n = 0; n < N; n++)
    m_windowdata[n] = hann(n, N);

  //printf("N=%09d start=%d\n", N, start);
}

// Creates a workgroup
int Copenclspectrometer::opencl_dft(float *psamples, int samplenum, int start, int stop, int updatewidth, float fmax, unsigned long *ptime)
{
  cl_int   err;
  size_t   width  = m_wx;
  size_t   height = m_wy;
  int      i;
  cl_event timing_events[2];
	
  //printf("samplenum=%d start=%d stop=%d\n", samplenum, start, stop);
  set_kernel_prameters(samplenum, start, stop, fmax);

  //-----------------------------------------------------------------------------
  // Filter the signal depending on the frequency zoom
  //-----------------------------------------------------------------------------
  filter_signal_zfactor(psamples, &samplenum, start, stop, fmax);

  //-----------------------------------------------------------------------------
  // Calculate once the hann window
  //-----------------------------------------------------------------------------
  han_window_setup();

  //-----------------------------------------------------------------------------
  // Set kernel arguments
  // Input: samples in the range of the sound analysis
  // Input: dft parameters
  // Output: spectrometer image
  //-----------------------------------------------------------------------------
#ifdef USEFIR_KERNEL
  set_decimation_arguments();
#endif
  err = clSetKernelArg(m_kernel, 0, sizeof(m_samples), &m_samples);
  if (err < 0)
    {
      perror("Couldn't set the track buffer as the kernel argument");
      exit(EXIT_FAILURE);
    }
  err = clSetKernelArg(m_kernel, 1, sizeof(m_kernelcfg), &m_kernelcfg);
  if (err < 0)
    {
      perror("Couldn't set the dft param structure as the kernel argument");
      exit(EXIT_FAILURE);
    }
  err = clSetKernelArg(m_kernel, 2, sizeof(m_hannwindow), &m_hannwindow);
  if (err < 0)
    {
      perror("Couldn't set the hannwindow buffer kernel argument");
      exit(EXIT_FAILURE);
    }
  err = clSetKernelArg(m_kernel, 3, sizeof(m_spectre_img), &m_spectre_img);
  if (err < 0)
    {
      perror("Couldn't set the output image as the kernel argument");
      exit(EXIT_FAILURE);
    }

  //-----------------------------------------------------------------------------
  // Determine maximum work-group size
  //-----------------------------------------------------------------------------
  size_t workgroup_size;
  err = clGetKernelWorkGroupInfo(m_kernel, m_device, CL_KERNEL_WORK_GROUP_SIZE, sizeof(workgroup_size), &workgroup_size, NULL);
  if (err < 0)
    {
      switch (err)
	{
	case CL_INVALID_DEVICE:
	  printf("invalid device\n");
	  break;
	case CL_INVALID_VALUE:
	  printf("invalid value\n");
	  break;
	case CL_INVALID_KERNEL:
	  printf("invalid kernel\n");
	  break;
	default:
	  printf("unknown error\n");
	  break;
	};
      perror("Couldn't find the maximum work-group size");
      exit(EXIT_FAILURE);
    }
#ifdef _DEBUG
  size_t prefered_workgroup_multiple;
  err = clGetKernelWorkGroupInfo(m_kernel, m_device, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
				 sizeof(prefered_workgroup_multiple), &prefered_workgroup_multiple, NULL);
  //printf("The kernel workgroup size is %d, the prefered size multiple is %d\n", (int)workgroup_size, (int)prefered_workgroup_multiple);

  size_t wrk_sizes[3];
  err = clGetKernelWorkGroupInfo(m_kernel, m_device, CL_KERNEL_COMPILE_WORK_GROUP_SIZE,
				 sizeof(wrk_sizes), &wrk_sizes, NULL);
  //printf("The kernel compile workgroup sizes are %d %d %d\n", (int)wrk_sizes[0], (int)wrk_sizes[1], (int)wrk_sizes[2]);
#endif
  // Q
  
  //-----------------------------------------------------------------------------
  //
  // Enqueue the track data write to the GPU
  //
  //-----------------------------------------------------------------------------
#ifdef USEFIR_KERNEL
  if (m_bhalfband)
    {
      err = clEnqueueWriteBuffer(m_queue, m_firsamplebuffer, CL_TRUE, 0, m_kfirparams.intracksize * sizeof(float),  psamples, 0, NULL, NULL);
      if (err != CL_SUCCESS)
	{
	  perror("Error: clEnqueueWriteBuffer failed.");
	  return false;
	}
    }
  else
#endif
    {
      err = clEnqueueWriteBuffer(m_queue, m_samples, CL_TRUE, 0, samplenum * sizeof(float),  psamples, 0, NULL, NULL);
      if (err != CL_SUCCESS)
	{
	  perror("Error: clEnqueueWriteBuffer failed.");
	  return false;
	}
    }
  //-----------------------------------------------------------------------------
  //
  // Enqueue the kernel parameters write to the GPU
  //
  //-----------------------------------------------------------------------------
  err = clEnqueueWriteBuffer(m_queue, m_kernelcfg, CL_TRUE, 0, sizeof(t_kparam), &m_kparams, 0, NULL, NULL);
  if (err != CL_SUCCESS)
    {
      perror("Error: clEnqueueWriteBuffer failed.");
      return false;
    }

  //-----------------------------------------------------------------------------
  //
  // Enqueue the hannwindow data to the GPU
  //
  //-----------------------------------------------------------------------------
  err = clEnqueueWriteBuffer(m_queue, m_hannwindow, CL_TRUE, 0, m_maxwindowsize * sizeof(float), m_windowdata, 0, NULL, NULL);
  if (err != CL_SUCCESS)
    {
      perror("Error: clEnqueueWriteBuffer failed.");
      return false;
    }

#ifdef USEFIR_KERNEL
  if (!enqueue_fir_kernels(timing_events))
    return false;
#endif

  //-----------------------------------------------------------------------------
  //
  // Enqueue the kernel execution command (forget enqueue task)
  //
  //-----------------------------------------------------------------------------
  //printf("Starting a kernel of %d x %d\n", (int)width, (int)height);
  cl_uint workdim = 2; // 2D result from 1D input
  // First global workgroup dimen sion size must be default workgroup size, second dimension can be set to whatever
  size_t updatew = updatewidth;
  if (updatew > width)
    updatew = width;
  const size_t global_work_size[2] = {height, updatew};  // Total numer of work items = pixels in output image
  while (workgroup_size > height)
    workgroup_size /= 2;
  const size_t local_work_size[2] = {workgroup_size, 1}; // 
  //const size_t local_work_size[2] = {1, workgroup_size};  // 97ms (one colum of spectrum) Must not exceed CL_DEVICE_MAX_WORK_GROUP_SIZE.
  //printf("updatewidth=%d workgroupsize=%d\n", local_work_size[1], local_work_size[0]);
  err = clEnqueueNDRangeKernel(m_queue, m_kernel, workdim, NULL, global_work_size, local_work_size, 0, NULL, timing_events);
  if (err != CL_SUCCESS)
    {
      print_enqueNDrKernel_error(err);
      printf("Couldn't enqueue the spectrum kernel execution.\n");
      exit(EXIT_FAILURE);
    }
  //else 
    //printf("Successfully queued kernel.\n");
  
  //-----------------------------------------------------------------------------
  //
  // Enqueue a memory read to the output image
  //
  //-----------------------------------------------------------------------------
  cl_bool blocking_read = CL_TRUE;
  size_t offset = 0;
  size_t cb = m_wx * m_wy * sizeof(float);
  err = clEnqueueReadBuffer(m_queue, m_spectre_img, blocking_read, offset, cb, m_outputbuffer, 0, NULL, NULL);
  if (err != CL_SUCCESS)
    {
      perror("Error: clEnqueueReadBuffer failed.");
      return false;
    }

  // Wait for completion
  clFinish(m_queue);

  //-----------------------------------------------------------------------------
  //
  // Display the processing time
  //
  //-----------------------------------------------------------------------------
#define OPENCLTIME
#ifdef OPENCLTIME
  cl_ulong  timestart, timestop;
  cl_ulong  total;
  int       eventnum;

#ifdef USEFIR_KERNEL
  eventnum = m_bhalfband? 2 : 1;
#else
  eventnum = 1;
#endif
  for (i = 0, total = 0; i < eventnum; i++)
    {
      clGetEventProfilingInfo(timing_events[i], CL_PROFILING_COMMAND_START, sizeof(timestart), &timestart, NULL);
      clGetEventProfilingInfo(timing_events[i], CL_PROFILING_COMMAND_END, sizeof(timestop), &timestop, NULL);
      total += timestop - timestart;
    }
  *ptime = total / 1000;
  //printf("Kernel execution time=%lums %luus\n", *ptime / 1000, *ptime);
#endif
  // Deallocate resources
  // Q
  return 0;
}

void Copenclspectrometer::dft_to_img(unsigned int *pimg, int updatewidth, int circularindex, bool dbview)
{
  size_t   width  = m_wx;
  size_t   height = m_wy;

  //-----------------------------------------------------------------------------
  //
  // Convert the output to RGB565
  //
  //-----------------------------------------------------------------------------
//#define TESTV
#ifdef TESTV
  for (int i = 0; i < 8 * 512; i+=4)
    printf("%f %f %f %f\n", m_outputbuffer[i], m_outputbuffer[i+1], m_outputbuffer[i+2], m_outputbuffer[i+3]);
  exit(EXIT_FAILURE);
#else
  img_float_to_RGB32(m_outputbuffer, pimg, width, height, updatewidth, circularindex, dbview);
#endif
}

