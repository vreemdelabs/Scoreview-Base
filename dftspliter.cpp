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

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include <iterator>
#include <list>

#include <pthread.h>

#include <SDL2/SDL.h>
#include <CL/cl.h>

#include "audiodata.h"
#include "audioapi.h"
#include "shared.h"
#include "spectrometre.h"
#include "opencldevice.h"
#include "opencldft.h"
//#include "thrportaudio.h"
#include "dftspliter.h"

int g_ct = 0;
int g_tm = 479581320;
int g_dur = 0;
int g_lcount = 0;

Cdftspliter::Cdftspliter(Copenclspectrometer *poclSpectrometer, int width, int samplerate):
  m_pdft(poclSpectrometer),
  m_processed_pixels(width),
  m_circularindex(width),
  m_samplerate(samplerate)
{
  m_previous_end_timecode = -1;
  m_previous_missing_samples = 0;
  m_previous_max_frequ = -1;
  m_previous_viewtime = -1;
  m_previouss_view_type = view_unknown;
  m_circularindex = 0;
  // Local buffer to store the samples sent to the video card
  m_buffer_size = (MAX_SPECTROMETER_LENGTH_SECONDS + 8) * m_samplerate;
  m_psamples = new float[m_buffer_size];
  m_preorganise_buffer = new float[2 * m_buffer_size];
  m_dftclist.clear();
  m_dftciter = m_dftclist.end();
}

Cdftspliter::~Cdftspliter()
{
  delete[] m_preorganise_buffer;
  delete[] m_psamples;
}

void Cdftspliter::get_needed_samples(Caudiodata *pad, t_dft_chunk *pch, double T)
{
  int samplesdisplayed;
  int Tsamples;
  int samplesneeded;
  int begin;
  //int buffersize;
  int obtainedsamples;
  int i;
  t_ret_samples r;

  if (pch->sample_stop > pad->get_samplenum())
    {
      printf("Not enough samples, filling with 0s.\n");
      //pch->sample_stop = pad->get_samplenum();
    }
  samplesdisplayed = pch->sample_stop - pch->sample_start;
  Tsamples = T * m_samplerate;
  // Adds 516/2 samples for the fir filtering if any
  samplesneeded = samplesdisplayed + 2 * Tsamples + 2 * FIR_SIZE;
  begin = pch->sample_start - Tsamples - FIR_SIZE;
  assert(samplesneeded < m_buffer_size);
  r = pad->get_data(begin, m_psamples, samplesneeded);
  obtainedsamples = r.obtained;
  // If nothing is obtained
  if (r.missing_right == samplesneeded || r.missing_left == samplesneeded)
    {
      for (i = 0; i < samplesneeded; i++)
	m_psamples[i] = 0;
      pch->sample_start = Tsamples + FIR_SIZE;
      pch->sample_stop  = pch->sample_start + samplesdisplayed;
      pch->totalsamples = samplesneeded;
    }
  else
    {
      // If less then requested samples
      if (r.missing_left != 0 ||
	  r.missing_right != 0)
	{
	  //if (begin + Tsamples + samplesdisplayed == pad->get_samplenum())
	  //  printf("end of the track-------------------------------------------\n");
	  //printf("missing left %d right %d\n", r.missing_left, r.missing_right);
	  // Fill with zeros
	  for (i = 0; i < samplesneeded; i++)
	    m_preorganise_buffer[i] = 0;
	  for (i = 0; i < obtainedsamples; i++)
	    m_preorganise_buffer[r.missing_left + i] = m_psamples[i];
	  // Change the sample numbers to the kernel sample buffer values
	  pch->sample_start = Tsamples + FIR_SIZE;
	  pch->sample_stop  = samplesneeded - Tsamples - FIR_SIZE;
	  pch->totalsamples = obtainedsamples - FIR_SIZE;
	  // Copy the correcterd result
	  for (i = 0; i < samplesneeded; i++)
	    m_psamples[i] = m_preorganise_buffer[i];
	}
      else
	{
	  // Change the reference from all the record buffer to the offsets in the chunk to be sent to the kernel
	  pch->sample_start = Tsamples + FIR_SIZE;
	  pch->sample_stop  = pch->sample_start + samplesdisplayed;
	  pch->totalsamples = samplesneeded - FIR_SIZE;
	}
    }
  pch->totalsamples = pch->totalsamples < 0? 0 : pch->totalsamples;
}

