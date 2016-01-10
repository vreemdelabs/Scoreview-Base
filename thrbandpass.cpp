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
#include <math.h>
#include <assert.h>
#include <iterator>
#include <list>

#include <pthread.h>

#include <CL/cl.h>
#include <SDL2/SDL.h>

#include "hann_window.h"
#include "audiodata.h"
#include "audioapi.h"
#include "shared.h"

#include "filterlib.h"
#include "opencldevice.h"
#include "thrbandpass.h"

int g_thrbandpassretval;

void reverse(float *buffer, int size)
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

#define HANN_FCUT 11025

void apply_hann_on_begin_and_end(float *psamples, int sample_num, int samplerate)
{
  int hann_length;
  int i;

  if (sample_num < samplerate / 2)
    {
      for (i = 0; i < sample_num; i++)
	{
	  psamples[i] = psamples[i] * hann(i, sample_num);
	}
    }
  else
    {
      hann_length = HANN_FCUT;
      for (i = 0; i < hann_length / 2 && i < sample_num; i++)
	{
	  psamples[i] = psamples[i] * hann(i, hann_length);
	}
      for (i = 0; i < hann_length / 2 && i < sample_num; i++)
	{
	  psamples[sample_num - i - 1] = psamples[sample_num - i - 1] * hann(i, hann_length);
	}
    }
}

//#define SHOW_FILTERING_TIME

void filter_signal(t_audioOutCmd *pcmd, float **psamples, int samplenum, int samplerate, unsigned long *ptime)
{
#ifdef SHOW_FILTERING_TIME
  double t1, t2;
#endif
  int    band;
  int    i;
  float  bandwidth;
  float  frequency;
  float *in;
  float *out;
  float *wrkbuffer;

#ifdef SHOW_FILTERING_TIME
  t1 = SDL_GetTicks();
#endif
  in        = psamples[0];
  wrkbuffer = psamples[1];
  out       = psamples[2];
  for (i = 0; i < samplenum; i++)
    out[i] = 0;
  // Apply a window function on the sides
  
  // Filter it with a bandpass filter
  for (band = 0; band < pcmd->bands; band++)
    {
      for (i = 0; i < samplenum; i++)
	wrkbuffer[i] = in[i];
      bandwidth = pcmd->fhicut[band] - pcmd->flocut[band];
      frequency = pcmd->fhicut[band] - (bandwidth / 2.);
      //printf("filtering band=%d f=%f bandwidth=%f samplenum=%d\n", band, frequency, bandwidth, samplenum);
      DSPCPPfilter_pass_band_Butterworth(wrkbuffer, samplenum, samplerate, frequency, bandwidth);
      if (pcmd->bsubstract)
	{
	  reverse(wrkbuffer, samplenum);
	  DSPCPPfilter_pass_band_Butterworth(wrkbuffer, samplenum, samplerate, frequency, bandwidth);
	}
      for (i = 0; i < samplenum; i++)
	out[i] += wrkbuffer[i];
    }
  if (pcmd->bsubstract)
    for (i = 0; i < samplenum; i++)
      in[i] = in[i] - out[i];
  else
    for (i = 0; i < samplenum; i++)
      in[i] = out[i];
#ifdef SHOW_FILTERING_TIME
  t2 = SDL_GetTicks();
  *ptime = (long)(t2 - t1);
#else
  *ptime = 0;
#endif
}

void wait_filter_event(t_shared *pshared_data)
{
  pthread_mutex_lock(&pshared_data->cond_soundmutex);
  pthread_cond_wait(&pshared_data->cond_sound, &pshared_data->cond_soundmutex);
  pthread_mutex_unlock(&pshared_data->cond_soundmutex);
}

