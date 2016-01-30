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
#include <assert.h>
#include <iterator>
#include <list>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/gl.h>

#include "audiodata.h"
#include "audioapi.h"
#include "shared.h"
#include "gfxareas.h"
#include "mesh.h"
#include "gl2Dprimitives.h"
#include "score.h"
#include "f2n.h"
#include "pictures.h"
#include "keypress.h"
#include "instrumenthand.h"
#include "scoreplacement.h"
#include "scoreplacement_violin.h"
#include "scoreplacement_piano.h"
#include "scoreplacement_guitar.h"
#include "scoreedit.h"
#include "scorerenderer.h"
#include "messages.h"
#include "messages_network.h"
#include "card.h"
#include "app.h"

void Cappdata::zoom_frequency(int v)
{
  t_shared   *pshared_data = (t_shared*)&m_shared_data;

  LOCK;
  if (m_shared_data.fmax < 1600) // Zoom less if low frequencies
    m_shared_data.zoom_f += -v * 0.002;
  else
    m_shared_data.zoom_f += -v * 0.01;
  if (m_shared_data.zoom_f > 1)
    m_shared_data.zoom_f = 1;
  if (m_shared_data.zoom_f < 0.0025)
    m_shared_data.zoom_f = 0.0025;
  m_shared_data.fmax = F_MAX * m_shared_data.zoom_f;
  m_localfbase = m_shared_data.fbase;
  m_localfmax = m_shared_data.fmax;
  UNLOCK;
  //printf("Zoomf %f, %f\n", m_shared_data.zoom_f, m_shared_data.fmax);
}

void Cappdata::zoom_viewtime(int v)
{
  t_shared     *pshared_data = (t_shared*)&m_shared_data;
  double        tracklen;
  static double last_sent = -1;
  const double  cdelay = 0.1;   // Do not send messages at a rate faster

  LOCK;
  m_shared_data.zoom_t += -v * 0.025;
  if (m_shared_data.zoom_t > 1)
    m_shared_data.zoom_t = 1;
  if (m_shared_data.zoom_t < 0.05)
    m_shared_data.zoom_t = 0.05;
//#define SIMPLE_ZOOM
#ifdef SIMPLE_ZOOM
  m_shared_data.viewtime = MAX_VIEW_TIME * m_shared_data.zoom_t;
#else
  double     newtime, diff;

  newtime = MAX_VIEW_TIME * m_shared_data.zoom_t;
  diff = newtime - m_shared_data.viewtime;
  m_shared_data.viewtime += diff;
  m_shared_data.timecode += diff / 2.;
// FIXME float error here
  //tracklen = (double)pshared_data->pad->get_samplenum() / (double)pshared_data->samplerate;
  tracklen = pshared_data->trackend;
  if (m_shared_data.timecode > tracklen)
    {
      m_shared_data.timecode = tracklen;
    }
#endif
  UNLOCK
  //printf("Zoomt %f, %f\n", m_shared_data.zoom_t, m_shared_data.viewtime);
  // FIXME limit the number of updates per second
  if (fabs(m_last_time - last_sent) > cdelay)
    {
      update_practice_stop_time(false);
    }
  last_sent = m_last_time;
}

void Cappdata::stick_to_area(Cgfxarea *pw, int x, int y, int &cx, int &cy)
{
  t_coord     pos;
  t_coord     dim;

  pw->get_pos(&pos);
  pw->get_dim(&dim);
  cx = x < pos.x + dim.x? x : pos.x + dim.x - 1;
  cx = cx < pos.x? pos.x : cx;
  cy = y < pos.y + dim.y? y : pos.y + dim.y - 1;
  cy = cy < pos.y? pos.y : cy;
}

void Cappdata::handle_weel(int x, int y, int v)
{
  // If the weel is on an area handling the wheel then change the zoom factor
  Cgfxarea *pw;
  t_coord  p;

  if (m_appstate != statewait)
    {
      return;
    }
  p.x = x;
  p.y = y;
  pw = m_layout->get_first();
  while (pw != NULL)
    {
      if (strcmp(pw->m_name, "spectre") == 0)
	{
	  // Zoom in out
	  if (pw->is_in(p))
	    {	    
	      //printf("inspectre\n");
	      zoom_frequency(v);
	    }
	}
      if (strcmp(pw->m_name, "freqscale") == 0)
	{
	  // Zoom in out
	  if (pw->is_in(p))
	    {
	      //printf("infreqscale\n");
	      zoom_frequency(v);
	    }
	}
      if (strcmp(pw->m_name, "timescale") == 0 ||
	  strcmp(pw->m_name, "track") == 0)
	{
	  // Zoom in out
	  if (pw->is_in(p))
	    {
	      //printf("intimescale\n");
	      zoom_viewtime(v);
	    }
	}
      if (strcmp(pw->m_name, "score") == 0)
	{
	  if (pw->is_in(p))
	    {
	      if (m_pScoreEdit->mouse_wheel(m_pScorePlacement, p, v))
		update_practice_view();
	    }
	}
      pw = m_layout->get_next();
    }
}