bool Cdftspliter::compare(const t_dft_chunk& first, const t_dft_chunk& second)
{
  return (first.sample_start < second.sample_start);
}

void Cdftspliter::cut_out_of_track_chunks(double last_time, s_dft_image_update *pimup, double viewtime, int width, bool bforward)
{
  t_dft_chunk ch;
  int         processed_pixels;
  double      dtime;
  double      pixels_per_second;

  pixels_per_second = (double)width / (double)viewtime;
  ch.creation_time = last_time;
  ch.type = chunk_out_of_track;
  ch.sample_start = ch.sample_stop = 0;
  ch.totalsamples = 0;
  ch.max_freq = 0;
  ch.bforward = bforward;
  if (pimup->update_begin_timecode <= 0)
    {
      pimup->update_begin_timecode = pimup->screen_begin_timecode;
      ch.tcode_start = pimup->update_begin_timecode;
      ch.tcode_stop = 0;
      dtime = ch.tcode_stop - ch.tcode_start;
      processed_pixels = dtime * pixels_per_second;
      ch.pixel_xstart = 0;
      ch.pixel_xstop = processed_pixels;
      ch.pixel_xstop = ch.pixel_xstop >= width? width : ch.pixel_xstop;
      //printf("adding the left out of track chunk, changing the timecodes %f - %f\n", pimup->update_begin_timecode, pimup->update_end_timecode);
      m_dftclist.push_back(ch);
      if (pimup->update_end_timecode <= 0)
	{
	  pimup->update_begin_timecode = pimup->update_end_timecode = 0;
	}
      else
	pimup->update_begin_timecode = 0;
      //printf("new timecodes %f - %f\n", pimup->update_begin_timecode, pimup->update_end_timecode);
    }
  if (pimup->update_end_timecode > pimup->tracklen)
    {
      ch.tcode_start = pimup->tracklen;
      ch.tcode_stop = pimup->update_end_timecode = pimup->screen_end_timecode;
      dtime = ch.tcode_stop - ch.tcode_start;
      processed_pixels = dtime * pixels_per_second;
      ch.pixel_xstart = width - ((pimup->screen_end_timecode - ch.tcode_start ) * pixels_per_second);
      ch.tcode_start = ch.tcode_start < 0? 0 : ch.tcode_start;
      //ch.pixel_xstop = ch.pixel_xstart + processed_pixels;
      //ch.pixel_xstop  = ch.pixel_xstop > width? width : ch.pixel_xstop;
      ch.pixel_xstop  = width;
      //printf("adding the right out of track chunk of %d pixels\n", ch.pixel_xstop - ch.pixel_xstart);
      m_dftclist.push_back(ch);
      pimup->update_end_timecode = pimup->tracklen;
      pimup->update_begin_timecode = pimup->update_begin_timecode > pimup->tracklen? pimup->tracklen : pimup->update_begin_timecode;
    }
}