void* thr_bandpass(void *p_data)
{
  std::list<t_audioOutCmd>::iterator  cmditer;
  t_audioOutCmd                       cut;
  t_shared*       pshared_data = (t_shared*)p_data;
  bool            bquit;
  int             samplerate;
  float           *wrkbuffer;
  float           *outbuffer;
  float           *samplesbuffers[3];
  unsigned long   time;
  unsigned long   total_time;
  int             samplestart;
  int             samplestop;
  int             samplenum;
  bool            bcompleted;
  t_ret_samples   r;
  float           *psamples;
  t_audio_strip   strip;
  std::list<t_audio_strip> tmp_filtered_sound_list;
  std::list<t_audio_strip>::iterator f_sound_iter;
  double          start_timecode;

  g_thrbandpassretval = 0;
#ifdef _DEBUG
  printf("Initialising the bandpass thread\n");
#endif
  LOCK;
  bquit = pshared_data->bquit;
  samplerate = pshared_data->samplerate;
  UNLOCK;
  wrkbuffer = new float[MAX_VIEW_TIME * samplerate];
  outbuffer = new float[MAX_VIEW_TIME * samplerate];
  samplesbuffers[1] = wrkbuffer;
  samplesbuffers[2] = outbuffer;
  while (!bquit)
    {
      SLOCK;
      if (pshared_data->filter_cmd_list.size() > 0)
	{
	  tmp_filtered_sound_list.clear();
	  total_time = 0;
	  // Frankenstein design in order to be able to add new commands and unlock the mutex during the filtering and hence not lock the interface
	  bcompleted = false;
	  while (!bcompleted)
	    {
	      // Find the next command to be filtered
	      cmditer = pshared_data->filter_cmd_list.begin();
	      while (cmditer != pshared_data->filter_cmd_list.end() &&
		     (*cmditer).notestate != state_wait_filtering)
		{
		  cmditer++;
		}
	      // Check if finished
	      bcompleted = (cmditer == pshared_data->filter_cmd_list.end());
	      if (!bcompleted)
		{
		  // Get the data
		  assert((*cmditer).notestate == state_wait_filtering);
		  (*cmditer).notestate = state_filtered;
		  cut = (*cmditer); // Copy it because it could be delete in between unlocks
		  SUNLOCK;
		  samplestart = cut.start * samplerate - (samplerate / HANN_FCUT) / 2;
		  samplestop  = cut.stop * samplerate + (samplerate / HANN_FCUT) / 2;
		  samplenum = samplestop - samplestart;
		  if (samplestart < samplestop)
		    {
		      psamples = new float[samplenum];
		      LOCK;
		      r = pshared_data->pad->get_data(samplestart, psamples, samplenum);
		      UNLOCK;
		      samplenum = r.obtained;
		      // Parallel filtering of bands
		      samplesbuffers[0] = psamples; // Filtering output
		      filter_signal(&cut, samplesbuffers, samplenum, samplerate, &time);
		      total_time += time;
		      apply_hann_on_begin_and_end(psamples, samplenum, samplerate);
		      //printf("playdelay is %f\n", cut.playdelay);
		      strip.playtimecode = cut.playdelay;
		      strip.samples = psamples;
		      strip.samplenum = samplenum;
		      strip.current_sample = 0;
		      tmp_filtered_sound_list.push_front(strip);
		      //printf("listsize ==%d\n", (int)pshared_data->filter_cmd_list.size());
		    }
		  SLOCK;
		}
	      // The command are erased in the rendering function
	    }
	  //---------------------------------------------------------------------------------
	  // All the notes must start at the same time otherwise it will mess the rythm up.
	  // Therefore update the srtips only when they are filtered, filtering takes 
	  // a noticeable time.
	  //---------------------------------------------------------------------------------
	  start_timecode = 0.001 * SDL_GetTicks(); // Playing the notes start here
	  FLOCK;
	  f_sound_iter = tmp_filtered_sound_list.begin();
	  while (f_sound_iter != tmp_filtered_sound_list.end())
	    {
	      // Update the first -1 element
	      (*f_sound_iter).playtimecode += start_timecode;
	      pshared_data->filtered_sound_list.push_front((*f_sound_iter));
	      f_sound_iter++;
	    }
	  FUNLOCK;
	  cmditer = pshared_data->filter_cmd_list.begin();
	  while (cmditer != pshared_data->filter_cmd_list.end())
	    {
	      if ((*cmditer).notestate == state_filtered)
		{
		  (*cmditer).playstart = start_timecode + (*cmditer).playdelay;
		  (*cmditer).notestate = state_ready_2play;
		}
	      cmditer++;
	    }
	}
      SUNLOCK;
#ifdef SHOW_FILTERING_TIME
      printf("Bandpass filtering took %lums.\n", total_time);
#endif
      // Wait for something
      wait_filter_event(pshared_data);
      LOCK;
      bquit = pshared_data->bquit;
      UNLOCK;
    }
  delete[] wrkbuffer;
  delete[] outbuffer;
#ifdef _DEBUG
  printf("Bandpass thread closing\n");
#endif
  return &g_thrbandpassretval;
}