void Cappdata::play_fifo_sound_selection(int fifo_index)
{
  t_shared   *pshared_data = (t_shared*)&m_shared_data;
  std::list<t_audioOutCmd>::iterator iter;
  int i;

  if (fifo_index >= REPLAY_FIFO_SIZE)
    return;
  SLOCK;
  iter = pshared_data->replay_cmd_fifo.begin();
  i = 0;
  while (iter != pshared_data->replay_cmd_fifo.end())
    {
      if (i == fifo_index)
	{
	  pshared_data->filter_cmd_list.push_front(*iter);
	}
      i++;
      iter++;
    }
  SUNLOCK;
  wakeup_bandpass_thread();
}

void Cappdata::key_on(int code)
{
  t_shared   *pshared_data = (t_shared*)&m_shared_data;
  int         fifo_index;

  m_kstates.key_on(code);
  // Space bar = play/pause
  if (code == ' ' && !(m_bdonotappendtoopen && m_opened_a_file))
    {
      play_record_enabled(!(pshared_data->play_State == state_record));
    }
  if (code == 'r' ||
      code == 't' ||
      code == 'y' ||
      code == 'u' ||
      code == 'i' ||
      code == 'o' ||
      code == 'p')
    {
      switch (code)
	{
	case 'r':
	  {
	    fifo_index = 0;
	  }
	  break;
	case 't':
	  {
	    fifo_index = 1;
	  }
	  break;
	case 'y':
	  {
	    fifo_index = 2;
	  }
	  break;
	case 'u':
	  {
	    fifo_index = 3;
	  }
	  break;
	case 'i':
	  {
	    fifo_index = 4;
	  }
	  break;
	case 'o':
	  {
	    fifo_index = 5;
	  }
	  break;
	case 'p':
	  {
	    fifo_index = 6;
	  }
	  break;
	default:
	  fifo_index = 0;
	  break;	  
	}
      play_fifo_sound_selection(fifo_index);
    }
  //printf("%c on\n", code);
}

void Cappdata::key_off(int code)
{
  m_kstates.key_off(code);
}

void Cappdata::mouseclickright(int x, int y)
{
  // If the weel is on an area handling the wheel then change the zoom factor
  Cgfxarea *pw;
  t_coord  p;

  if (m_appstate != statewait)
    {
      return;
    }
#ifdef __DEBUG
  printf("clickright\n");
#endif
  p.x = x;
  p.y = y;
  m_cstart = p;
  pw = m_layout->get_first();
  while (pw != NULL)
    {
      if (strcmp(pw->m_name, "freqscale") == 0 && pw->is_in(p))
	{
	  m_pianofrequdisplay = !m_pianofrequdisplay;
	}
      if ((strcmp(pw->m_name, "spectre") == 0 && pw->is_in(p)))
	{
	  if (m_kstates.is_pressed('w'))
	    {
	      // Play the note frequency slot and his harmonics
	      m_appstate = stateselectharmonics;
	    }
	  else
	    {
	      if (m_kstates.is_pressed('g'))
		{
		  SDL_ShowCursor(SDL_DISABLE);
		  m_appstate = statemovehand;
		}
	      else
		{
		  // Add note state
		  m_appstate = stateaddnotedirect;
#ifdef __DEBUG
		  printf("stateaddnotedirect\n");
#endif
		}
	    }
	}
      if ((strcmp(pw->m_name, "timescale") == 0 && pw->is_in(p)))
	{
	  SDL_ShowCursor(SDL_DISABLE);
	  m_appstate = statemovehand;
	}
      if ((strcmp(pw->m_name, "track") == 0 && pw->is_in(p)))
	{
	  //SDL_ShowCursor(SDL_DISABLE);
	  m_appstate = statemove;
	}
      if (strcmp(pw->m_name, "score") == 0 && pw->is_in(p))
	{
	  m_pScoreEdit->mouse_right_down(p);
	  SDL_ShowCursor(SDL_DISABLE);
	  m_appstate = statemovehand;
	}
      pw = m_layout->get_next();
    }
  card_mouseclickright(x, y);
}

