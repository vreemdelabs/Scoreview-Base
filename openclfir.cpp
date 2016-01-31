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

#include <CL/cl.h>

#include "spectrometre.h"
#include "colorscale.h"
#include "hann_window.h"
#include "opencldevice.h"
#include "opencldft.h"

// Low frequency decimation
#include <SDL2/SDL.h>
#include "filterlib.h"
#ifdef USEFIR_KERNEL
#include "fircoef.h"
#endif

void Copenclspectrometer::reverse(float *buffer, int size)
{
  int   i;
  float tmp;
  
  for (i = 0; i < size / 2; i++)
    {
      tmp = buffer[i];
      buffer[i] = buffer[size - i - 1];
      buffer[size - i - 1] = tmp;
    }
}

void Copenclspectrometer::filterloop(float *samples, float *pcoeffs, t_fir_params *params)
{
  double sum;
  int    i, step;
  float  *scopy;
  int    numinsamples;
  int    numoutsamples;
  int    sample;
  
  numinsamples = params->intracksize;
  scopy = new float[numinsamples];
  for (i = 0; i < numinsamples; i++)
    scopy[i] = samples[i];
  numoutsamples = params->intracksize / params->decimation;
  for (i = 0; i < numoutsamples; i++)
    {
      sum = 0;
      for (step = 0; step < FIR_SIZE; step++)
	{
	  sample = (i * params->decimation) - 256 + step;
	  if ((sample >= 0) && (sample < numinsamples))
	    sum += pcoeffs[step] * scopy[sample];
	}
      samples[i] = sum;
    }
  delete[] scopy;
}

void Copenclspectrometer::downsample(float *psamples, int samplenum, int scut)
{
  int s, s2;

  for (s = 0, s2 = 0; s < samplenum; s += scut)
    {
      psamples[s2++] = psamples[s];
    }
}

void Copenclspectrometer::filter_signal_zfactor(float *psamples, int *psamplenum, int start, int stop, float fmax)
{
//return;
  float fact[] = {64, 32, 16, 8, 4};
  int   i;
  float cutfrequency;
  int   scut;
  int   arsize;
  int   precision = 0;

  m_bhalfband = false;
  arsize = (int)(sizeof(fact) / sizeof(float));
  for (i = 0; i < arsize; i++)
    {
      if ((int)(2. * fmax) < (m_sampling_frequency / fact[i]) - precision)
	{
	  scut = fact[i] / 2;
	  cutfrequency = (float)m_sampling_frequency / fact[i];
#ifdef USEFIR_KERNEL
	  switch (scut * 2)
	    {
	    case 4:
	      m_pfir_coefs = xcoeffs4; // cuttoff = f / 4 samplingf = f / 2
	      break;
	    case 8:
	      m_pfir_coefs = xcoeffs8;
	      break;
	    case 16:
	      m_pfir_coefs = xcoeffs16;
	      break;
	    case 32:
	      m_pfir_coefs = xcoeffs32;
	    default:
	      m_pfir_coefs = xcoeffs64;
	      break;
	    }
	  if (scut >= 2)
	    {
	      m_bhalfband = true;
	      m_kfirparams.intracksize = *psamplenum;
	      m_kfirparams.decimation = scut;
//#define CPU_FIR
#ifdef CPU_FIR
	      double t1 = SDL_GetTicks();
	      m_bhalfband = false;
	      //downsample(psamples, *psamplenum, scut);
	      filterloop(psamples, m_pfir_coefs, &m_kfirparams);
	      double t2 = SDL_GetTicks();
	      //printf("filtering took %fms\n", t2 - t1);
#endif
	    }
#else
	  double t1 = SDL_GetTicks();
	  DSPCPPfilter_low_pass_Butterworth(psamples, *psamplenum, m_sampling_frequency, cutfrequency);
	  reverse(psamples, *psamplenum);
	  DSPCPPfilter_low_pass_Butterworth(psamples, *psamplenum, m_sampling_frequency, cutfrequency);
	  reverse(psamples, *psamplenum);
	  downsample(psamples, *psamplenum, scut);
	  double t2 = SDL_GetTicks();
	  //printf("filtering took %fms\n", t2 - t1);
#endif
	  m_kparams.tracksize /= scut;
	  m_kparams.start_sample /= scut;
	  m_kparams.stop_sample /= scut;
	  m_kparams.sampling_frequency = 2 * cutfrequency;
	  m_kparams.N = get_N(m_kparams.sampling_frequency);
	  *psamplenum /= scut;
	  //printf("cutoff=%f decimation=%d\n", cutfrequency, m_kfirparams.decimation);
	  return;
	}
    }
}

