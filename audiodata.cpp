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
#include <string.h>

#include <iterator>
#include <list>

#include "audiodata.h"

CaudioChunk::CaudioChunk():
  m_size(0),
  m_used(0),
  m_pdata(NULL)
{
  try
    {
      m_pdata = new float[AUDIO_CHUNK_SIZE];
      m_size = AUDIO_CHUNK_SIZE;
    }
  catch (int e)
    {
      printf("Error in audio chunk allocation\n");
    }
}

CaudioChunk::~CaudioChunk()
{
  delete[] m_pdata;
}

// Adds data to the current buffer and returns how many samples did not fit in it
int CaudioChunk::add_data(float *pdata, int size)
{
  int rsize;
  
  rsize = m_used + size > AUDIO_CHUNK_SIZE ? AUDIO_CHUNK_SIZE - m_used : size;
  memcpy(m_pdata + m_used, pdata, rsize * sizeof(float));
  m_used += rsize;
  return (size - rsize);
}

bool CaudioChunk::spaceavailable()
{
  return (m_used < m_size);
}

Caudiodata::Caudiodata()
{
  m_CaudioChunk_list.clear();
}

Caudiodata::~Caudiodata()
{
  for (m_iter = m_CaudioChunk_list.begin(); m_iter != m_CaudioChunk_list.end(); m_iter++)
    delete *m_iter;
}

int Caudiodata::add_data(float *pdata, int size)
{
  int          local_size;
  CaudioChunk* pchunk;
  std::list<CaudioChunk*>::iterator iter;

  local_size = size;
  // Look for space in the last chunk
  if (m_CaudioChunk_list.begin() != m_CaudioChunk_list.end())
    {
      iter = m_CaudioChunk_list.end();
      iter--;
      pchunk = *iter;
      // Sapce left
      if (pchunk->spaceavailable())
	{
	    local_size = pchunk->add_data(pdata, local_size);
	}
    }
  if (local_size > 0)
    {
      pchunk = new CaudioChunk();
      local_size = pchunk->add_data(pdata, local_size);
      m_CaudioChunk_list.push_back(pchunk);
      while (local_size > 0)
	{
	  pchunk = new CaudioChunk();
	  local_size = pchunk->add_data(pdata + size - local_size, local_size);
	  m_CaudioChunk_list.push_back(pchunk);
	}
    }
  return 0;
}

CaudioChunk *Caudiodata::get_first()
{
  m_iter = m_CaudioChunk_list.begin();
  if (m_iter == m_CaudioChunk_list.end())
    return (NULL);
  return (*m_iter);
}

CaudioChunk *Caudiodata::get_next()
{
  m_iter++;
  if (m_iter == m_CaudioChunk_list.end())
    return (NULL);
  return (*m_iter);  
}

// Returns the number of samples stored in the class
long Caudiodata::get_samplenum()
{
  CaudioChunk *pchunk = get_first();
  long        samplenum = 0;

  while (pchunk != NULL)
    {
      samplenum += pchunk->m_used;
      pchunk = get_next();
    }
  return samplenum;
}

// Copies size samples from the position samplepos to pdataout, returns the number of samples copied
t_ret_samples Caudiodata::get_data(int samplepos, float *pdataout, int size)
{
  int currentsample = 0;
  CaudioChunk *pchunk;
  int inchunkindex;
  int copysize;
  int progress;
  t_ret_samples r;

  r.obtained = 0;
  r.missing_left = 0;
  r.missing_right = size;
  if (samplepos < 0)
    {
      r.missing_left = -samplepos;
      r.missing_right -= r.missing_left;
      size += samplepos;
      samplepos = 0;
      if (size < 0)
	return r;
    }
  pchunk = get_first();
  if (pchunk == NULL)
    {
      printf("Sample not available at this offset\n");
      return (r);
    }
  while (currentsample + pchunk->m_used <= samplepos)
    {
      currentsample += pchunk->m_used;
      pchunk = get_next();
      if (pchunk == NULL)
	{
	  printf("Sample not available at this offset\n");
	  return (r);
	}
    }
  inchunkindex = samplepos - currentsample;
  // Copy the samples from the consecutive chunks of data
  progress = 0;
  while (progress < size)
    {
      copysize = (pchunk->m_used - inchunkindex) > size - progress? size - progress : (pchunk->m_used - inchunkindex);
      memcpy(pdataout + progress, pchunk->m_pdata + inchunkindex, copysize * sizeof(float));
      progress += copysize;
      inchunkindex = 0;
      pchunk = get_next();
      if (pchunk == NULL)
	break;
    }
  r.obtained = progress;
  r.missing_right = size - progress;
  return(r);
}

// Returns the sample n
float Caudiodata::get_sample(int n)
{
  CaudioChunk *pchunk = get_first();
  long        samplenum = 0;
  long        prevsamplenum;

  if (pchunk->m_used > n)
    return (pchunk->m_pdata[n]);
  while (pchunk != NULL)
    {
      prevsamplenum = samplenum;
      samplenum += pchunk->m_used;
      if (samplenum > n)
	{
	  return (pchunk->m_pdata[n - prevsamplenum]);
	}
      pchunk = get_next();
    }
  return 0;
}

