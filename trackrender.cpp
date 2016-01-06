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
#include <math.h>
#include <assert.h>

#include <iterator>
#include <list>

#include <SDL2/SDL.h>

#include "audiodata.h"
#include "trackrender.h"

Ctrack_render::Ctrack_render(int w, int h):
  m_wrk_size(0),
  m_divf(0),
  m_group_size(1),
  m_width(w),
  m_height(h)
{
  assert(m_width == DIV_LIMIT); // Spectrogram buffer size reused here
  reset();
}

Ctrack_render::~Ctrack_render()
{
}

void Ctrack_render::reset()
{
  int i;
  t_track_data td;

  m_wrk_size = 0;
  m_divf = 0;
  m_group_size = 1;
  assert(m_width == DIV_LIMIT);
  td.pmax = td.nmax = td.pmid = td.nmid = 0;
  for (i = 0; i < 2 * DIV_LIMIT; i++)
    m_pwrksamples[i] = td;
}

void Ctrack_render::merge_work_data(t_track_data *pwrk)
{
  int dest, src;

  src = 0;
  for (dest = 0; dest < DIV_LIMIT; dest++)
    {
      pwrk[dest].pmax = pwrk[src].pmax > pwrk[src + 1].pmax? pwrk[src].pmax : pwrk[src + 1].pmax;
      pwrk[dest].nmax = pwrk[src].nmax > pwrk[src + 1].nmax? pwrk[src].nmax : pwrk[src + 1].nmax;
      pwrk[dest].pmid = (pwrk[src].pmid + pwrk[src + 1].pmid) / 2;
      pwrk[dest].nmid = (pwrk[src].nmid + pwrk[src + 1].nmid) / 2;
      src += 2;
    }
}

void Ctrack_render::get_group_mid_e_max(Caudiodata *pad, int total_size, int groupsize, int lastgroup)
{
  int j;
  int posi;
  t_track_data td;
  int samplepos;
  const int clocalsz = 512;
  float local[clocalsz];
  t_ret_samples ret;
  float sample;
  int   sz;
  int   e;

  td.pmax = td.nmax = td.pmid = td.nmid = 0;
  samplepos = groupsize * lastgroup;
  sz = clocalsz < groupsize? clocalsz : groupsize;
  for (j = 0, posi = 0; j < groupsize; j += sz)
    {
      ret = pad->get_data(samplepos + j, local, sz);
      assert(samplepos + j < total_size);
      assert(samplepos + j >= 0);
      assert(ret.obtained == sz);
      for (e = 0; e < sz; e++)
	{
	  //sample = pad->get_sample(samplepos + j);
	  sample = local[e];
	  sample = sample * float(m_height); // +1 -1 -> +h -h
	  if (sample > 0)
	    {
	      td.pmax = sample > td.pmax? sample : td.pmax;
	      td.pmid += sample;
	      posi++;
	    }
	  else
	    {
	      td.nmax = fabs(sample) > td.nmax? fabs(sample) : td.nmax;
	      td.nmid += fabs(sample);
	    }
	}
    }
  td.pmid = posi != 0? td.pmid / posi : 0;
  td.nmid = (groupsize - posi) != 0? td.nmid / (groupsize - posi) : 0;
  // Store
  assert(m_width == DIV_LIMIT);
  assert((lastgroup < 2 * DIV_LIMIT) && lastgroup >= 0);
  m_pwrksamples[lastgroup] = td;
}

//#define SHOW_TRACK_TIME

void Ctrack_render::add_new_data(Caudiodata *pad)
{
  int total_size;
  int new_samples, prsamples;
#ifdef SHOW_TRACK_TIME
  double t1, t2;

  t1 = SDL_GetTicks();
#endif
  total_size = pad->get_samplenum();
  new_samples = total_size - (m_group_size * m_wrk_size);
  // Truncate to group size
  if (new_samples < m_group_size)
    return;
  //printf("size=%d groupsize=%d nbgroups=%d\n", total_size, new_samples, nbgroups);
  prsamples = new_samples;
  while (prsamples > m_group_size)
    {
      get_group_mid_e_max(pad, total_size, m_group_size, m_wrk_size);
      prsamples -= m_group_size;
      m_wrk_size++;
      // Is it full
      if (m_wrk_size >= DIV_LIMIT * 2)
	{
//	  printf("size=%d newsamples=%d remaining=%d nbgroups=%d groupsize=%d\n", total_size, new_samples, prsamples, nbgroups, m_group_size);
	  // Divide the size by two
	  merge_work_data(m_pwrksamples);
	  // Half groups of twice the size
	  m_wrk_size /= 2;
	  m_divf++;
	  m_group_size = 1 << m_divf;
	}
    }
#ifdef SHOW_TRACK_TIME
  t2 = SDL_GetTicks();
  printf("Adding track data took %fms\n", t2 - t1);
#endif
}