void Cdftspliter::cut_dft_to_chunks(double last_time, s_dft_image_update *pimup, double viewtime, int width, float max_freq, bool bforward)
{
  t_dft_chunk ch;
  int         totalpixels, chunkxstart;
  double      dtime, processed_pixels_duration;
  double      pixels_per_second;
  int         steps;
  int         step;

  m_dftclist.clear();
  // First of all create empty chunks for outside of the track
  cut_out_of_track_chunks(last_time, pimup, viewtime, width, bforward);
  if (pimup->update_begin_timecode == pimup->update_end_timecode)
    {
      m_dftciter = m_dftclist.begin();
      return;
    }
  //
  ch.creation_time = last_time;
  ch.type = chunk_on_track;
  ch.bforward = bforward;
  ch.max_freq = max_freq;
  // The limiting unit is the pixels
  dtime = pimup->update_end_timecode - pimup->update_begin_timecode;
  totalpixels = dtime * (double)width / viewtime;
  //printf("processing %f seconds %d pixels %f pixels begins at %f ends at %f\n", dtime, totalpixels, dtime * (double)width / viewtime, pimup->update_begin_timecode, pimup->update_end_timecode);
  if (totalpixels > width)
    totalpixels = width;
  pixels_per_second = (double)width / (double)viewtime;
  //if (totalpixels == width) // Hides a problem of rounding
    {
      // Starts at 0
      chunkxstart = width - ((pimup->screen_end_timecode - pimup->update_begin_timecode ) * pixels_per_second);
    }
  //else
  //  {
      // Will end exactly on width if needed
  //    chunkxstart = width - floor((pimup->screen_end_timecode - pimup->update_begin_timecode) * pixels_per_second);
  //  }
  //printf("chunkstart is %d tdiff=%f\n", chunkxstart, pimup->screen_end_timecode - pimup->update_begin_timecode);
  step = 0;
  processed_pixels_duration = (double)m_processed_pixels * viewtime / (double)width;
  steps = floor(dtime / processed_pixels_duration);
  // time per pixel, aliasing problem 
  for (step = 0; step < steps; step++)
    {
      ch.pixel_xstart = chunkxstart + step * m_processed_pixels;
      ch.pixel_xstop  = ch.pixel_xstart + m_processed_pixels;
      ch.tcode_start = pimup->update_begin_timecode + (double)step * processed_pixels_duration;
      ch.tcode_start = ch.tcode_start < 0? 0 : ch.tcode_start;
      ch.tcode_stop  = ch.tcode_start + processed_pixels_duration;
      ch.tcode_stop  = ch.tcode_stop > width? width : ch.tcode_stop;
      ch.sample_start = ch.tcode_start * (double)m_samplerate;
      ch.sample_stop  = ch.tcode_stop  * (double)m_samplerate;
      //printf("adding a dft chunk\n");
      m_dftclist.push_back(ch);
      totalpixels -= m_processed_pixels;
    }
  // Add the last chunk of pixels
  if (totalpixels > 0)
    {
      ch.pixel_xstart = chunkxstart + steps * m_processed_pixels;
      //ch.pixel_xstop  = ch.pixel_xstart + totalpixels;
      ch.pixel_xstop  = ch.pixel_xstart + totalpixels + 1; // Sometimes the end of the segment is lost for rounding problems, add one pixel and check if it does not overflow
      ch.pixel_xstop  = ch.pixel_xstop > width? width : ch.pixel_xstop;
      ch.tcode_start = pimup->update_begin_timecode + (double)step * processed_pixels_duration;
      ch.tcode_start = ch.tcode_start < 0? 0 : ch.tcode_start;
      ch.tcode_stop  = ch.tcode_start + (double)totalpixels * viewtime / (double)width;
      ch.tcode_stop  = ch.tcode_stop > width? width : ch.tcode_stop;
      ch.sample_start = ch.tcode_start * (double)m_samplerate;
      ch.sample_stop  = ch.tcode_stop  * (double)m_samplerate;
      //printf("adding the last dft chunk of %d pixels\n", ch.pixel_xstart - ch.pixel_xstop);
      m_dftclist.push_back(ch);
    }
}

void  Cdftspliter::draw_out_of_track_chunks(t_shared *pshared_data, int updatewidth, int cut, int xstart, int width, int height, bool view_calibrated)
{
  int         color;
  const int   coffset = 2;
  const int   cstripwidth = 64;
  int         i, j, decal;
  int         dest;

  //color = view_calibrated? 0xFFDEAD00 : 0xFF00DEAD;
  color = view_calibrated? 0xFF3E2D00 : 0xFF052114;
  if (xstart == 0)
    {
      decal = 2 * cstripwidth - (updatewidth % (2 * cstripwidth));
    }
  else
    decal = 0;
  //printf("cut == %d xstart == %d\n", cut, xstart);
  cut += xstart;
  ILOCK;
  for (j = 0; j < height; j++)
    {
      for (i = 0; i < updatewidth; i++)
	{
	  dest = j * width + ((cut + i) % width);
	  assert(dest < width * height);
	  assert(dest >= 0);
	  pshared_data->poutimg[dest] = (((decal + i) / cstripwidth) & 1)? color : 0xFF000000;
	}
      decal += coffset;
    }
  IUNLOCK;
}

