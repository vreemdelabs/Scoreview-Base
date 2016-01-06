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
#include <algorithm>
#include <assert.h>

#include <CL/cl.h>

#include "spectrometre.h"
#include "colorscale.h"
#include "hann_window.h"
#include "f2n.h"
#include "opencldevice.h"
#include "openclchroma.h"
#define LOW_FREQUENCY_DECIMATION
#ifdef LOW_FREQUENCY_DECIMATION
#include <SDL2/SDL.h>
#include "filterlib.h"
#endif

#define OCTAVESZ 12
#define C0  0
#define C1 12
#define C8 (8 * 12)
#define STRSIZ 1024
#define KERNELPROGRAMNAME "./chroma.cl"
#define IMGH (256)

#define REORDERKERNEL

Copenclchroma::Copenclchroma(cl_device_id deviceid, int samplingf, int wx, int wy, int maxsptime):
  m_sampling_frequency(samplingf),
  m_wx(wx),
  m_output_height(256 * 12),
  m_recomp_height(128),
  m_f2n(440., 2.),
  m_maxsptime(maxsptime),
  m_usedoctaves(9),
  m_device(deviceid)
{
  float T;
  int   N;

  T = get_T(C0 + 2);
  N = get_N(T);
  m_max_needed_samples = 2 * N; // 2 for resolution
  m_output_buffer_size = m_wx * m_output_height * 2;
  m_outputbuffer = new float[m_output_buffer_size];
  m_recomposebuffersize = m_wx * m_recomp_height;
  m_recomposebuffer = new float[m_recomposebuffersize];
  set_kernel_prameters(0, 0, 0);
  m_windowdata = new float[OCTAVESZ * m_max_needed_samples * 2];
}

Copenclchroma::~Copenclchroma()
{
  release_opencl_chromagram();
  delete[] m_recomposebuffer;
  delete[] m_windowdata;
  delete[] m_outputbuffer;
}

float Copenclchroma::get_T(int note)
{
  float fhi;
  float flo;
  float fresolution;
  float T;

  m_f2n.absnote_frequ_boundary(&flo, &fhi, note);
  fresolution = fabs(fhi - flo);
  T = 1. / fresolution;
  //printf ("T for %d is  %f flo=%f fhi=%f resolution=%f\n", note, T, flo, fhi, fresolution);
  return T;
}

int Copenclchroma::get_N(float T)
{
  return (T * m_sampling_frequency);
}

// Create a hann window for each note
void Copenclchroma::init_hanwindows()
{
  int note, i;
  int octave;
  int N;
  int bufferoffset;
  float *pwindow;
  int start;

  for (note = 0; note < 12; note++)
    {
      bufferoffset = note * m_max_needed_samples * 2;
      N = m_max_needed_samples;
      start = 0;
      for (octave = 0; octave < m_usedoctaves; octave++) 
	{
	  pwindow = &m_windowdata[bufferoffset + start];
	  for (i = 0; i < N; i++)
	    {
	      pwindow[i] = hann((double)i, N);
	    }
	  start += N;
	  if (octave < 8)
	    N = N / 2;
	}
    }
}

float Copenclchroma::get_needed_interval_time()
{
  return get_T(C0);
}

void Copenclchroma::set_kernel_prameters(int tracksize, int startsample, int stopsample)
{
  // Does not move during execution
  m_kparams.sampling_frequency = m_sampling_frequency;
  m_kparams.N = m_max_needed_samples;
  // Changes
  m_kparams.tracksize = tracksize;
  m_kparams.start_sample = startsample;
  m_kparams.stop_sample = stopsample;
}

void Copenclchroma::create_kernels(t_ckparam *pKP)
{
   int        numfiles = 1;
   char       filename[STRSIZ];
   char       *filenames[4];
   cl_int     err;

   /* Build the program */
   strcpy(filename, KERNELPROGRAMNAME);
   filenames[0] = filename;
   m_program = build_progam(numfiles, filenames, &m_device, &m_context);
   /* Create the chromagram kernel */
   m_kernel = clCreateKernel(m_program, "chromagramme", &err);
   if (err < 0)
   {
      perror("Couldn't create the kernel");
      exit(EXIT_FAILURE);
   }
#ifdef REORDERKERNEL
   // Reordering kernel
   m_kernelReorder = clCreateKernel(m_program, "last_sum", &err);
   if (err < 0)
   {
      perror("Couldn't create the kernel");
      exit(EXIT_FAILURE);
   }
#endif
}