void Cappdata::mouseclickleft(int x, int y)
{
  //t_shared *pshared_data = (t_shared*)&m_shared_data;
  Cgfxarea *pw;
  t_coord  p;

#ifdef __DEBUG
  printf("clickleft ");
#endif
  if (m_appstate == statewait) // If practicing, do not take any input except cards
    {
      p.x = x;
      p.y = y;
      pw = m_layout->get_first();
      while (pw != NULL)
	{
	  if (strcmp(pw->m_name, "spectre") == 0 && pw->is_in(p))
	    {
	      //printf("inspectre ");
	      switch (m_appstate)
		{
		case statewait:
		  /*
		  if (m_kstates.is_pressed('a'))
		    {
		      // Add note state
		      m_appstate = stateaddnote;
		      m_cstart.x = x;
		      m_cstart.y = y;
		      //printf("stateaddnote\n");
		    }
		  else
		  */
		    {
		      if (m_kstates.is_pressed('e'))
			{
			  // Create the measure
			  m_appstate = stateaddmeasure;
			  m_cstart.x = x;
			  m_cstart.y = y;
			  //printf("stateaddmeasure\n");
			}
		      else
			{
			  // Play an area
			  m_appstate = stateselectsound;
			  m_cstart.x = x;
			  m_cstart.y = y;
			  //printf("stateselectsound\n");
			}
		    }
		default:
		  break;
		}
	    }
	  if (strcmp(pw->m_name, "track") == 0 && pw->is_in(p))
	    {
	      m_appstate = stateselectsoundontrack;
	      m_cstart.x = x;
	      m_cstart.y = y;
	    }
	  if (strcmp(pw->m_name, "score") == 0 && pw->is_in(p))
	    {
	      m_pScoreEdit->mouse_left_down(m_pscore, m_pcurrent_instrument, m_pScorePlacement, p);
	      m_appstate = stateeditscore;
	    }
	  if (strcmp(pw->m_name, "instrument") == 0 && pw->is_in(p))
	    {
	      m_appstate = stateselectinstrument;
	    }
	  pw = m_layout->get_next();
	}
    }
#ifdef __DEBUG
  printf("\n");
#endif
  // The cards at last
  card_mouseclickleft(x, y);
}

// Used to be able to do something if the mouse did not move
void Cappdata::mousedidnotmove(bool bmoved)
{
  static double  lasstillTimecode = -1;
  static t_coord playpos;
  Cgfxarea      *pw;
  t_coord        pos, dim;
  int            cx, cy;
  bool           bplaying;

  if (bmoved)
    playpos.x = playpos.y = 0; // State reset, avoids playing the same note in a loop
  if (lasstillTimecode == -1 || bmoved)
    {
      lasstillTimecode = m_last_time;
      return;
    }
  if (m_last_time - lasstillTimecode < 3 * REFLEXTIME)
    {
      return;
    }
  if (m_appstate == stateaddnotedirect || m_appstate == stateselectharmonics)
    {
      pw = m_layout->get_first();
      while (pw != NULL)
	{
	  if (strcmp(pw->m_name, "spectre") == 0 && pw->is_in(m_ccurrent))
	    {
	      // Play the current selected note, but only if nothing is playing
	      bplaying = queued_filtered_band();
	      if (!bplaying && playpos.x != m_ccurrent.x && playpos.y != m_ccurrent.y)
		{
		  playpos = m_ccurrent;
		  stick_to_area(pw, m_ccurrent.x, m_ccurrent.y, cx, cy);
		  pw->get_pos(&pos);
		  pw->get_dim(&dim);
		  // Filter and play the sound in the box
		  create_filter_play_harmonics_message(cx, cy, pos, dim); 
		}
	      else
		lasstillTimecode = m_last_time;
	    }
	  pw = m_layout->get_next();
	}
    }
}