void Copenclspectrometer::allocate_fir_buffers()
{
  cl_int err;

  // Create an additional sample buffer for fir filtering
  m_firsamplebuffer = clCreateBuffer(m_context,
				     CL_MEM_READ_ONLY,
				     (MAX_SPECTROMETER_LENGTH_SECONDS + 2) * m_sampling_frequency * sizeof(float), NULL, &err);
  if (err < 0)
    {
      perror("Couldn't create the GPU track buffer");
      exit(1);
    }
  // FIR coefficient global memory buffer
  m_fircoef = clCreateBuffer(m_context,
			     CL_MEM_READ_ONLY,
			     FIR_SIZE * sizeof(float), NULL, &err);
  if (err < 0)
    {
      perror("Couldn't create the GPU track buffer");
      exit(1);
    }
  // FIR kernel parameter structure
  m_fircfg = clCreateBuffer(m_context,
			    CL_MEM_READ_ONLY,
			    sizeof(m_kfirparams), NULL, &err);
}

void Copenclspectrometer::set_decimation_arguments()
{
  cl_int err;

  if (m_bhalfband)
    {
      err = clSetKernelArg(m_firkernel, 0, sizeof(m_firsamplebuffer), &m_firsamplebuffer);
      if (err < 0)
	{
	  perror("Couldn't set the track in buffer as the FIR kernel argument");
	  exit(1);
	}
      err = clSetKernelArg(m_firkernel, 1, sizeof(m_samples), &m_samples);
      if (err < 0)
	{
	  perror("Couldn't set the track out buffer as the FIR kernel argument");
	  exit(1);
	}
      err = clSetKernelArg(m_firkernel, 2, sizeof(m_fircfg), &m_fircfg);
      if (err < 0)
	{
	  perror("Couldn't set the parameters as the FIR kernel argument");
	  exit(1);
	}
      err = clSetKernelArg(m_firkernel, 3, sizeof(m_fircoef), &m_fircoef);
      if (err < 0)
	{
	  perror("Couldn't set the parameters as the FIR kernel argument");
	  exit(1);
	}
    }
}

bool Copenclspectrometer::enqueue_fir_kernels(cl_event *ptimingevents)
{
  cl_uint workdim;
  size_t  global_work_size[1];
  size_t  local_work_size[1];
  cl_int  err;
  int     samplenum;
  size_t  max_workgroup_size;
 
  //-----------------------------------------------------------------------------
  //
  // Enqueue a FIR filtering kernel
  //
  //-----------------------------------------------------------------------------
  if (m_kparams.tracksize < 256)
    {
      return false; // Return if not enough samples, FIXME it messes with the begining of the screen
    }
  if (m_bhalfband)
    {
      err = clGetKernelWorkGroupInfo(m_kernel, m_device, CL_KERNEL_WORK_GROUP_SIZE,
				     sizeof(max_workgroup_size), &max_workgroup_size, NULL);
      if (!m_bdefautlsizeswritten)
	{
	  printf("max fir kernel workgroup size=%d\n", (int)max_workgroup_size);
	  m_bdefautlsizeswritten = true;
	}
      // Fir parameters
      err = clEnqueueWriteBuffer(m_queue, m_fircfg, CL_TRUE, 0, sizeof(m_kfirparams), &m_kfirparams, 0, NULL, NULL);
      if (err != CL_SUCCESS)
	{
	  perror("Error: clEnqueueWriteBuffer failed.");
	  return false;
	}
      // Fir coefficients for initialisation
      err = clEnqueueWriteBuffer(m_queue, m_fircoef, CL_TRUE, 0, FIR_SIZE * sizeof(float), m_pfir_coefs, 0, NULL, NULL);
      if (err != CL_SUCCESS)
	{
	  perror("Error: clEnqueueWriteBuffer failed.");
	  return false;
	}
      workdim = 1;
      // Overflow compensated with a test inside the kernel
      samplenum = m_kparams.tracksize;
      global_work_size[0] = (samplenum / 256) * 256;
      //local_work_size[0] = 256;
      local_work_size[0] = 256 < max_workgroup_size? 256 : max_workgroup_size;
      //printf("Queuing a kernel of op%lu local=%lu\n", global_work_size[0], local_work_size[0]);
      err = clEnqueueNDRangeKernel(m_queue, m_firkernel, workdim, NULL, global_work_size, local_work_size, 0, NULL, &ptimingevents[1]);
      if (err < 0)
	{
	  print_enqueNDrKernel_error(err);
	  printf("Couldn't enqueue the fir kernel execution");
	  exit(1);
	}
    }
  return true;
}

