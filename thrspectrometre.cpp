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
//#include "colorscale.h"
//#include "hann_window.h"
#include "spectrometre.h"
#include "opencldevice.h"
#include "opencldft.h"
#include "dftspliter.h"
#include "trackrender.h"

// Spectrometer rendering thread

int g_thrspretval;

void wait_draw_event(t_shared *pshared_data)
{
  pthread_mutex_lock(&pshared_data->cond_drawspectremutex);
  pthread_cond_wait(&pshared_data->cond_drawspectre, &pshared_data->cond_drawspectremutex);
  pthread_mutex_unlock(&pshared_data->cond_drawspectremutex);
}

void shift_left_img(unsigned int* pimg, int wx, int wy, int elapsedpixels)
{
  int i, j;
  unsigned int *pline;
  int end;

  end = wx - elapsedpixels;
  for (i = 0; i < wy; i++)
    {
      pline = &pimg[i * wx];
      for (j = elapsedpixels; j < end; j++)
	{
	  pline[j - elapsedpixels] = pline[j];
	}
    }
}

#define SKIP_VALUE 4

void* thr_spectrometre(void *p_data)
{
  t_shared   *pshared_data = (t_shared*)p_data;
  int   wx;
  int   wy;
  int   twx;
  int   twy;
  int   samplerate;
  int   samplepos;
  int   samplenum;
  int   samplewidth;
  float fbase, fmax;
  bool  bquit;
  bool  bimgupdate;
  bool  bopencl_not_initialised;
  Cdftspliter *pdftspliter;
  int   trackimgupdate;
  Ctrack_render *ptrackrenderer;
  std::list<t_audio_track_cmd>::iterator iter;
  double last_time;

  g_thrspretval = 0;
  LOCK;
  wx = pshared_data->dft_w;
  wy = pshared_data->dft_h;
  twx = pshared_data->trackimgw;
  twy = pshared_data->trackimgh;
  samplerate = pshared_data->samplerate; 
  fbase = pshared_data->fbase;
  fmax = pshared_data->fmax;
  UNLOCK;
  //-----------------------------------------------------------------------------
  // Atomic load
  //-----------------------------------------------------------------------------
  LOCK;
  printf("Initialising the opencl thread\n");
  //-----------------------------------------------------------------------------
  // Initialise the class
  //-----------------------------------------------------------------------------
  // Create the opencl classes
  Copenclspectrometer *pdft = new Copenclspectrometer(samplerate, wx, wy, fbase, fmax);
  bopencl_not_initialised = pdft->init_opencl_dft();
  pshared_data->bOpenClEnabled = true; // At least the worst is completed
  UNLOCK;
  if (bopencl_not_initialised)
    {
      g_thrspretval = 1;
    }
  else
    {
      pdft->calibration();
      pdftspliter = new Cdftspliter(pdft, wx, samplerate);
      trackimgupdate = 0;
      ptrackrenderer = new Ctrack_render(twx, twy);
      // Loop to redraw spectrometer chunks if needed
      LOCK;
      bquit = pshared_data->bquit;
      UNLOCK;
      while (!bquit)
	{
	  //---------------------------------------------------------------------------------------
	  // Do the math
	  //---------------------------------------------------------------------------------------
	  last_time = 0.001 * SDL_GetTicks();
	  bimgupdate = pdftspliter->update_spectrometer_image(pshared_data, last_time);

	  //------------------------------------------------------------
	  // Track display is done here to spare a thread
	  //------------------------------------------------------------
	  if (trackimgupdate > SKIP_VALUE || bimgupdate)
	    {
	      LOCK;
	      samplepos = pshared_data->timecode * samplerate;
	      samplenum = pshared_data->pad->get_samplenum();
	      // The current timecode can overflow, stay in the samples
	      samplepos = (samplepos > samplenum)? samplenum : samplepos;
	      samplewidth = pshared_data->viewtime * samplerate;
	      ptrackrenderer->add_new_data(pshared_data->pad);
	      UNLOCK;
	      if (samplenum)
		{
		  ILOCK;
		  ptrackrenderer->render_track_data(samplewidth, samplepos, pshared_data->ptrackimg, twx * twy);
		  IUNLOCK;
		}
	      LOCK;
	      pshared_data->btrack_img_updated = true;
	      UNLOCK;
	      trackimgupdate = 0;
	    }
	  else
	    {
	      trackimgupdate++;
	    }
	  //------------------------------------------------------------
	  // Process commands from the other threads
	  //------------------------------------------------------------
	  LOCK;
	  if (pshared_data->audio_cmds.size() > 0)
	    {
	      iter = pshared_data->audio_cmds.begin();
	      while (iter != pshared_data->audio_cmds.end())
		{
		  if ((*iter).direction == ad_app2threadspectr)
		    {
		      switch ((*iter).command)
			{
			case ac_reset:
			  {
			    samplepos = 0;
			    ptrackrenderer->reset();
			    samplewidth = pshared_data->viewtime * samplerate;
			    UNLOCK;
			    ILOCK;
			    ptrackrenderer->render_track_data(samplewidth, samplepos, pshared_data->ptrackimg, twx * twy);
			    IUNLOCK;
			    LOCK;
			    pshared_data->btrack_img_updated = true;
			    pdft->reset_attack(pshared_data->pattackdata, wx);
			  }
			  break;
			default:
			  break;
			}
		      iter = pshared_data->audio_cmds.erase(iter);
		    }
		  else
		    iter++;
		}
	    }
	  UNLOCK;
	  // Wait event
	  wait_draw_event(pshared_data);
	  LOCK;
	  bquit = pshared_data->bquit;
	  UNLOCK;
	}
      delete ptrackrenderer;
      delete pdftspliter;
    }
  delete pdft;
#ifdef _DEBUG
  printf("Spectrogram thread closing\n");
#endif
  return &g_thrspretval;
}