void Cappdata::mousemove(int x, int y)
{
  t_shared   *pshared_data = (t_shared*)&m_shared_data;
  Cgfxarea   *pw;
  t_coord    p;
  double     t1, t2;
  int        note_id;

  //printf("mousemove\n");
  p.x = x;
  p.y = y;
  pw = m_layout->get_first();
  while (pw != NULL)
    {
      if (strcmp(pw->m_name, "spectre") == 0 && pw->is_in(p))
	{
	  switch (m_appstate)
	    {
	      //case stateaddnote:
	    case stateaddnotedirect:
	    case stateselectharmonics:
	      {
		double  timecode;
		double  duration;
		float   f;
		t_coord pos;
		t_coord dim;
		
		m_cstart.y = y;
		pw = m_layout->get_area((char*)"spectre");
		if (pw != NULL)
		  {
		    pw->get_pos(&pos);
		    pw->get_dim(&dim);
		    LOCK;
		    t1 = getSpectrum_time_fromX(m_cstart.x, pos, dim, pshared_data->timecode, pshared_data->viewtime);
		    t2 = getSpectrum_time_fromX(x, pos, dim, pshared_data->timecode, pshared_data->viewtime);
		    timecode = t1 < t2? t1 : t2;
		    duration = fabs(t1 - t2);
		    UNLOCK;
		    f = getSpectrum_freq_fromY(y, pos, dim, m_localfbase, m_localfmax);
		    m_pScorePlacement->change_tmp_note(timecode, duration, f);
		  }
	      }
	      break;
	    //case statemove:
	    case statemovehand:
	      stick_to_area(pw, x, y, p.x, p.y);
	      move_trackpos_to(p.x, pw, true);
	      break;
	    default:
	      break;
	    };
	}
      if (strcmp(pw->m_name, "timescale") == 0 && pw->is_in(p))
	{
	  if (m_appstate == statemovehand)
	    {
	      stick_to_area(pw, x, y, p.x, p.y);
	      move_trackpos_to(p.x, pw, true);
	    }
	}
      if (strcmp(pw->m_name, "track") == 0 && pw->is_in(p))
	{
	  if (m_appstate == statemove)
	    {
	      stick_to_area(pw, x, y, p.x, p.y);
	      move_trackpos_to(p.x, pw, false);
	    }
	}
      if (strcmp(pw->m_name, "score") == 0 && pw->is_in(p))
	{
	  if (m_appstate == statemovehand)
	    {
	      stick_to_area(pw, x, y, p.x, p.y);
	      move_trackpos_to(p.x, pw, true);
	    }
	}
      pw = m_layout->get_next();
    }
  //
  if (m_appstate == stateeditscore ||
      m_layout->is_in(x, y, (char*)"score"))
    {
      pw = m_layout->get_area((char*)"score");
      if (pw != NULL)
	{
	  stick_to_area(pw, x, y, p.x, p.y);
	  if (m_pScoreEdit->mouse_move(m_pscore, m_pScorePlacement, p))
	    {
	      update_practice_view();
	    }
	  if (m_pScoreEdit->on_note_change(&note_id))
	    {
	      send_note_highlight(note_id);
	    }
	}
    }
  //
  card_mousemove(x, y);
  m_ccurrent.x = x;
  m_ccurrent.y = y;
}

void Cappdata::move_trackpos_to(int mousex, Cgfxarea *pw, bool buseviewtime)
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;
  t_coord  pos;
  t_coord  dim;
  double   tref;
  bool     bdisabled = false;
  static double last_sent = -1;
  const double  cdelay = 0.1;   // Do not send messages at a rate faster

  // Change visualisation time
  pw->get_pos(&pos);
  pw->get_dim(&dim);
  LOCK;
  if (pshared_data->play_State == state_record)
    {
      set_record_card(bdisabled);
      pshared_data->play_State = state_stop;
    }
  if (buseviewtime)
    {
      if (mousex != m_cstart.x)
	{
	  tref = (double)(m_cstart.x - mousex) * pshared_data->viewtime / (double)dim.x;
	 if ((pshared_data->timecode + tref < pshared_data->trackend + pshared_data->viewtime / 2.) &&
	     (pshared_data->timecode + tref > pshared_data->viewtime / 2.))
	  {
	    m_cstart.x = mousex;
	    pshared_data->timecode += tref;
	  }
	}
    }
  else
    {
      tref = (double)(mousex - pos.x) * pshared_data->trackend / (double)dim.x;
      //printf("tracklen = %f moving from %f to", tracklen, pshared_data->timecode);
      pshared_data->timecode = tref + (pshared_data->viewtime / 2.);
    }
  //printf(" %f\n", pshared_data->timecode);
  UNLOCK;
  // FIXME limit the number of updates per second
  if (fabs(m_last_time - last_sent) > cdelay)
    {
      update_practice_stop_time(false);
    }
  last_sent = m_last_time;
}