void  Cdftspliter::draw_next_chunks(t_shared *pshared_data, double last_time, double imgtcode, double T, int width, int height, eviewtype viewtype)
{
  t_dft_chunk       ch;
  t_update_segment  updt_segment;
  Caudiodata       *pad;
  int               updatewidth;
  unsigned long     processingtime;

  //------------------------------------------------------------------------
  //
  // The steps go from left to right of the spectrogram with the steps
  // adjusted to limit the kernel time below 20ms.
  //
  //------------------------------------------------------------------------
//#define SHOW_DFT_CHUNKS
#ifdef SHOW_DFT_CHUNKS
  static int frame = 0;
  unsigned int scolors[] = {0xFFFF0000, 0xFF00FFFF};
  unsigned int scolorsAlter[] = {0xFFFF00D4, 0xFFEFEFEF};
  int step = 0;
#endif
  //#define SHOW_TIME
#ifdef SHOW_TIME
  double timespent = 0;
  double t1 = SDL_GetTicks();
#endif

  // Reorder the list
  m_dftclist.sort(Cdftspliter::compare);
  m_dftciter = m_dftclist.begin();
  while (m_dftciter != m_dftclist.end())
    {
      ch = *m_dftciter;
      m_dftciter++;
      assert(ch.pixel_xstop <= width);
      //printf("dft chunk %s: start %d stop %d\n", ch.type == chunk_out_of_track? "out" : "in", ch.pixel_xstart, ch.pixel_xstop);
      if (ch.pixel_xstart < 0)
	{
	  printf("inf 0 %d\n", ch.pixel_xstart);
	  if (ch.type == chunk_out_of_track)
	    printf("out of track\n");
	  else
	    printf("in track\n");
	  exit(EXIT_FAILURE);
 	}
      assert(ch.pixel_xstart >= 0);
      updatewidth = ch.pixel_xstop - ch.pixel_xstart;
      if (ch.type == chunk_out_of_track)
	{
	  //printf("drawing the out of track chunk ---------------------------------\n");
	  draw_out_of_track_chunks(pshared_data, updatewidth, m_circularindex, ch.pixel_xstart, width, height,(viewtype == view_calibrated));
	}
      else
	{
	  //printf("drawing an on track chunk --------------------------------------\n");
	  //------------------------------------------------------------------------
	  // Get the sound data for the current timecode
	  //------------------------------------------------------------------------
	  //printf("requesting samples from %f to %f\n", ch.tcode_start, ch.tcode_stop);
	  LOCK;
	  pad = pshared_data->pad;
	  get_needed_samples(pad, &ch, T);
	  UNLOCK;
	  //------------------------------------------------------------------------
	  // Calculate the spectrum chunk
	  //------------------------------------------------------------------------
	  m_pdft->opencl_dft(m_psamples, ch.totalsamples, ch.sample_start, ch.sample_stop, updatewidth, ch.max_freq, &processingtime);
	  if (processingtime > MAX_KERNEL_TIME && m_processed_pixels > DFTBANDLIMIT)
	    {
#ifdef _DEBUG
	      printf("processed pixels from %d", m_processed_pixels);
#endif
	      m_processed_pixels /= 2;
	      m_processed_pixels = m_processed_pixels < DFTBANDLIMIT? DFTBANDLIMIT : m_processed_pixels;
#ifdef _DEBUG
	      printf(" down to %d\n", m_processed_pixels);
#endif
	    }
	  ILOCK;
	  // 40 ms
	  m_pdft->dft_to_img((unsigned int*)pshared_data->poutimg, updatewidth, (m_circularindex + ch.pixel_xstart) % width, (viewtype == view_calibrated));
	  IUNLOCK;
#ifdef SHOW_TIME
	  timespent += processingtime;
#endif
	}
#ifdef SHOW_DFT_CHUNKS
      ILOCK;
      for (int g = 0; g < updatewidth; g++)
	{
	  int *ppxel = (int*)pshared_data->poutimg;
	  int index = ((m_circularindex + ch.pixel_xstart + g) % width);

	  if (ch.bforward)
	    {
	      ppxel[(2 * step)     * width + index] = (step & 1) == 0? scolorsAlter[frame & 1] :  scolorsAlter[frame & 1];
	      ppxel[(2 * step + 1) * width + index] = (step & 1) == 0? scolorsAlter[frame & 1] :  scolorsAlter[frame & 1];
	    }
	  else
	    {
	      ppxel[(2 * step)     * width + index] = (step & 1) == 0? scolors[frame & 1] :  scolors[frame & 1];
	      ppxel[(2 * step + 1) * width + index] = (step & 1) == 0? scolors[frame & 1] :  scolors[frame & 1];
	    }
	}
      IUNLOCK;
      step++;
#endif
    }
  if (m_dftciter == m_dftclist.end())
    {
#ifdef SHOW_TIME
      printf("total kernel execution time = %dms\n", (int)(timespent / 1000));
      double t2 = SDL_GetTicks();
      printf("total kernel SDL ticks execution time = %dms\n", (int)(t2 - t1));
#endif
      LOCK;
      pshared_data->bspectre_img_updated = true;
      pshared_data->lastimgtimecode = imgtcode;
      //------------------------------------------------------------------------
      // Add the areas to update in texture memory
      //------------------------------------------------------------------------
      pshared_data->update_segment_list.clear();
      m_dftciter = m_dftclist.begin();
      while (m_dftciter != m_dftclist.end())
	{
	  ch = *m_dftciter;
	  m_dftciter++;
	  updt_segment.xstart = (m_circularindex + ch.pixel_xstart) % width;
	  updt_segment.xstop  = (m_circularindex + ch.pixel_xstop);
	  updt_segment.xstop  = (updt_segment.xstop > width)? updt_segment.xstop - width : updt_segment.xstop;
	  //printf("Adding update segment %d %d.\n", updt_segment.xstart, updt_segment.xstop);
	  if (updt_segment.xstart > updt_segment.xstop) // Overlaping the image buffer boundary, split the segment
	    {
	      updt_segment.xstop = width;
	      if (updt_segment.xstart !=  updt_segment.xstop)
		{
		  assert(updt_segment.xstart < updt_segment.xstop);
		  assert(updt_segment.xstart >= 0 &&  updt_segment.xstop <= width);
	          //printf("start=%d stop = %d from %d - %d\n", updt_segment.xstart, updt_segment.xstop, ch.pixel_xstart, ch.pixel_xstop);
		  pshared_data->update_segment_list.push_front(updt_segment);
		}
	      updt_segment.xstart = 0;
	      updt_segment.xstop  = (m_circularindex + ch.pixel_xstop) % width;
	      if (updt_segment.xstart !=  updt_segment.xstop)
		{
		  if (updt_segment.xstart > updt_segment.xstop)
		    {
		      printf("start=%d stop = %d from %d - %d\n", updt_segment.xstart, updt_segment.xstop, ch.pixel_xstart, ch.pixel_xstop);
		      exit(EXIT_FAILURE);
		    }
		  assert(updt_segment.xstart < updt_segment.xstop);
		  assert(updt_segment.xstart >= 0 &&  updt_segment.xstop <= width);
		  pshared_data->update_segment_list.push_front(updt_segment);
		}
	    }
	  else
	    {
	      if (updt_segment.xstart < updt_segment.xstop)
		pshared_data->update_segment_list.push_front(updt_segment);
	    }
	}
      pshared_data->circularcut = m_circularindex;
      //------------------------------------------------------------
      // Attack display is done here to spare a dft
      //------------------------------------------------------------
      m_pdft->get_attack(pshared_data->pattackdata, width, pshared_data->circularcut);
      UNLOCK;
      m_dftclist.clear();
    }
#ifdef SHOW_DFT_CHUNKS
  frame++;
#endif
}

