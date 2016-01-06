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
#include <stdlib.h>
#include "assert.h"
//#include "portaudio.h"
#include "pa_ringbuffer.h"
#include "pa_memorybarrier.h"

#include "audiofifo.h"

CaudioFifo::CaudioFifo(int samplerate, int framesize):
  m_frame_size(framesize),
  m_bquit(false)
{
  int i;

  m_frame_count = 32; // Must be a power of 2
  m_buffers_size = m_frame_count * framesize;
  m_psamples_in = new float[m_buffers_size];
  if (PaUtil_InitializeRingBuffer(&m_rbufin, framesize * sizeof(float), m_frame_count, m_psamples_in) == -1)
    {
      printf("Portaudio ringbuffer error: element count is not a power of 2.\n");
      exit(EXIT_FAILURE);
    }
  m_psamples_out = new float[m_buffers_size];
  if (PaUtil_InitializeRingBuffer(&m_rbufout, framesize * sizeof(float), m_frame_count, m_psamples_out) == -1)
    {
      printf("Portaudio ringbuffer error: element count is not a power of 2.\n");
      exit(EXIT_FAILURE);
    }
  for (i = 0; i < m_buffers_size; i++)
    {
      m_psamples_in[i] = m_psamples_out[i] = 0;
    }
}

CaudioFifo::~CaudioFifo()
{
  delete[] m_psamples_in;
  delete[] m_psamples_out;
}

void CaudioFifo::push_input_samples(float *psamples, int size)
{
  int available;
  ring_buffer_size_t elementCount;

  assert(size == m_frame_size);
  available = PaUtil_GetRingBufferWriteAvailable(&m_rbufin);
  if (available > 0)
    {
      elementCount = 1;
      PaUtil_WriteRingBuffer(&m_rbufin, psamples, elementCount);      
    }
}

bool CaudioFifo::pop_input_samples(float *psamples, int size)
{
  int available;
  ring_buffer_size_t elementCount;

  assert(size == m_frame_size);
  available = PaUtil_GetRingBufferReadAvailable(&m_rbufin);
  if (available > 0)
    {
      elementCount = 1;
      PaUtil_ReadRingBuffer(&m_rbufin, psamples, elementCount);
      return true;
    }
  return false;
}

void CaudioFifo::push_output_samples(float *psamples, int size)
{
  int available;
  ring_buffer_size_t elementCount;

  assert(size == m_frame_size);
  available = PaUtil_GetRingBufferWriteAvailable(&m_rbufout);
  if (available > 0)
    {
      elementCount = 1;
      PaUtil_WriteRingBuffer(&m_rbufout, psamples, elementCount);
    }
}

int CaudioFifo::pop_output_samples(float *psamples, int size)
{
  int                available;
  ring_buffer_size_t elementCount;
  int                i;

  assert(size == m_frame_size);
  available = PaUtil_GetRingBufferReadAvailable(&m_rbufout);
  if (available > 0)
    {
      elementCount = 1;
      PaUtil_ReadRingBuffer(&m_rbufout, psamples, elementCount);
    }
  else
    {
      for (i = 0; i < m_frame_size; i++)
	{
	  psamples[i] = 0;
	}
    }
  return available;
}

void CaudioFifo::set_quit(bool bquit)
{
  m_bquit = bquit;
}

bool CaudioFifo::get_quit()
{
  return m_bquit;
}