void Cappdata::mouseupright(int x, int y)
{
  Cgfxarea   *pw;
  t_coord     p;
  t_coord     pos, dim;
  int         cx, cy;

  p.x = x;
  p.y = y;
  switch (m_appstate)
    {
    case statewait:
      {
	m_pScoreEdit->mouse_right_up(p);
      }
      break;
    case statemovehand:
      {
	m_pScoreEdit->mouse_right_up(p);
	SDL_ShowCursor(SDL_ENABLE);
	m_appstate = statewait;
      }
      break;
    case statemove:
      {
	pw = m_layout->get_area((char*)"track");
	assert(pw != NULL);
	move_trackpos_to(p.x, pw, false);
	//SDL_ShowCursor(SDL_ENABLE);
	m_appstate = statewait;
      }
      break;
    case stateselectharmonics:
      {
	if (m_kstates.is_pressed('w'))
	  {
	    // ----------------------------------------
	    // Filter and play the sound in the box
	    // If a selection state is already activated,
	    // do not care if inside spectre.
	    pw = m_layout->get_area((char*)"spectre");
	    assert(pw != NULL);
	    stick_to_area(pw, x, y, cx, cy);
	    pw->get_pos(&pos);
	    pw->get_dim(&dim);
	    create_filter_play_harmonics_message(cx, cy, pos, dim);
	  }
	// Cleat the tmp note
	m_pScorePlacement->set_tmp_note(&m_empty_note);
	m_appstate = statewait;
      }
      break;
    case stateaddnotedirect:
      {
	// Clear the tmp note
	m_pScorePlacement->set_tmp_note(&m_empty_note);
	// ----------------------------------------
	// If a selection state is already activated,
	// do not care if inside spectre.
	pw = m_layout->get_area((char*)"spectre");
	assert(pw != NULL);
	stick_to_area(pw, x, y, cx, cy);
	pw->get_pos(&pos);
	pw->get_dim(&dim);
	add_note(cx, cy, pos, dim);
	m_appstate = statewait;
      }
      break;
    default:
      card_mouseupright(x, y);
      break;
    }
}

float Cappdata::getSpectrum_freq_fromY(int y, t_coord pos, t_coord dim, float fbase, float fmax)
{
  float f;

  f = fbase + ((float)(pos.y + dim.y - y) * (fmax - fbase) / dim.y);
  return f;
}

double Cappdata::getSpectrum_time_fromX(int x, t_coord pos, t_coord dim, double timecode, double viewtime)
{
  double time;

  time = timecode - ((float)abs(pos.x + dim.x - x) * viewtime / (float)dim.x);
  //printf("viewtime %f timecode %f note back of %fs\n", viewtime, timecode, ((float)abs(pos.x + dim.x - x) * viewtime / (float)dim.x));
  return time;
}

int Cappdata::getSpectrumY_from_Frequ(float f, t_coord pos, t_coord dim, float fbase, float fmax)
{
  int y;

  y = pos.y + dim.y - floor((f - fbase) * (float)dim.y / (fmax - fbase));
  y = (y < pos.y)? pos.y : y;
  y = (y > pos.y + dim.y)? pos.y + dim.y - 1 : y;
  return y;
}

int Cappdata::getSpectrumX_from_time(double time, t_coord pos, t_coord dim, double timecode, double viewtime)
{
  int x;

  if (time > timecode)
    return pos.x + dim.x - 1;
  x = pos.x + dim.x - fabs(timecode - time) * (float)dim.x / viewtime;
  x = (x < pos.x)? pos.x : x;
  x = (x >= pos.x + dim.x)? pos.x + dim.x - 1 : x;
  return x;
}

void Cappdata::setlooptime(double time)
{
  m_last_time = time;
}

void Cappdata::note_from_spectrumy(t_coord pos, t_coord dim, int y, int *pnote, int *poctave, float fbase, float fmax)
{
  float f;
  float frequ_err;

  f = getSpectrum_freq_fromY(y, pos, dim, fbase, fmax);
  m_f2n.convert(f, poctave, pnote, &frequ_err);
}

void Cappdata::set_cmd_start_stop(t_audioOutCmd &cmd, t_coord pos, t_coord dim, int xstart, int x)
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;
  double   timecode;
  double   duration;
  int      xp;

  timecode = pshared_data->timecode;
  duration = pshared_data->viewtime;
  xp = xstart < x? xstart : x;
  cmd.start = getSpectrum_time_fromX(xp, pos, dim, timecode, duration);
  xp = xstart > x? xstart : x;
  cmd.stop = getSpectrum_time_fromX(xp, pos, dim, timecode, duration);
}