// Clamps the timecode to a pixel timecode
void Cdftspliter::remove_sample_to_pixel_aliasing(double &end_timecode, double viewtime, int width)
{
  double seconds_per_pixel;
  double diff;

  seconds_per_pixel = viewtime / (double)width;
  //printf("samples per pixel=%f seconds per pixel=%f.\n", samples_per_pixel, seconds_per_pixel);
  diff = end_timecode / seconds_per_pixel;
  diff -= floor(diff);
  diff = diff * seconds_per_pixel;
  end_timecode -= diff;
  //printf("diff=%f.\n", diff);
}

bool Cdftspliter::update_spectrometer_image(t_shared *pshared_data, double last_time)
{
  t_dft_image_update imup;
  double    elapsed_pixel_dec;
  double    pixels_per_second;
  double    diff;
  float     max_frequ;
  double    viewtime;
  double    lastimgtimecode;
  float     T;
  //int       samplerate;
  int       width;
  int       height;
  eviewtype viewtype;
  bool      bupdate;

  // Get timing info only here in order to be able to free the mutex during the opencl kernel execution
  LOCK;
  viewtime                 = pshared_data->viewtime;
  imup.screen_end_timecode = pshared_data->timecode;   // Timecode of the end of the screen
  imup.tracklen            = pshared_data->trackend;   // Recorder audio data length
  max_frequ                = pshared_data->fmax;
  width                    = pshared_data->dft_w;
  height                   = pshared_data->dft_h;
  viewtype                 = pshared_data->blogdb? view_calibrated : view_enhanced;
  bupdate                  = pshared_data->bspectre_img_updated;
  lastimgtimecode          = pshared_data->lastimgtimecode;
  //samplerate               = pshared_data->samplerate;
  UNLOCK;

  // If still waiting to copy the previous image on screen, then return
  if (bupdate)
    return false;

  //------------------------------------------------------------------------
  //
  // Clamps end_timecode to an exact pixel
  // 
  //------------------------------------------------------------------------
  remove_sample_to_pixel_aliasing(imup.screen_end_timecode, viewtime, width);

  //------------------------------------------------------------------------
  // Usefull values
  //------------------------------------------------------------------------
  imup.screen_begin_timecode = imup.screen_end_timecode - viewtime;
  pixels_per_second = (double)width / (double)viewtime;
  T = m_pdft->get_analysis_interval_T(max_frequ);

  //------------------------------------------------------------------------
  //
  // Check if something must be updated
  //
  //------------------------------------------------------------------------
  //
  if (imup.screen_begin_timecode > m_previous_end_timecode ||
      imup.screen_end_timecode < (m_previous_end_timecode - m_previous_viewtime) ||
      viewtime  != m_previous_viewtime ||
      max_frequ != m_previous_max_frequ ||
      viewtype  != m_previouss_view_type)
    {
      //printf("updating all\n");
      // Update all
      m_circularindex = 0; // Complete dft picture calculated
      imup.update_begin_timecode = imup.screen_begin_timecode;
      imup.update_end_timecode   = imup.screen_end_timecode;
      cut_dft_to_chunks(last_time, &imup, viewtime, width, max_frequ, true);
      m_previous_end_timecode = imup.screen_end_timecode;
      m_previous_viewtime     = viewtime;
      m_previous_max_frequ    = max_frequ;
      m_previouss_view_type   = viewtype;
      m_previous_missing_samples = 0;
    }
  else
    {
      // Moved forward
      if (imup.screen_end_timecode - m_previous_end_timecode > 1. / 50.)
	{
	  //printf("updating little right %f diff %f\n", imup.screen_end_timecode, imup.screen_end_timecode - m_previous_end_timecode);
	  //printf("T=%f timecode=%f\n", T, imup.screen_end_timecode);
	  // Check if it must be reupdated because of resolution if not enough samples are available because it is on the end of the track
	  // FIXME should be T but smears slightly under 2 x T. and close to the end of the track.
	  if (abs(imup.tracklen - imup.screen_end_timecode) <= (T * 2.) ||
	      abs(imup.tracklen - lastimgtimecode) <= (T * 2.))
	    {
	      imup.update_begin_timecode = m_previous_end_timecode - (T * 2.);
	      if (imup.update_begin_timecode < imup.screen_end_timecode - viewtime)
		imup.update_begin_timecode = imup.screen_end_timecode - viewtime;
	    }
	  else
	    imup.update_begin_timecode = m_previous_end_timecode;
	  if ((m_previous_missing_samples > 2. * T) && imup.update_begin_timecode > m_previous_missing_samples)
	    {
	      imup.update_begin_timecode = m_previous_missing_samples;
	    }
	  imup.update_end_timecode = imup.screen_end_timecode;
	  cut_dft_to_chunks(last_time, &imup, viewtime, width, max_frequ, true);
	  //
	  diff              = imup.screen_end_timecode - m_previous_end_timecode;
	  elapsed_pixel_dec = diff * pixels_per_second;
	  //printf("old circular index= %d\n", m_circularindex);
	  // BIG FIXME the float floor operaiton will fail here from time to time and convert 2.00 to 1.00
	  //m_circularindex = (m_circularindex + (int)(floor(elapsed_pixel_dec))) % width;
	  m_circularindex = (m_circularindex + (int)round(elapsed_pixel_dec)) % width;
	  //printf("elapsed pixels = %f\n", elapsed_pixel_dec);
	  //printf("new circular index= %d\n", m_circularindex);
	  m_previous_end_timecode = imup.screen_end_timecode;
	  //printf("updating %f pixels, updated time = %f tracklen=%f \n", elapsed_pixel_dec, imup.update_end_timecode - imup.update_begin_timecode, imup.tracklen);
	  if (imup.screen_end_timecode > imup.tracklen - 2. * T)
	    {
	      m_previous_missing_samples = imup.tracklen - 2. * T;
	    }
	  else
	    m_previous_missing_samples = 0;
	}
      // Moved backward
      if (imup.screen_end_timecode - m_previous_end_timecode < -1. / 50.)
	{
	  //printf("updating little left %f diff %f\n", imup.screen_end_timecode, imup.screen_end_timecode - m_previous_end_timecode);
	  imup.update_begin_timecode = imup.screen_end_timecode - viewtime;
	  imup.update_end_timecode   = imup.update_begin_timecode + (m_previous_end_timecode - imup.screen_end_timecode);
	  cut_dft_to_chunks(last_time, &imup, viewtime, width, max_frequ, false);
	  //
	  diff              = imup.screen_end_timecode - m_previous_end_timecode;
	  elapsed_pixel_dec = diff * pixels_per_second;
	  // FIXME same, floor will defecate and drop a 1.0 from 2., giving 1.
	  //m_circularindex = (m_circularindex + width + (int)(floor(elapsed_pixel_dec))) % width; // + width to avoid negavite values
	  m_circularindex = (m_circularindex + width + (int)round(elapsed_pixel_dec)) % width; // + width to avoid negavite values
	  assert(m_circularindex >= 0);
	  m_previous_end_timecode = imup.screen_end_timecode;
	  m_previous_missing_samples = 0;
	  //printf("updating %f pixels\n", elapsed_pixel_dec);
	}
    }
  bupdate = m_dftclist.size() > 0;
  if (bupdate)
    {
      //printf("updating, T=%f,  viwetime=%f\n", T, viewtime);
      draw_next_chunks(pshared_data, last_time, imup.screen_end_timecode, T, width, height, viewtype);
    }
  return bupdate;
}