void Copenclchroma::calibration()
{
  float  *psinebuffer;
  double time;
  int    samples;
  int    i, note;
  float  f;
  float  sr;
  unsigned long  exectime;
  int    updatewidth;
  int    start, stop;
  float  max;
  float  *pchromaline;

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
  opencl_chromagram(psinebuffer, samples, start, stop, updatewidth, &exectime);
  // Find the hightest value in the chromagram
  output_recomposition(updatewidth);
  for (i = 0, max = 0; i < updatewidth; i++)
    {
      pchromaline = &m_recomposebuffer[i * 128];
      for (note = 0; note < C8 + 1; note++)
	{
	  f = pchromaline[note]; 
	  if (f > max)
	    max = f;
	}
    }
  m_maxdbvalue = max; 
  delete[] psinebuffer;
  printf("The chromagram maximum audio level is %f\n", m_maxdbvalue);
}

int Copenclchroma::init_opencl_chromagram()
{
  cl_int   err;
  int      i;

  init_hanwindows();
  for (i = 0; i < m_output_buffer_size; i++)
    m_outputbuffer[i] = 0;
  //-----------------------------------------------------------------------------
  // Create a context
  //-----------------------------------------------------------------------------
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
  create_kernels(&m_kparams);
  //-----------------------------------------------------------------------------
  // Buffers
  //-----------------------------------------------------------------------------
  // Create the input buffer
  m_samples = clCreateBuffer(m_context,
	                     CL_MEM_READ_ONLY,
			     (m_maxsptime + 2) * m_sampling_frequency * sizeof(float), NULL, &err);
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
				OCTAVESZ * 2 * m_max_needed_samples * sizeof(float), NULL, &err);
  if (err < 0)
    {
      perror("Couldn't create the GPU track buffer");
      exit(EXIT_FAILURE);
    }

  // Create the output buffer (image format)
  m_chroma_img = clCreateBuffer(m_context,
				CL_MEM_READ_WRITE,
				m_output_buffer_size * sizeof(float), NULL, &err);
  if (err < 0)
    {
      perror("Couldn't create the GPU spectrum buffer");
      exit(EXIT_FAILURE);
    }
  //-----------------------------------------------------------------------------
  // Find the maximum audio level in order to display absolute values
  //-----------------------------------------------------------------------------
  //qualibration();
  return 0;
}

void Copenclchroma::release_opencl_chromagram()
{
  clReleaseKernel(m_kernel);
  clReleaseProgram(m_program);
  clReleaseContext(m_context);
}

//#define DISP_FILTER_TIME
void Copenclchroma::filter_signal_zfactor(float *psamples, int *psamplenum)
{
//return;
#ifdef LOW_FREQUENCY_DECIMATION
  float fact = 2;
  int   s, s2;
  float cutfrequency;
  int   scut;

  cutfrequency = m_sampling_frequency / fact;
  scut = fact;
#ifdef DISP_FILTER_TIME
  printf("fmax=%f cut off frequency=%f decimation=%d\n", m_f2n.note2frequ(C8 + 1), cutfrequency, scut);
  double t1 = SDL_GetTicks();
#endif
  DSPCPPfilter_low_pass_Butterworth(psamples, *psamplenum, m_sampling_frequency, cutfrequency);
#ifdef DISP_FILTER_TIME
  double t2 = SDL_GetTicks();
  printf("filtering took %fms\n", t2 - t1);
#endif
  for (s = 0, s2 = 0; s < *psamplenum; s += scut)
    {
      psamples[s2++] = psamples[s];
    }
  for (s = 0, s2 = 0; s < OCTAVESZ * m_max_needed_samples * 2; s += scut)
    {
      m_windowdata[s2++] = m_windowdata[s];
    }
  m_kparams.start_sample /= scut;
  m_kparams.stop_sample /= scut;
  m_kparams.tracksize /= scut;
  m_kparams.sampling_frequency = cutfrequency;
  m_kparams.N /= scut;
  *psamplenum /= scut;
  return;
#endif
}