void Cappdata::add_sound_selection_to_fifo(t_audioOutCmd *pcmd)
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;

  if (pshared_data->replay_cmd_fifo.size() > REPLAY_FIFO_SIZE)
    {
      pshared_data->replay_cmd_fifo.pop_back();
    }
  pshared_data->replay_cmd_fifo.push_front(*pcmd);
}

void Cappdata::create_filter_play_message(t_coord boxstart, int x, int y, t_coord pos, t_coord dim)
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;
  float    fbase;
  float    fmax;
  int      yp;
  t_audioOutCmd cmd;
  std::list<t_audioOutCmd> *plist = &pshared_data->filter_cmd_list;

  LOCK;
  // Start and stop time
  set_cmd_start_stop(cmd, pos, dim, boxstart.x, x);
  fbase = pshared_data->fbase;
  fmax = pshared_data->fmax;
  UNLOCK;
  cmd.playdelay =  0.;
  cmd.playstart = -1.;
  cmd.bands = 1;
  cmd.bsubstract = false;
  cmd.bloop_data = false;
  cmd.notestate = state_wait_filtering;
  cmd.pnote = NULL;
  if (fabs(cmd.stop - cmd.start) > 0.005)
    {
      yp = boxstart.y < y? boxstart.y : y;
      cmd.fhicut[0] = getSpectrum_freq_fromY(yp, pos, dim, fbase, fmax);
      yp = boxstart.y > y? boxstart.y : y;
      cmd.flocut[0] = getSpectrum_freq_fromY(yp, pos, dim, fbase, fmax);
      if (fabs(cmd.fhicut[0] - cmd.flocut[0]) > 1.) // Hz
	{
	  SLOCK;
	  plist->push_front(cmd);
	  add_sound_selection_to_fifo(&cmd);
	  SUNLOCK;
	}
    }
  wakeup_bandpass_thread();
}

void Cappdata::create_filter_play_message_from_track(int xstart, int xstop, t_coord pos, t_coord dim)
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;
  float    fbase;
  float    fmax;
  double   t1, t2;
  double   duration;
  int      xp;
  t_audioOutCmd cmd;
  std::list<t_audioOutCmd> *plist = &pshared_data->filter_cmd_list;

  flush_filtered_strips();
  delete_filter_commands(&m_note_selection.cmdlist, true);
  LOCK;
  // Start and stop time
  duration = pshared_data->trackend;
  UNLOCK;
  xp = xstart < xstop? xstart : xstop;
  t1 = (double)(xp - pos.x) * duration / (double)dim.x;
  xp = xstart > xstop? xstart : xstop;
  t2 = (double)(xp - pos.x) * duration / (double)dim.x;
  // All the frequency range
  fbase = MINNOTEF;
  fmax = 44000;
  t2 = t2 - t1 > MAX_INTRACK_PLAY_TIME? t1 + MAX_INTRACK_PLAY_TIME : t2;
  cmd.start = t1;
  cmd.stop = t2;
  cmd.playdelay =  0.;
  cmd.playstart = -1.;
  cmd.bsubstract = false;
  cmd.notestate = state_wait_filtering;
  cmd.bands = 1;
  cmd.pnote = NULL;
  if (fabs(cmd.stop - cmd.start) > 0.005)
    {
      cmd.fhicut[0] = fmax;
      cmd.flocut[0] = fbase;
      if (fabs(cmd.fhicut[0] - cmd.flocut[0]) > 1.) // Hz
	{
	  SLOCK;
	  plist->push_front(cmd);
	  SUNLOCK;
	}
    }
  wakeup_bandpass_thread();
}

bool Cappdata::set_cmd_bands(t_audioOutCmd &cmd, float basef, float fbase, float fmax)
{
  int   j;
  int   octave;
  int   note;
  float ferr;
  float notefh;
  float notefl;
  bool  bnullband;

  bnullband = false;
  for (j = 1; j <= HARMONICS && ((float)j * basef) < fmax; j++)
    {
      // Note limit as y coordinates
      m_f2n.convert((float)j * basef, &octave, &note, &ferr);
      if (note == -1)
	break;
      m_f2n.note_frequ_boundary(&notefl, &notefh, note, octave);
      //printf("j == %d adding %f %f\n.", j, notefl, notefh);
      cmd.fhicut[j - 1] = notefh;
      cmd.flocut[j - 1] = notefl;
      if (fabs(notefh - notefl) <= 1.)
	{
	  bnullband = true;
	}
    }
  cmd.bands = j - 1;
  return (!bnullband);
}