int Ctrack_render::get_backgcolor(int max, int h2, int v, bool bcurrent_disp)
{
  int color;
  float dif;
  float pos;

  //printf("values max=%d h2=%d v=%d\n", max, h2, v);
  dif = h2 - max;
  pos = v - max;
  if (bcurrent_disp)
    color = 250;
  else
    color = (int)(pos * 42. / dif) & 0xFF;
  color = (color << 16) | (color << 8) | color;
  return color;
}

int Ctrack_render::psrandom(int max)
{
  static int seed = 0x1FEA3412;
  int i;

  for (i = 0; i < 3; i++)
    {
      seed = seed + (seed << (seed & 3));
      seed = seed ^ seed >> 1;
    }
  seed++;
  return seed;
}

void  Ctrack_render::get_antialiased_values(int i, int &max, int &mid, int &nmax, int &nmid)
{
  float ratio1, ratio2;
  float wrksize, dsize;
  float srcpos;
  int   j;
  int   r;
  t_track_data *pwrk = m_pwrksamples;
  
  dsize = (float)m_width;
  wrksize = (float)m_wrk_size;
  srcpos = (float)i * wrksize / dsize;
/*
  j = round(srcpos);
  max = pwrk[j].pmax;
  nmax = pwrk[j].nmax;
  mid = pwrk[j].pmid;
  nmid = pwrk[j].nmid;
  return;
*/
  ratio2 = srcpos - floor(srcpos);
  ratio1 = 1. - ratio2;
  j = floor(srcpos);
  j = j + 2 > m_wrk_size? m_wrk_size - 2 : j;
  max = pwrk[j].pmax > pwrk[j + 1].pmax? pwrk[j].pmax : pwrk[j + 1].pmax;
  nmax = pwrk[j].nmax > pwrk[j + 1].nmax? pwrk[j].nmax : pwrk[j + 1].nmax;
  mid = (pwrk[j].pmid * ratio1) + (pwrk[j + 1].pmid * ratio2);
  nmid = (pwrk[j].nmid * ratio1) + (pwrk[j + 1].nmid * ratio2);
  // It's alive
  r = psrandom(max);
  r = (r & (16 | 4)) == (16 | 4)? 1 : 0;
  max += r;
  mid += r;
  nmax -= r;
  nmid -= r;
}

void Ctrack_render::render_track_data(int dispsize, int currentsample, int *pimg, int sz)
{
  int i;
  int j;
  int fgcolor;
  int midcolor;
  int color;
  int max, mid;
  int nmax, nmid;
  bool bcurrent_disp;
  int  samplepos;
  float cgsize;
  bool  bpresentbar;
#ifdef SHOW_TRACK_TIME
  double t1, t2;

  t1 = SDL_GetTicks();
#endif
  assert(m_width == DIV_LIMIT);
  cgsize = (float)m_group_size * (float)m_wrk_size / m_width;
  //printf("current sample: %d samplevietime=%d\n", currentsample, dispsize);
  for (i = 0; i < m_width; i++)
    {
      get_antialiased_values(i, max, mid, nmax, nmid);
      // Draw the column
      samplepos = i * cgsize;
      bpresentbar = (samplepos >= currentsample - (dispsize / 2) &&
		     samplepos <  (currentsample - (dispsize / 2) + ceil((float)currentsample / (float)m_width)));
      if ((samplepos > currentsample - dispsize) && (samplepos < currentsample))
	{
	  bcurrent_disp = true;
	  fgcolor = 0x4242A1;
	  midcolor = 0xA0A0FF;
	}
      else
	{
	  bcurrent_disp = false;
	  fgcolor = 0xEEE024;
	  midcolor = 0xDF9423;
	}
      for (j = 0; j < m_height; j++)
	{
	  int ind = j - (m_height / 2);
	  // Upper side
	  if (ind < 0)
	    {
	      ind = abs(ind);
	      if (ind > max)
		{
		  color = get_backgcolor(max, m_height / 2, ind, bcurrent_disp);
		}
	      else
		{
		  if (ind > mid)
		    color = fgcolor;
		  else
		    color = midcolor;
		}
	    }
	  else
	    {
	      if (ind > nmax)
		color = get_backgcolor(nmax, m_height / 2, ind, bcurrent_disp);
	      else
		{
		  if (ind > nmid)
		    color = fgcolor;
		  else
		    color = midcolor;
		}
	    }
	  color = bpresentbar? 0xFF7F7F7F : color;
	  // FIXME swaped R and B
	  color = (color & 0xFF00FF00) |  (color & 0x00FF0000) >> 16 | (color & 0x000000FF) << 16;
	  assert(j * m_width + i < sz);
	  pimg[j * m_width + i] = color;
	}
    }
#ifdef SHOW_TRACK_TIME
  t2 = SDL_GetTicks();
  printf("Drawing the track took %fms\n", t2 - t1);
#endif
}