int Copenclchroma::opencl_chromagram(float *psamples, int samplenum, int start, int stop, int updatewidth, unsigned long *ptime)
{
  cl_int   err;
  size_t   width  = m_wx;

  //fmax = 7902;
  set_kernel_prameters(samplenum, start, stop);

  //-----------------------------------------------------------------------------
  // Filter the signal depending on the frequency zoom
  //-----------------------------------------------------------------------------
  //filter_signal_zfactor(psamples, &samplenum);
  // TODO use 2 tracks, lower filtered at 130 hz * 4 = 520hz
  //      use 8 filters, same N for each octave + add 2 octaves

  //-----------------------------------------------------------------------------
  // Set kernel arguments
  // Input: samples in the range of the sound analysis
  // Input: dft parameters
  // Output: chromagram image
  //-----------------------------------------------------------------------------
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
  err = clSetKernelArg(m_kernel, 3, sizeof(m_chroma_img), &m_chroma_img);
  if (err < 0)
    {
      perror("Couldn't set the output image as the kernel argument");
      exit(EXIT_FAILURE);
    }
#ifdef REORDERKERNEL
  err = clSetKernelArg(m_kernelReorder, 0, sizeof(m_kernelcfg), &m_kernelcfg);
  if (err < 0)
    {
      perror("Couldn't set the dft param structure as the kernel argument");
      exit(EXIT_FAILURE);
    }
  err = clSetKernelArg(m_kernelReorder, 1, sizeof(m_chroma_img), &m_chroma_img);
  if (err < 0)
    {
      perror("Couldn't set the output image as the kernel argument");
      exit(EXIT_FAILURE);
    }
#endif
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
  size_t prefered_workgroup_multiple;
  err = clGetKernelWorkGroupInfo(m_kernel, m_device, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
				 sizeof(prefered_workgroup_multiple), &prefered_workgroup_multiple, NULL);
  //printf("The kernel workgroup size is %d, the prefered size multiple is %d\n", (int)workgroup_size, (int)prefered_workgroup_multiple);

  //-----------------------------------------------------------------------------
  // Create the command queue
  //-----------------------------------------------------------------------------
  m_queue = clCreateCommandQueue(m_context, m_device, CL_QUEUE_PROFILING_ENABLE, &err);
  if (err < 0)
    {
      perror("Couldn't create the command queue");
      exit(EXIT_FAILURE);
    }
  cl_event timing_event;
#ifdef REORDERKERNEL
  cl_event timing_event_reorder;
#endif
  //-----------------------------------------------------------------------------
  //
  // Enqueue the data writes to the GPU
  //
  //-----------------------------------------------------------------------------
  // Track data
  err = clEnqueueWriteBuffer(m_queue, m_samples, CL_TRUE, 0,  samplenum * sizeof(float),  psamples, 0, NULL, NULL);
  if (err != CL_SUCCESS)
    {
      perror("Chromagram Error: clEnqueueWriteBuffer failed.");
      return false;
    }
  // kernel parameters
  err = clEnqueueWriteBuffer(m_queue, m_kernelcfg, CL_TRUE, 0, sizeof(t_ckparam), &m_kparams, 0, NULL, NULL);
  if (err != CL_SUCCESS)
    {
      perror("Chromagram Error: clEnqueueWriteBuffer failed.");
      return false;
    }
  // han windows
  err = clEnqueueWriteBuffer(m_queue, m_hannwindow, CL_TRUE, 0, OCTAVESZ * 2 * m_max_needed_samples * sizeof(float), m_windowdata, 0, NULL, NULL);
  if (err != CL_SUCCESS)
    {
      perror("Chromagram Error: clEnqueueWriteBuffer failed.");
      return false;
    }

  //-----------------------------------------------------------------------------
  //
  // Enqueue the kernel execution command (forget enqueue task)
  //
  //-----------------------------------------------------------------------------
  //printf("Starting a kernel of %d x %d\n", (int)width, (int)m_output_height);
  cl_uint workdim = 3; // 2D result from 1D input
  // First global workgroup dimension size must be default workgroup size, second dimension can be set to whatever
  size_t updatew = updatewidth;
  //updatew &= ~1;
  if (updatew > width)
    updatew = width;
  size_t workh = 256;
  size_t notes = 12;
  const size_t global_work_size[3] = {workh, notes, updatew};  // Total numer of work items = N / 128 slots
  const size_t local_work_size[3] = {workgroup_size, 1, 1}; // 
  err = clEnqueueNDRangeKernel(m_queue, m_kernel, workdim, NULL, global_work_size, local_work_size, 0, NULL, &timing_event);
  if (err < 0)
    {
      print_enqueNDrKernel_error(err);
      perror("Couldn't enqueue the kernel execution command");
      exit(EXIT_FAILURE);
    }
#ifdef REORDERKERNEL
  const size_t cglobal_work_size[3] = {9, notes, updatew};  // Total numer of work items = octaves * 12 * width
  const size_t clocal_work_size[3] = {9, 12, 1}; // 
  err = clEnqueueNDRangeKernel(m_queue, m_kernelReorder, workdim, NULL, cglobal_work_size, clocal_work_size, 0, NULL, &timing_event_reorder);
  if (err < 0)
    {
      print_enqueNDrKernel_error(err);
      printf("Couldn't enqueue the chroma kernel execution.\n");
      exit(EXIT_FAILURE);
    }
#endif
  //-----------------------------------------------------------------------------
  //
  // Enqueue a memory read to the output image
  //
  //-----------------------------------------------------------------------------
  cl_bool blocking_read = CL_TRUE;
  size_t offset = 0;
  size_t cb = m_output_buffer_size * sizeof(float);
  err = clEnqueueReadBuffer(m_queue, m_chroma_img, blocking_read, offset, cb, m_outputbuffer, 0, NULL, NULL);
  if (err != CL_SUCCESS)
    {
      perror("Chromagram Error: clEnqueueReadBuffer failed.");
      print_enqueReadBuffer_error(err);
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

  clGetEventProfilingInfo(timing_event, CL_PROFILING_COMMAND_START, sizeof(timestart), &timestart, NULL);
  clGetEventProfilingInfo(timing_event, CL_PROFILING_COMMAND_END, sizeof(timestop), &timestop, NULL);
  *ptime = timestop - timestart;
  //printf("Chroma Kernel execution time=%lums %luus\n", *ptime / 1000, *ptime);
#ifdef REORDERKERNEL
  clGetEventProfilingInfo(timing_event_reorder, CL_PROFILING_COMMAND_START, sizeof(timestart), &timestart, NULL);
  clGetEventProfilingInfo(timing_event_reorder, CL_PROFILING_COMMAND_END, sizeof(timestop), &timestop, NULL);
  *ptime += timestop - timestart;
  //printf("Reorder Kernel execution time=%lums %luus\n", (timestop - timestart) / 1000, timestop - timestart);
#endif
  *ptime = *ptime / 1000;
#endif

  // Deallocate resources
  clReleaseCommandQueue(m_queue);
  return 0;
}

void Copenclchroma::partial_dft_to_note(float *dft_arr, float *pnotes, int note)
{
  int octave;

#ifdef REORDERKERNEL
  for (; note < 9 * 12; note += 12)
    {
      octave = note / 12;
      pnotes[note] = dft_arr[octave];
    }
#else
  int limit;
  int i;
  int octaved[] = {128, 64, 32, 16, 8, 4, 2, 1, 1};
  float real;
  float im;

  for (limit = 0; note < 9 * 12; note += 12)
    {
      i = 0;
      octave = note / 12;
      //pnotes[note] = 0;
      real = 0;
      im = 0;
      for (i = 0; i < octaved[octave]; i++)
	{
	  real += dft_arr[(limit + i) * 2];
	  im   += dft_arr[(limit + i) * 2 + 1];
	}
      pnotes[note] = sqrt(real * real + im * im);
      pnotes[note] /= octaved[octave];
      limit += octaved[octave];
    }
#endif
}

void Copenclchroma::output_recomposition(int updatewidth)
{
  int    col;
  int    result_index;
  int    recomp_index;
  int    note;
  float *dft_arr;
  float *pnotes;

  for (col = 0; col < updatewidth; col++)
    {
      result_index = col * 256 * 12 * 2;
      recomp_index = col * 128;
      for (note = 0; note < 12; note++)
	{
	  dft_arr = &m_outputbuffer[result_index + note * 256 * 2];
	  pnotes  = &m_recomposebuffer[recomp_index];
	  partial_dft_to_note(dft_arr, pnotes, note);
	}
    }
}

void Copenclchroma::chromagram_to_img(unsigned int *pimg, int updatewidth, int circularindex, int imgheight)
{
  int    width  = m_wx;
  float  *fbuffer = m_recomposebuffer;
  int    j;
  int    col;
  int    column_index;
  int    start;
  float  f;
  int    note;
  float  max;

  //printf("updatewidth=%d\n", updatewidth);
  output_recomposition(updatewidth);
  for (col = 0; col < updatewidth; col++)
    {
      column_index = col * 128;
#define USE_COULM_MAX
#ifdef USE_COULM_MAX
      max = 0;
      for (note = C0; note <= C8; note++)
	{
	  if (fbuffer[column_index + note] > max)
	    max = fbuffer[column_index + note];
	}
      max += 1;
#else
      max = m_maxdbvalue;
#define LOGVIEW
#endif
      for (note = C0; note < 9 * 12; note++)
	{
	  int octavenote = note % 12;
	  for (j = 0; j < 4; j++)
	    {
	      int *p = (int*)&pimg[(imgheight - (note * 4) - j - 1) * width];
	      start = (circularindex + col) % width;
	      if (j == 3)
		if (octavenote == 11)
		  p[start] = 0xFFFFFFFF;
		else
		  p[start] = 0x7F7F7F7F;
	      else
		{
		  f = fbuffer[column_index + note];
#ifdef LOGVIEW
		  const int scp = 1000;
		  //p[start] = value_to_color_888(10. * log(scp * f / m_maxdbvalue), 10 * log(scp));
		  p[start] = value_to_color_888(10. * log(scp * f / max), 40);
#else
		  p[start] = value_to_color_888(f, max);
#endif
		}
	    }
	}
    }
}

int Copenclchroma::note_id(int chroma_column, char *note_name, const int namestrsz)
{
  const int   charmonics = 10;
  const float fpowerthreshold = 0.001;
  int absnote;
  int note;
  int octave;
  int h;
  t_note_harmonic har[charmonics];
  float harmonic_score[128];
  float *chromabuffer;
  float max;
  float hused;

  chromabuffer = &m_recomposebuffer[chroma_column * m_recomp_height];
  for (absnote = 0; absnote <= C8; absnote++)
    {
      m_f2n.note_harmonics(absnote, har, charmonics);
//#define MAX_NOTE_ONLY
#ifdef MAX_NOTE_ONLY
      harmonic_score[absnote] = chromabuffer[absnote];
#else
      harmonic_score[absnote] = 0;
      hused = 0;
      for (h = 0; h < charmonics; h++)
	{
	  if (har[h].note < m_usedoctaves * OCTAVESZ)
	    {
	      harmonic_score[absnote] += chromabuffer[har[h].note];
	      hused++;
	      //if (chromabuffer[har[h].note] > 0.01)
		//printf("hnote=%d value=%f\n", har[h].note, chromabuffer[har[h].note]);
	    }
	}
      // Average so that lower notes's harmonics don't take all the power
      harmonic_score[absnote] /= hused;
#endif
    }
  for (absnote = 0, note = 0, max = 0; note <= C8; note++)
    {
      if (harmonic_score[note] > max)
	{
	  absnote = note;
	  max = harmonic_score[note];
	}
    }
  m_f2n.noteoctave(absnote, &note, &octave);
  m_f2n.notename(octave, note, note_name, namestrsz);
  if (max < fpowerthreshold)
    note = C0;
  if (note > C0)
    printf("maxnote is %d freqpower=%f %s\n", absnote, max, note_name);
  return note;
}