// Plays second to 16th harmonics of a note
void Cappdata::create_filter_play_harmonics_message(int x, int y, t_coord pos, t_coord dim)
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;
  int      octave;
  int      note;
  t_coord  boxstart;
  float    fbase;
  float    fmax;
  float    basef;
  t_audioOutCmd cmd;
  std::list<t_audioOutCmd> *plist = &pshared_data->filter_cmd_list;

  boxstart = m_cstart;
  LOCK;
  // Max and min frequency
  fbase = pshared_data->fbase;
  fmax = pshared_data->fmax;
  // Find the note at y
  note_from_spectrumy(pos, dim, y, &note, &octave, fbase, fmax);
  if (note == -1)
      return ;
  // Find the limits
  basef = m_f2n.note2frequ(note, octave);
  // Start and stop time
  set_cmd_start_stop(cmd, pos, dim, boxstart.x, x);
  UNLOCK;
  cmd.playdelay =  0.;
  cmd.playstart = -1.;
  cmd.bsubstract = false;
  cmd.bloop_data = false;
  cmd.notestate = state_wait_filtering;
  cmd.pnote = NULL;
  if (fabs(cmd.stop - cmd.start) > 0.005)
    {
      if (set_cmd_bands(cmd, basef, fbase, fmax))
	{
	  SLOCK;
	  plist->push_front(cmd);
	  SUNLOCK;
	}
    }
  wakeup_bandpass_thread();
}

void Cappdata::set_score_renderer(string instrument_name)
{
  remove_note_selection(&m_note_selection);
  m_pScorePlacement = get_instrumentPlacement(instrument_name);
  m_pScoreEdit = get_instrumentEdit(instrument_name);
  m_pScorePlacement->set_autobeam(m_card_states.autobeam);
  m_pScoreEdit->set_chord_fuse(m_card_states.chordfuse);
}

void Cappdata::select_instrument(int mouse_x, int mouse_y)
{
  CInstrument *pins;
  int          i, y, tabsize;
  Cgfxarea    *pw;
  t_coord      pos, dim;

  pw = m_layout->get_area((char*)"instrument");
  if (pw != NULL)
    {
      pw->get_pos(&pos);
      pw->get_dim(&dim);
      tabsize = instrument_tab_height(pw);
      pins = m_pscore->get_first_instrument();
      i = 0;
      while (pins != NULL)
	{
	  y = pos.y + i * tabsize;
	  if (mouse_y > y &&
	      mouse_y < y + tabsize &&
	      m_pcurrent_instrument != pins)
	    {
	      m_pcurrent_instrument = pins;
	      update_practice_stop_time(true);
	      set_score_renderer(m_pcurrent_instrument->get_name());
	    }
	  pins = m_pscore->get_next_instrument();
	  i++;
	}
    }
}

void Cappdata::add_note(int cx, int cy, t_coord pos, t_coord dim)
{
  t_shared    *pshared_data = (t_shared*)&m_shared_data;
  CInstrument *pins;
  CNote       *pn;
  double       t1, t2;
  double       timecode;
  double       duration;
  double       total_length;
  float        f;
  int          string_number;

  // Create a note of x length in seconds and y frequency
  pins = m_pcurrent_instrument;
  if (pins)
    {
      LOCK;
      t1 = getSpectrum_time_fromX(m_cstart.x, pos, dim, pshared_data->timecode, pshared_data->viewtime);
      t2 = getSpectrum_time_fromX(cx, pos, dim, pshared_data->timecode, pshared_data->viewtime);
      total_length = pshared_data->trackend;
      UNLOCK;
      if (t1 < 0 || t2 > total_length)
	return;
      timecode = t1 < t2? t1 : t2;
      duration = fabs(t1 - t2);
      //printf("duration %f from %f to %f\n", duration, timecode,  getSpectrum_time_fromX(cx, pos, dim, pshared_data->timecode, pshared_data->viewtime));
      f = getSpectrum_freq_fromY(cy, pos, dim, m_localfbase, m_localfmax);
      f = m_f2n.f2nearest_note(f);
      string_number = m_pScorePlacement->get_hand()->get_first_string(f);
      // On the non piano and violin instruments, notes outside of the instrument range are rejected
      // With this, people who have only the base version car do more.
      if (string_number == OUTOFRANGE &&
	  m_pcurrent_instrument->get_name() != string("violin") &&
	  m_pcurrent_instrument->get_name() != string("piano"))
	{
	  return;
	}
      pn = new CNote(timecode, duration, f, string_number);
      pins->add_note(pn);
      create_filter_play_harmonics_message(cx, cy, pos, dim);
      // Send to practice
      send_note_to_practice_view(pn, false);
#ifdef __DEBUG
      printf("addnote %d to %d, at %fs for %fs f=%fhz\n", m_cstart.x, cx, timecode, duration, f);
#endif
    }
}

void Cappdata::mouseupleft(int x, int y)
{
  t_shared    *pshared_data = (t_shared*)&m_shared_data;
  double      timecode;
  double      duration;
  t_coord     p;
  Cgfxarea    *pw;
  t_coord     pos;
  t_coord     dim;
  int         cx, cy;

#ifdef __DEBUG
  printf("upleft ");
#endif
  // x, y are global values if the mouse is outside the window
  // therefore use the last values from mous move
  x = m_ccurrent.x;
  y = m_ccurrent.y;
  // ----------------------------------------
  // If a selection state is already activated,
  // do not care if inside spectre. Get the state
  // related area dimensions.
  switch (m_appstate)
    {
    //case stateaddnote:
    case stateaddmeasure:
    case stateselectsound:
    case stateeditscore:
      {
	pw = m_layout->get_area((char*)"spectre");
	assert(pw != NULL);
	stick_to_area(pw, x, y, cx, cy);
	pw->get_pos(&pos);
	pw->get_dim(&dim);
      }
      break;
    case stateselectsoundontrack:
      {
	pw = m_layout->get_area((char*)"track");
	assert(pw != NULL);
	stick_to_area(pw, x, y, cx, cy);
	pw->get_pos(&pos);
	pw->get_dim(&dim);
      }
      break;
    case stateselectinstrument:
    default:
      break;
    }
  //
  m_pScorePlacement->set_tmp_note(&m_empty_note);
  //
  switch (m_appstate)
    {
    case stateaddnote:
      if (m_kstates.is_pressed('a'))
	{
	  add_note(cx, cy, pos, dim);
	}
      m_appstate = statewait;
      break;
    case stateaddmeasure:
      if (m_kstates.is_pressed('e'))
      {
	LOCK;
	timecode = getSpectrum_time_fromX(m_cstart.x, pos, dim, pshared_data->timecode, pshared_data->viewtime);
	duration = getSpectrum_time_fromX(cx, pos, dim, pshared_data->timecode, pshared_data->viewtime);
	//printf("time from x=%d to %d t1=%f t2=%f\n", cx, m_cstart.x, timecode, duration);
	UNLOCK;
	m_pscore->add_measure(timecode, duration);
      }
      m_appstate = statewait;
      break;
    case stateselectsound:
      {
	// Filter and play the sound in the box
	create_filter_play_message(m_cstart, cx, cy, pos, dim);
	m_appstate = statewait;
      }
      break;
    case stateselectsoundontrack:
      {
	// Filter and play the sound in the box
	create_filter_play_message_from_track(m_cstart.x, cx, pos, dim);
	m_appstate = statewait;
      }
      break;
    case stateeditscore:
      {
	p.x = x;
	p.y = y;
	if (m_pScoreEdit->mouse_left_up(m_pscore, m_pcurrent_instrument, m_pScorePlacement, p))
	  {
	    update_practice_view(); // Update only the changed notes in the practice dialog.
	  }
	play_edition_sel_notes(m_pScoreEdit->get_edit_state());
	m_appstate = statewait;
      }
      break;
    case stateselectinstrument:
      {
	if (m_layout->is_in(x, y, (char*)"instrument"))
	  select_instrument(x, y);
	m_appstate = statewait;
      }
      break;
    default:
      break; 
    };
  // ----------------------------------------
/*
  p.x = x;
  p.y = y;
  pw = m_layout->get_first();
  while (pw != NULL)
    {
      if (strcmp(pw->m_name, "spectre") == 0 && pw->is_in(p))
	{
	}
      if (strcmp(pw->m_name, "track") == 0 && pw->is_in(p))
	{
	  // Do nothing
	  m_appstate = statewait;
	}
      if (strcmp(pw->m_name, "score") == 0 && pw->is_in(p))
	{
	}
      pw = m_layout->get_next();
    }
*/
#ifdef __DEBUG
  printf("\n");
#endif
}

