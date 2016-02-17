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

#include <math.h>
#include <string.h>
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
#include "stringcolor.h"
#include "instrumenthand.h"
#include "scoreplacement.h"
#include "scoreplacement_violin.h"
#include "scoreplacement_piano.h"
#include "scoreplacement_guitar.h"
#include "scoreedit.h"
#include "scoreedit_piano.h"
#include "scorerenderer.h"
#include "card.h"
#include "messages.h"
#include "tcp_message_receiver.h"
#include "messages_network.h"
#include "app.h"
//#include "sdlcalls.h"

#define MAKE_COLOR_32(r, g, b, a) ((r << 16) | (g << 8) | b | (a << 24))

void Cappdata::draw_basic_rectangle(Cgfxarea *pw, int color)
{
  t_fcoord pos;
  t_fcoord dim;
  bool     bantialiased = true;

  pw->get_posf(&pos);
  pw->get_dimf(&dim);
  m_gfxprimitives->box(pos, dim, color, bantialiased);
}

void Cappdata::renderTextBlended(t_coord pos, int color, string text, bool blended)
{
  t_fcoord fpos;
  char     str[4096];

  blended = true;
  fpos.x = (float)pos.x;
  fpos.y = (float)pos.y;
  strcpy(str, text.c_str());
  m_gfxprimitives->print(str, m_font, fpos, color, blended);
}

void Cappdata::rendertext(t_coord pos, int color, char *text)
{
  renderTextBlended(pos, color, text, true);
}

void Cappdata::draw_h_line(t_coord pos, int length, int color)
{
  m_gfxprimitives->hline(pos.x, pos.y, length, color);
}

void Cappdata::draw_v_line(t_coord pos, int length, int color)
{
  m_gfxprimitives->vline(pos.x, pos.y, length, color);
}

void Cappdata::copy_spectrogram(Cgfxarea *pw)
{
  t_shared   *pshared_data = &m_shared_data;
  int         w, h;
  int         cut, xstart, xstop;
  std::list<t_update_segment>::iterator iter;

//  return ;
//#define COPY_ALL_SPECTRUMETER
#ifdef COPY_ALL_SPECTRUMETER
  LOCK;
  w = pshared_data->dft_w;
  h = pshared_data->dft_h;
  cut = pshared_data->circularcut;
  xstart = 0;
  xstop = pshared_data->dft_w;
  UNLOCK;
  ILOCK;
  m_gfxprimitives->update_spectrum_texture_chunk(xstart, xstop, w, h, pshared_data->poutimg);
  IUNLOCK;
  LOCK;
  pshared_data->bspectre_img_updated = false;
  UNLOCK;
#else
  LOCK;
  //if (pshared_data->bspectre_img_updated) ?????
  {
      w = pshared_data->dft_w;
      h = pshared_data->dft_h;
      cut = pshared_data->circularcut;
      //drw = pshared_data->update_segment_list.size() > 0;
      iter = pshared_data->update_segment_list.begin();
      while (iter != pshared_data->update_segment_list.end())
	{
	  xstart = (*iter).xstart;
	  xstop  = (*iter).xstop;
	  assert(xstart >= 0 && xstart <= w);
	  assert(xstop >= 0 && xstop <= w);
	  assert(xstart < xstop);
	  UNLOCK;
	  ILOCK;
	  m_gfxprimitives->update_spectrum_texture_chunk(xstart, xstop, w, h, pshared_data->poutimg);
	  IUNLOCK;
	  LOCK;
	  iter = pshared_data->update_segment_list.erase(iter);
	}
      pshared_data->bspectre_img_updated = false;
  }
  UNLOCK;
#endif
  m_gfxprimitives->draw_spectrum_texture(pw, cut);
}

void Cappdata::copy_track(Cgfxarea *pw)
{
  t_shared   *pshared_data = &m_shared_data;
  t_fcoord    fpos, fdim;
  bool        bupdated, bfiltering;
  int         w, h;
  
  pw->get_posf(&fpos);
  pw->get_dimf(&fdim);
  LOCK;
  w = pshared_data->trackimgw;
  h = pshared_data->trackimgh;
  bupdated = pshared_data->btrack_img_updated;
  UNLOCK;
  if (bupdated)
    {
      ILOCK;
      m_gfxprimitives->update_texture_texels(string("track_bitmap"), w, h, pshared_data->ptrackimg);
      IUNLOCK;
      LOCK;
      m_shared_data.btrack_img_updated = false;
      UNLOCK;
    }
  bfiltering = false;
  m_gfxprimitives->draw_texture(string("track_bitmap"), fpos, fdim, bfiltering);
}

void Cappdata::draw_scale(Cgfxarea *pw, int color, float minf, float maxf, float tensfactor)
{
  int      length;
  float    pente;
  float    lowlimit;
  float    f;
  float    fspan;
  float    begin;
  t_coord  pos;
  t_coord  dim;
  char     text[TXTSZ];
  int      top;

  pw->get_pos(&pos);
  top = pos.y;
  pw->get_dim(&dim);
  lowlimit = 5. * tensfactor;
  pente = ((float)dim.x) / ((80. *  tensfactor) - lowlimit);
  length = maxf >= lowlimit? dim.x - (pente * (maxf - lowlimit)) : dim.x;
  if (length < 0)
    length = 0;
  begin = tensfactor > 20? tensfactor : 30;
  fspan = dim.y / (maxf - minf);
  for (f = begin; f < maxf; f += (int)tensfactor)
    {
        pw->get_pos(&pos);
	pos.y = pos.y + dim.y - (int)(fspan * (f - minf));
	if (f != 10000)
	  {
	    draw_h_line(pos, length, color);
	    pos.x += dim.x + (dim.x * 5) / 100;
	    pos.y -= 8;
	    if (pos.y > top)
	      {
		if (maxf >= 1500 && tensfactor == 1000)
		  {
		    sprintf(text, "%1.0fk", f / 1000.);
		    rendertext(pos, color, text);
		  }
		else
		  if (maxf < 1500 && tensfactor == 100)
		    {
		      sprintf(text, "%3.0f", f);
		      rendertext(pos, color, text);	    
		    }
	      }
	  }
	else
	  {
	    draw_h_line(pos, dim.x, color);
	    pos.x += dim.x + (dim.x * 5) / 100;
	    pos.y -= 8;
	    sprintf(text, "10k");
	    rendertext(pos, color, text);
	  }
    }
}

void Cappdata::draw_attacks(Cgfxarea *pw, int color)
{
  t_shared *pshared_data = &m_shared_data;
  t_coord   pos;
  t_coord   dim;
  int       i;
  int       spwidth;
  int       length;
  t_coord   pline;
  float     max;
  float     v;
  float     ratio;
  float     vratio;

  pw->get_pos(&pos);
  pw->get_dim(&dim);
  LOCK;
  spwidth = pshared_data->dft_w;
  // Get the biggets value
  for (i = 0, max = 0; i < spwidth - 100; i++)
    {
      v = pshared_data->pattackdata[i];
      max = max > v? max : v;
    }
  ratio = (float)spwidth / (float)dim.x;
  vratio = (float)(dim.y - 1) / max;
  for (i = 0; i < dim.x; i++)
    {
      v = pshared_data->pattackdata[(int)((float)i * ratio)];
      length = vratio * v;
      if (length < 0)
	length = 0;
      if (length > dim.y)
	length = dim.y;
      pline.x = pos.x + i;
      pline.y = pos.y + (dim.y - length) - 1;
      length = length - 1 < 0? 0 : length - 1;
      draw_v_line(pline, length, color);
    }
  UNLOCK;
}

// Displays current sample time and draws bars on the seconds
void Cappdata::draw_time_scale(Cgfxarea *pw, int color, float viewtime, double timecode)
{
  char     text[TXTSZ];
  t_coord  pos;
  t_coord  dim;
  t_coord  linepos;
  int      tcolor = TXTCOLOR;
  double   minutes;
  double   seconds;
  double   mseconds;
  double   second;
  int      w, h, texth, textw;

  //--------------------------------------------
  // Render the timecode
  //--------------------------------------------
  minutes = floor(timecode / 60.);
  seconds = timecode - (minutes * 60.);
  mseconds = seconds;
  //seconds = floor(seconds);
  mseconds -= seconds;
  mseconds *= 100;
  if (minutes == 0)
    sprintf(text, "     %- 2.2fs", seconds);
  else
    sprintf(text, "%- 3.0fm %- 2.2fs", minutes, seconds);
  if (TTF_SizeText(m_font, text, &w, &h) == 0)
    {
      texth = h;
      textw = w;
    }
  else
    {
      texth = 15;
      textw = 80;
    }
  pw->get_pos(&pos);
  pw->get_dim(&dim);
  pos.x += dim.x - textw - 4;
  pos.y += dim.y - texth > 0? dim.y - texth : 0;
  //pos.y += (texth - dim.x > 2)? dim.x / 2 : 0;
  rendertext(pos, tcolor, text);
  // Draw the seconds bars
  pw->get_pos(&pos);
  pw->get_dim(&dim);
  linepos = pos;
  linepos.x += dim.x;
  seconds = timecode - (minutes * 60.);
  mseconds = seconds;
  seconds = floor(seconds);
  mseconds -= seconds;
  linepos.x -= (int)mseconds * (double)dim.x / viewtime;
  for (second = 0; second + mseconds < viewtime; second++)
    {
      linepos.x = pos.x + dim.x;
      linepos.x -= (second + mseconds) * (double)dim.x / viewtime;
      draw_v_line(linepos, (dim.y * 3) / 4, color);
    }
  // Draw the displayed time value
  sprintf(text, "view time:%-2.1fs", viewtime);
  pw->get_pos(&pos);
  pw->get_dim(&dim);
  pos.x += 4;
  if (TTF_SizeText(m_font, text, &w, &h) == 0)
    {
      pos.y += dim.y - texth > 0? dim.y - texth : 0;
    }
  else
    pos.y += dim.y / 4;
  rendertext(pos, tcolor, text);
}

void Cappdata::render_sound_strips_progress(Cgfxarea *pw, double currentime, double starttime, double viewtime)
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;
  std::list<t_audioOutCmd>::iterator cmditer;
  std::list<t_audioOutCmd> cmd_list_cpy;
  t_audioOutCmd *pcmd;
  double       elapsedtime;
  double       duration;
  int          band;
  int          curcut;
  t_coord      pos;
  t_coord      dim;
  int          length;
  t_coord      start;
  t_coord      stop;
  t_coord      linestart;
  double       timecode;
  t_fcoord     fstart, fstop;

  SLOCK;
  if (pshared_data->filter_cmd_list.size() > 0)
    {
      cmditer = pshared_data->filter_cmd_list.begin();
      while (cmditer != pshared_data->filter_cmd_list.end())
	{
	  pcmd = &(*cmditer);
	  if (pcmd->playstart != -1 && pcmd->notestate >= state_ready_2play)
	    {
	      if ((pcmd->playstart + pcmd->stop - pcmd->start < currentime || pcmd->stop - pcmd->start <= 0))
		{
		  if (pcmd->bloop_data)
		    {
		      pcmd->notestate = state_done;
		      cmditer++; // pass
		    }
		  else
		    cmditer = pshared_data->filter_cmd_list.erase(cmditer);
		}
	      else
		{
		  if (pcmd->playstart < currentime)
		    {
		      cmd_list_cpy.push_front((*cmditer));
		    }
		  cmditer++;
		}
	    }
	  else
	    cmditer++;
	}
    }
  SUNLOCK;  
  cmditer = cmd_list_cpy.begin();
  while (cmditer != cmd_list_cpy.end())
    {
      pcmd = &(*cmditer);
      pw->get_pos(&pos);
      pw->get_dim(&dim);
      timecode = starttime + viewtime;
      duration = pcmd->stop - pcmd->start;        // Track time: note stop - note start = m_time + m_duration - m_time
      //printf("elapsed time = %f\n", currentime - pcmd->playstart);
      assert(currentime > pcmd->playstart);
      elapsedtime = currentime - pcmd->playstart; // Current time
      start.x = getSpectrumX_from_time(pcmd->start, pos, dim, timecode, viewtime);
      stop.x  = getSpectrumX_from_time(pcmd->start + duration, pos, dim, timecode, viewtime);
      curcut  = getSpectrumX_from_time(pcmd->start + elapsedtime, pos, dim, timecode, viewtime);
      linestart.x = curcut;
      //
      for (band = 0; band < pcmd->bands; band++)
	{
	  start.y = getSpectrumY_from_Frequ(pcmd->fhicut[band], pos, dim, m_localfbase, m_localfmax);
	  stop.y = getSpectrumY_from_Frequ(pcmd->flocut[band], pos, dim, m_localfbase, m_localfmax);
	  stop.y = stop.y >= dim.y + pos.y? dim.y + pos.y - 1 : stop.y;
	  m_gfxprimitives->box(start.x, start.y, curcut, stop.y, 0x3FF0E230);
	  linestart.y = start.y;
	  length = abs(stop.y - start.y);
	  draw_v_line(linestart, length, 0xFFFFFFFF);
	  m_gfxprimitives->box(curcut, start.y, stop.x, stop.y, 0x7F0F4240);
	  m_gfxprimitives->rectangle(start.x, start.y, stop.x, stop.y, 0xFFFFFFFF);
	}
      cmditer++;
    }
}

void Cappdata::render_sound_strips_progress_on_track(Cgfxarea *pw, double tracklen, double currentime)
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;
  std::list<t_audioOutCmd>::iterator cmditer;
  std::list<t_audioOutCmd> cmd_list_cpy;
  t_audioOutCmd *pcmd;
  double       elapsedtime;
  double       duration;
  int          band;
  int          curcut;
  t_coord      pos;
  t_coord      dim;
  int          length;
  t_coord      start;
  t_coord      stop;
  t_coord      linestart;
  t_fcoord     fstart, fstop;

  SLOCK;
  if (pshared_data->filter_cmd_list.size() > 0)
    {
      cmditer = pshared_data->filter_cmd_list.begin();
      while (cmditer != pshared_data->filter_cmd_list.end())
	{
	  pcmd = &(*cmditer);
	  if (pcmd->playstart != -1 && pcmd->notestate >= state_ready_2play && (pcmd->playstart < currentime) && (pcmd->playstart + pcmd->stop - pcmd->start >= currentime))
	    {
	      cmd_list_cpy.push_front((*cmditer));
	    }
	  cmditer++;
	}
    }
  SUNLOCK;
  cmditer = cmd_list_cpy.begin();
  while (cmditer != cmd_list_cpy.end())
    {
      pcmd = &(*cmditer);
      pw->get_pos(&pos);
      pw->get_dim(&dim);
      duration = pcmd->stop - pcmd->start;        // Track time: note stop - note start = m_time + m_duration - m_time
      //printf("elapsed time = %f\n", currentime - pcmd->playstart);
      assert(currentime > pcmd->playstart);
      elapsedtime = currentime - pcmd->playstart; // Current time
      start.x = pos.x + (int)((float)dim.x * pcmd->start / tracklen);
      stop.x = pos.x + (int)((float)dim.x * (pcmd->start + duration) / tracklen);
      curcut  = pos.x + (int)((float)dim.x * (pcmd->start + elapsedtime) / tracklen);
      //
      if (stop.x > pos.x && start.x < pos.x + dim.x)
	{
	  start.x = start.x < pos.x? pos.x : start.x;
	  stop.x = stop.x > pos.x + dim.x? pos.x + dim.x : stop.x;
	  curcut = curcut < pos.x? pos.x : curcut;
	  curcut = curcut > pos.x + dim.x? pos.x + dim.x : curcut;
	  linestart.x = curcut;
	  for (band = 0; band < pcmd->bands; band++)
	    {
	      start.y = getSpectrumY_from_Frequ(pcmd->fhicut[band], pos, dim, m_localfbase, m_localfmax);
	      stop.y = getSpectrumY_from_Frequ(pcmd->flocut[band], pos, dim, m_localfbase, m_localfmax);
	      stop.y = stop.y >= dim.y + pos.y? dim.y + pos.y - 1 : stop.y;
	      m_gfxprimitives->box(start.x, start.y, curcut, stop.y, 0x3FF0E230);
	      linestart.y = start.y;
	      length = abs(stop.y - start.y);
	      draw_v_line(linestart, length, 0xFFFFFFFF);
	      m_gfxprimitives->box(curcut, start.y, stop.x, stop.y, 0x7F0F4240);
	      m_gfxprimitives->rectangle(start.x, start.y, stop.x, stop.y, 0xFFFFFFFF);
	    }
	}
      cmditer++;
    }
}

void Cappdata::render_selected_notes_box(Cgfxarea *pw, CScorePlacement *pplacement, double viewtime)
{
  bool   send_practice_msg, playing_finished;
  int    type;
  double duration, elapsed_time;
  t_note_selection_box *psb;

  duration = m_note_selection.box.stop_tcode - m_note_selection.box.start_tcode;
  // FIXME not rendering really, should be in a app state management
  send_practice_msg = check_if_notes_just_filtered(&m_note_selection.cmdlist);
  if (send_practice_msg)
    {
      // If filtering just finished, update the timecode used to show the progression in the box
      m_note_selection.state = loopstate_playing; // FIXME not used yet
      m_note_selection.play_timecode = m_last_time;
      // Start in the practice window if present
      if (m_pserver->is_practice_dialog_enable())
	{
	  psb = &m_note_selection.box;
	  type = practice_area;
#ifdef _DEBUG
	  printf("sending practice\n");
#endif
	  send_practice_message(type, psb->start_tcode, psb->stop_tcode, psb->hif, psb->lof, duration, m_note_selection.practice_speed);
	}
    }
  if (m_note_selection.state == loopstate_playing)
    {
      playing_finished = m_note_selection.play_timecode + (duration / m_note_selection.practice_speed) < m_last_time;
      if (playing_finished)
	{
	  //printf("Finished playing the selection.\n");
	  m_note_selection.state = loopstate_stop;
	}
    }
  // Draw the box
  if (duration > 0.001 /*&& (m_note_selection.play_timecode + duration > m_last_time)*/)
    {
      if (m_note_selection.state == loopstate_stop)
	{
	  elapsed_time = 0;
	}
      else
	{
	  elapsed_time = m_last_time - m_note_selection.play_timecode;
	  elapsed_time = elapsed_time * m_note_selection.practice_speed;
	}
      m_pScoreRenderer->render_sel_box(pw, pplacement, &m_note_selection.box, elapsed_time, true);
    }
}

void Cappdata::set_played_note_list_for_score_highlight(std::list<CNote*> *pplayed_note_list)
{
  t_shared *pshared_data = &m_shared_data;
//  std::list<t_audioOutCmd>::iterator filter_cmd_iterator;
  std::list<t_audioOutCmd*>           *pcmdlist;
  std::list<t_audioOutCmd*>::iterator  cmditer;
  t_audioOutCmd *pcmd;
  CNote  *pnote;
  double current_time, duration;

  //printf("current time=%f loop current time=%f\n", current_time, m_last_time);
  if (m_appstate == statepractice)
    {
      LOCK;
      current_time = pshared_data->timecode - (pshared_data->viewtime / 2);
      UNLOCK;
      pnote = m_pcurrent_instrument->get_first_note(0);
      while (pnote != NULL)
	{
	  if (pnote->m_time <= current_time && pnote->m_time + pnote->m_duration > current_time)
	    {
	      pplayed_note_list->push_front(pnote);      
	    }
	  pnote = m_pcurrent_instrument->get_next_note();
	}
    }
  else
    {
      current_time = m_last_time;
      SLOCK;
      pcmdlist = &m_note_selection.cmdlist;
      cmditer = pcmdlist->begin();
      while (cmditer != pcmdlist->end())
	{
	  pcmd = (*cmditer);
	  if (pcmd->pnote != NULL)
	    {
	      pnote = (CNote*)pcmd->pnote;
	      duration = (pcmd->stop - pcmd->start);
	      if (pcmd->playstart <= current_time &&
		  pcmd->playstart + duration > current_time &&
		  duration > 0.001)
		{
		  pplayed_note_list->push_front(pnote);
		}
	    }
	  cmditer++;
	}
      /*
	filter_cmd_iterator = pshared_data->filter_cmd_list.begin();
	while (filter_cmd_iterator != pshared_data->filter_cmd_list.end())
	{
	pcmd = &(*filter_cmd_iterator);
	if (pcmd->pnote != NULL)
	{
	pnote = (CNote*)pcmd->pnote;
	if (pcmd->playstart < current_time &&
	pcmd->playstart + pcmd->playdelay > current_time)
	{
	pplayed_note_list->push_front(pnote);
	}
	}
	filter_cmd_iterator++;
	}
      */
      SUNLOCK;
    }
}

void Cappdata::print_harmonic_note_info(Cgfxarea *pw, float f, int xsta, int xsto, int ysta, int ysto, int curoctave)
{
  t_coord pos;
  t_coord dim;
  const int cstrsz = 16;
  char    str[cstrsz];
  t_coord tpos;
  t_coord bpos;
  int     color;
  int     note, octave;
  float   ferr;
  int     rem;
  bool    bdifnotesharmonics;

  pw->get_pos(&pos);
  pw->get_dim(&dim);
  m_f2n.convert(f, &octave, &note, &ferr);
  if (note == -1)
    return ;
  m_f2n.notename(octave, note, str, cstrsz);
  bdifnotesharmonics = fabs(log2(curoctave) - round(log2(curoctave))) > 0.01;
  if (bdifnotesharmonics)
    tpos.x = std::max(xsta, xsto) + 5;
  else
    tpos.x = std::min(xsta, xsto) - 25;
  color = 0xFFFFFFFF;
  tpos.y = std::min(ysta, ysto) - 10 + abs(ysto - ysta) / 2;
  if (bdifnotesharmonics || (curoctave == 1))
    {
      // white to red depending on the harmonic coefficient error
      rem = (int)floor(256 * fabs(ferr / 0.4));
      //printf("err == %f %x\n", ferr, rem);
      color = 0xFFFFFFFF - rem - (rem << 8);
    }
  else
    sprintf(str, "  %d", curoctave);
  if (((tpos.x >= pos.x) && !bdifnotesharmonics) || ((tpos.x + 25 < pos.x + dim.x) && bdifnotesharmonics))
    {
      bpos.x = tpos.x + 1;
      bpos.y = tpos.y + 1;
      rendertext(bpos, 0xFF000000, str);
      rendertext(tpos, color, str);
    }
}

void Cappdata::render_sel_note_square(Cgfxarea *pw, int note, int octave, int xstart, int xstop, bool bbasefreqonly)
{
  t_coord  pos;
  t_coord  dim;
  float    fbase;
  float    fmax;
  float    notefh, notefl;
  float    f;
  int      yl, yh;
  int      j;
  float    basef;
  float    ferr;
  int      color;

  fbase = m_localfbase;
  fmax = m_localfmax;
  pw->get_pos(&pos);
  pw->get_dim(&dim);
  // Find the note in the middle of the box
  //note_from_spectrumy(pos, dim, y, &note, &octave, fbase, fmax);
  if (note == -1)
    return ;
  // Find the limits 
  basef = m_f2n.note2frequ(note, octave);
  for (j = 1; j <= HARMONICS && ((float)j * basef) < fmax; j++)
    {
      // Note limit as y coordinates
      m_f2n.convert((float)j * basef, &octave, &note, &ferr);
      m_f2n.note_frequ_boundary(&notefl, &notefh, note, octave);
      if (note == -1)
	break;
      yh = getSpectrumY_from_Frequ(notefh, pos, dim, fbase, fmax);
      yl = getSpectrumY_from_Frequ(notefl, pos, dim, fbase, fmax);
      color = 0xFFEFEFEF;
      //rectangleColor(m_sdlRenderer, xstart, yh, xstop, yl, color);
      m_gfxprimitives->rectangle(xstart, yh, xstop, yl, color);
      // Display odd harmonics note codes and even harmonics numbers
      f = (float)j * basef;
      print_harmonic_note_info(pw, f, xstart, xstop, yl, yh, j);
      if (bbasefreqonly)
	return;
    }
}

void Cappdata::render_sel_note_square(Cgfxarea *pw, int x, int y, bool bbasefreqonly)
{
  t_coord  pos;
  t_coord  dim;
  int      note, octave;
  float    fbase;
  float    fmax;

  fbase = m_localfbase;
  fmax = m_localfmax;
  pw->get_pos(&pos);
  pw->get_dim(&dim);
  // Find the note in the middle of the box
  note_from_spectrumy(pos, dim, y, &note, &octave, fbase, fmax);
  render_sel_note_square(pw, note, octave, m_cstart.x, x, bbasefreqonly);
  return;
}

void Cappdata::render_sel_note_square(Cgfxarea *pw, CNote *pn, double starttime, double viewtime, bool bbasefreqonly)
{
  std::list<t_notefreq>::iterator freqlist_iter;
  int                             absnote;
  int                             note, octave;
  t_coord                         pos;
  t_coord                         dim;
  int                             xstart, xstop;
  double                          timecode;

  pw->get_pos(&pos);
  pw->get_dim(&dim);
  timecode = starttime + viewtime;
  xstart = getSpectrumX_from_time(pn->m_time, pos, dim, timecode, viewtime);
  xstop  = getSpectrumX_from_time(pn->m_time + pn->m_duration, pos, dim, timecode, viewtime);
  freqlist_iter = pn->m_freq_list.begin();
  while (freqlist_iter != pn->m_freq_list.end())
    {
      absnote = m_f2n.frequ2note((*freqlist_iter).f);
      m_f2n.noteoctave(absnote, &note, &octave);
      render_sel_note_square(pw, note, octave, xstart, xstop, bbasefreqonly);
      freqlist_iter++;
    }
}

void Cappdata::render_measure_selection(Cgfxarea *pw, int measure_times, int xstart, int xstop)
{
  t_coord  pos;
  t_coord  dim;
  int      length;
  float    fi;
  int      i;
  int      steps, end;
  float    delta;
  int      color;

  pw->get_pos(&pos);
  pw->get_dim(&dim);
  end = pos.x + dim.x;
  //boxColor(m_sdlRenderer, xstart, pos.y, xstop, pos.y + dim.y, 0x7F0F4240);
  pos.x = xstart;
  length = dim.y;
  draw_v_line(pos, length, 0xFFFFFFFF);
  steps = 2 * measure_times; // the default measure is a 4/4, and show the subtimes
  delta = double(abs(xstop - xstart)) / steps;
  fi = delta;
  for (i = 1; i < steps; i++)
    {
      pos.x = xstart + (int)round(fi);
      if (pos.x >= end)
	return;
      color = (i % 2) == 0? /*0x0007D1D7*/0xFF7F7F7F : 0xFF40420F;
      draw_v_line(pos, length, color);
      fi += delta;
    }
  pos.x = xstop;
  if (pos.x < end)
    draw_v_line(pos, length, 0xFFFFFFFF);
}

void Cappdata::render_measures_edition(Cgfxarea *pw, CMesure *pmeasure, double starttime, double viewtime)
{
  int     xstart;
  int     xstop;
  t_coord pos;
  t_coord dim;
  int     i;
  double  t1, t2;
  double  timecode;

  pw->get_pos(&pos);
  pw->get_dim(&dim);
  // Draw from first to last seen bar
  timecode = starttime + viewtime;
  t1 = t2 = 0;
  i = pmeasure->m_start < starttime? (int)((pmeasure->m_start - starttime) / pmeasure->duration()) : 0;
  for (; t1 < starttime + viewtime && i < 100; i++)
    {
      t1 = pmeasure->m_start + (double)i * pmeasure->duration();
      t2 = pmeasure->m_start + (double)(i + 1) * pmeasure->duration();
      if (t1 > starttime && t1 < starttime + viewtime)
	{
	  xstart = getSpectrumX_from_time(t1, pos, dim, timecode, viewtime);
	  xstop =  getSpectrumX_from_time(t2, pos, dim, timecode, viewtime);
	  if (xstop >= pos.x + dim.x - 1)
	    xstop = xstart + (int)(pmeasure->duration() * (float)dim.x / viewtime);
	  render_measure_selection(pw, pmeasure->m_times, xstart, xstop);
	}
    }
}

void Cappdata::draw_measures_on_track(Cgfxarea *pw, CScore *pscore, double tracklen)
{
  std::list<CMesure*>::reverse_iterator it;
  CMesure                              *pmeasure;
  t_coord                               pos, barpos;
  t_coord                               dim;
  double                                last_limit, t;
  int                                   i;
  int                                   color;

  pw->get_pos(&pos);
  pw->get_dim(&dim);
  color = 0x6F7F7F7F;
  barpos.y = pos.y;
  last_limit = tracklen;
  it = pscore->m_mesure_list.rbegin();
   while (it != pscore->m_mesure_list.rend())
    {
      pmeasure = (*it);
      t = pmeasure->m_start;
      while (t < last_limit)
	{
	  barpos.x = pos.x + (int)(t * (float)dim.x / tracklen);
	  draw_v_line(barpos, dim.y, color);
	  t += pmeasure->duration();
	  i++;
	}
      last_limit = pmeasure->m_start;
      it++;
    }
}

// Reder visible measures on the spectrum, used when moving a note
void Cappdata::render_measures_on_spectrum(Cgfxarea *pw, CScore *pscore, double starttime, double viewtime)
{
  std::list<CMesure*>::reverse_iterator it;
  CMesure                              *pmeasure;
  double                                last_limit;

  last_limit = starttime + viewtime;
  it = pscore->m_mesure_list.rbegin();
   while (it != pscore->m_mesure_list.rend())
    {
      pmeasure = (*it);
      if (pmeasure->m_start < last_limit)
	{
	  last_limit = pmeasure->m_start;
	  render_measures_edition(pw, pmeasure, starttime, viewtime);	  
	}
      it++;
    }
}

int Cappdata::instrument_tab_height(Cgfxarea *pw)
{
  t_coord dim;
  int     insnum;

  pw->get_dim(&dim);
  insnum = m_pscore->instrument_number();
  return (insnum < 4? dim.y / 4 :  dim.y / insnum);
}

void Cappdata::render_instrument_tabs(Cgfxarea *pw)
{
  CInstrument* pins;
  t_coord  pos, dim;
  t_fcoord tabpos, tabdim;
  float    radius;
  int      i, y;
  int      tabsize;
  int      sel_pos, tcolor;
  string   instrument_name;
  t_fcoord dest_pos, dest_dim;
  bool     filtering = true;
  bool     blend = true;
  int      color;

  pw->get_pos(&pos);
  pw->get_dim(&dim);
  tabsize = instrument_tab_height(pw);
  pins = m_pscore->get_first_instrument();
  i = 0;
  sel_pos = 0;
  while (pins != NULL)
    {
      y = pos.y + i * tabsize;
      // Darken when not used
      instrument_name = pins->m_name;
      dest_pos.x = pos.x;
      dest_pos.y = y;
      dest_dim.x = dim.x;
      dest_dim.y = tabsize;
      m_gfxprimitives->draw_texture(string("nail_") + instrument_name, dest_pos, dest_dim, filtering, blend);
      if (pins != m_pcurrent_instrument) // Darken the texture if not selected
	{
	  color = 0x7F3F3F3F;
	  tabpos.x = pos.x;
	  tabpos.y = y;
	  tabdim.x = dim.x + 1;
	  tabdim.y = tabsize;
	  radius = 6;
	  m_gfxprimitives->rounded_box(tabpos, tabdim, color, true, radius);
	  //m_gfxprimitives->box(pos.x, y, pos.x + dim.x + 1, y + tabsize, color, filtering);
	}
      else
	sel_pos = y;
      pins = m_pscore->get_next_instrument();
      i++;
    }
  tcolor = 0xFFFFFFFF;
  char str[64];
  snprintf(str, 64, "%s %d", m_pcurrent_instrument->get_name().c_str(), m_pcurrent_instrument->identifier());
  pw->get_pos(&pos);
  pw->get_dim(&dim);
  tabpos.x = pos.x + 5;
  tabpos.y = sel_pos + 4;
  tabdim.x = dim.x - 8;
  tabdim.y = tabsize / 6;
  m_gfxprimitives->print(str, m_font, tabpos, tabdim, tcolor, true);
}

void Cappdata::draw_countdown(Cgfxarea *pw, double countdown_time)
{
  t_coord     pos, dim;
  t_fcoord    fpos, fdim;
  char        str[4096];
  int         outline;
  bool        blended;

  pw->get_pos(&pos);
  pw->get_dim(&dim);
  sprintf(str, "%1.0f", ceil(fabs(COUNTDOWN - countdown_time)));
  fdim.x = m_width / 10;
  fdim.y = m_height / 10;
  fpos.x = (dim.x / 2) - (fdim.x / 2); // FIXMe middle of the screen
  fpos.y = (dim.y / 2) - (fdim.y / 2);
  fdim.x = m_width / 10;
  fdim.y = m_height / 10;
  outline = 2;
  blended = true;
  m_gfxprimitives->print(str, m_bigfont, fpos, fdim, WHITE, blended, outline, BLACK);
}

void Cappdata::render_background_tile()
{
#ifndef _DEBUG
#define SKINUSED
#ifdef SKINUSED
  t_fcoord  fpos, fdim;
  bool      filtering = true;

  fpos.x = fpos.y = 0;
  fdim.x = m_width;
  fdim.y = m_height;
  m_gfxprimitives->draw_texture(string("firstskin"), fpos, fdim, filtering);
#else
  t_coord      tdim;
  SDL_Surface *ptexture;
  SDL_Rect     dstrect;
  int          x;
  int          y;

  ptexture = m_picturelist->get_texture(std::string("back_carbon"), &tdim);
  if (ptexture != NULL)
    {
      dstrect.w = tdim.x;
      dstrect.h = tdim.y;
      //dstrect.w = 16;
      //dstrect.h = 16;
      for (y = 0; y < m_height; y += dstrect.h / 4)
	{
	  dstrect.y = y;
	  for (x = 0; x < m_width; x += dstrect.w / 2)
	    {
	      dstrect.x = x;
	      SDL_RenderCopy(m_sdlRenderer, ptexture, NULL, &dstrect);
	    }
	}
    }
#endif
#else
  m_gfxprimitives->box(0, 0, m_width, m_height, 0xFF0000FF);
#endif
}

void Cappdata::drawhandcursor(t_coord mousepos)
{
  t_fcoord     pos, dim;
  bool         bfiltering;
  bool         blend = true;
  t_coord      tdim;
  string       tname = string("cursorhand");

  if (m_gfxprimitives->get_texture_size(tname, &tdim))
    {
      pos.x = mousepos.x - tdim.x / 2;
      pos.y = mousepos.y - tdim.y / 3 ;
      dim.x = 1. * tdim.x;
      dim.y = 1. * tdim.y;
      bfiltering = true;
      m_gfxprimitives->draw_texture(tname, pos, dim, bfiltering, blend);
    }
}

void Cappdata::render_notes_on_spectrum(Cgfxarea *pw, CInstrument *pinstrument, double starttime, double viewtime)
{
  CNote *pn;
  bool   basefreqonly;

  pn = pinstrument->get_first_note(0.);
  while (pn != NULL)
    {
      if (pn->m_time + pn->m_duration > starttime && pn->m_time < starttime + viewtime)
	{
	  basefreqonly = true;
	  render_sel_note_square(pw, pn, starttime, viewtime, basefreqonly);
	}
      pn = pinstrument->get_next_note();
    }
}

void Cappdata::draw_edited_times(Cgfxarea *pw, CInstrument *pinstrument, float tracklen)
{
  //t_shared *pshared_data = (t_shared*)&m_shared_data;
  t_coord   pos;
  t_coord   dim;
  float     pixels_per_second;
  int       i, len;
  const int ctsize = 4096;
  char      edited[4096];
  CNote    *pn;
  int       color = MAKE_COLOR_32(51, 51, 251, 255);

  pw->get_pos(&pos);
  pw->get_dim(&dim);
  for (i = 0; i < ctsize; i++)
    {
      edited[i] = 0;
    }
  pixels_per_second = ((float)dim.x) / tracklen;
  pn = pinstrument->get_first_note(0.);
  while (pn != NULL)
    {
      i = (int)(pn->m_time * pixels_per_second);
      len = (int)(pn->m_duration * pixels_per_second);
      while (len >= 0)
	{
	  edited[i + len] = 1;
	  len--;
	}
      pn = pinstrument->get_next_note();
    }
  len = dim.y / 12.;
  pos.y += (dim.y - len) / 2;
  for (i = 0; i < dim.x; i++)
    {
      if (edited[i] != 0)
	{
	  m_gfxprimitives->vline(pos.x, pos.y, len, color);
	}
      pos.x++;
    }
}

void Cappdata::draw_fps(double last_time)
{
  static double prevtime = -1;
  static double avgfps = 0;
  double        instfps;
  Cgfxarea     *pw;
  char          text[TXTSZ];
  t_coord       pos;

  if (prevtime == -1 && last_time > 0)
    {
      prevtime = last_time;
      return;
    }
  pw = m_layout->get_area((char*)"main");
  if (pw != NULL && (last_time - prevtime) > 0)
    {
      if (last_time > 0)
	{
	  instfps = 1. / (last_time - prevtime);
	  prevtime = last_time;
	  avgfps = (25. * avgfps + instfps) / 26.;
	}
      snprintf(text, TXTSZ, "%f", avgfps);
      pw->get_dim(&pos);
      pos.x = pos.x / 2;
      pos.y = pos.y / 2;
      rendertext(pos, WHITE, text);
    }
}

void Cappdata::render_gui()
{
  t_shared *pshared_data = &m_shared_data;
  Cgfxarea *pw;
  int      scalebck = MAKE_COLOR_32(35, 35, 35, 255);
  t_coord  pos, dim;
  int      cx, cy;
  bool     bcountdown;
  double   countdown;
  double   viewtime;
  double   starttime;
  float    tracklen;

  LOCK;
  viewtime  = pshared_data->viewtime;
  starttime = pshared_data->timecode - viewtime;
  tracklen  = pshared_data->trackend;
  bcountdown = false;
  countdown = 0;
  if (pshared_data->play_State == state_wait_practice || pshared_data->play_State == state_wait_practiceloop)
    {
      bcountdown = true;
      countdown = m_last_time - pshared_data->practice_countdown_tick;
    }
  UNLOCK;
  m_gfxprimitives->Clear();
#define BACKGROUND
#ifdef BACKGROUND
  // Background rendering
  render_background_tile();
  pw = m_layout->get_first();
  while (pw != NULL)
    {
      if (strcmp(pw->m_name, "spectre") == 0)
	{
	  copy_spectrogram(pw);
	}
      if (strcmp(pw->m_name, "freqscale") == 0)
	{
	  draw_basic_rectangle(pw, scalebck);
	  if (!m_pianofrequdisplay)
	    draw_freq_scale(pw, TXTCOLOR, m_shared_data.fbase, m_shared_data.fmax);
	  else
	    draw_piano_scale(pw, TXTCOLOR, m_shared_data.fbase, m_shared_data.fmax, m_ccurrent);
	}
      if (strcmp(pw->m_name, "timescale") == 0)
	{
	  draw_basic_rectangle(pw, scalebck);
	  draw_attacks(pw, 0xFf655A0F);
	  draw_time_scale(pw, MAKE_COLOR_32(142, 140, 150, 255), m_shared_data.viewtime, m_shared_data.lastimgtimecode);
	}
      if (strcmp(pw->m_name, "track") == 0)
	{
	  draw_basic_rectangle(pw, MAKE_COLOR_32(40, 40, 40, 255));
	  copy_track(pw);
	  draw_edited_times(pw, m_pcurrent_instrument, tracklen);
	  draw_measures_on_track(pw, m_pscore, tracklen);
	}
      if (strcmp(pw->m_name, "score") == 0)
	{
	  std::list<CNote*> played_note_list;

	  draw_basic_rectangle(pw, MAKE_COLOR_32(240, 240, 250, 255));
	  // Place the score image in a descriptive list
	  m_pScorePlacement->clear();
	  m_pScorePlacement->set_metrics(pw, starttime, viewtime, m_appstate == statepractice);
	  m_pScorePlacement->place_notes_and_bars(m_pscore, m_pcurrent_instrument);
	  // Render the score
	  set_played_note_list_for_score_highlight(&played_note_list);
	  m_pScoreRenderer->render(pw, m_pScorePlacement, m_pScoreEdit->get_edit_state(), &played_note_list);
	  render_selected_notes_box(pw, m_pScorePlacement, viewtime);
	}
      if (strcmp(pw->m_name, "instrument") == 0)
	{
	  render_instrument_tabs(pw);
	}
      pw = m_layout->get_next();
    }
#endif
  // Foreground rendering
  pw = m_layout->get_first();
  while (pw != NULL)
    {
      if (strcmp(pw->m_name, "spectre") == 0)
	{
	  stick_to_area(pw, m_ccurrent.x, m_ccurrent.y, cx, cy);
	  if (m_appstate == stateaddnote ||
	      m_appstate == stateaddnotedirect)
	    {
	      render_sel_note_square(pw, cx, cy);
	      //printf("line %d %d l%d\n", pos.x, pos.y, length);
	    }
	  if (m_appstate == stateselectsound)
	    {
	      m_gfxprimitives->rectangle(m_cstart.x, m_cstart.y,
					 cx, cy, 0xFF0FE20F);
	    }
	  if (m_appstate == stateselectharmonics)
	    {
	      render_sel_note_square(pw, cx, cy);
	    }
	  if (m_appstate == stateaddmeasure)
	    {
	      render_measure_selection(pw, 4, m_cstart.x, cx);
	      render_notes_on_spectrum(pw, m_pcurrent_instrument, starttime, viewtime);
	    }
	  if (m_appstate == statemovehand)
	    {
	      drawhandcursor(m_ccurrent);
	    }
	  // Render the manipulated note on the score edition area
	  t_edit_state* pedit_state = m_pScoreEdit->get_edit_state();
	  if (pedit_state->pmodified_note != NULL)
	    {
	      //printf("rendering tmp note %x\n", pedit_state->pmodified_note);
	      render_measures_on_spectrum(pw, m_pscore, starttime, viewtime);
	      render_sel_note_square(pw, pedit_state->pmodified_note, starttime, viewtime);
	    }
	  else
	    {
	      //printf("tmp note is NULL\n");
	    }
	  // Render the manipulated measure on the spectrum area
	  if (pedit_state->pmodified_measure != NULL)
	    {
	      render_measures_edition(pw, pedit_state->pmodified_measure, starttime, viewtime);
	      render_notes_on_spectrum(pw, m_pcurrent_instrument, starttime, viewtime);
	    }
	  else
	    {
	      //printf("tmp measure is NULL\n");
	    }
	  // Render the progress of the selected sound strips being played
	  render_sound_strips_progress(pw, m_last_time, starttime, viewtime);
	}
      if (strcmp(pw->m_name, "track") == 0)
	{
	  stick_to_area(pw, m_ccurrent.x, m_ccurrent.y, cx, cy);
	  if (m_appstate == stateselectsoundontrack)
	    {
	      pw->get_pos(&pos);
	      pw->get_dim(&dim);
	      m_gfxprimitives->rectangle(m_cstart.x, pos.y,
					 cx, pos.y + dim.y, 0xFF0FE20F);
	    }
	  render_sound_strips_progress_on_track(pw, tracklen, m_last_time);
	}
      if (strcmp(pw->m_name, "score") == 0)
	{
	  // Draw the selection square
	  if (m_appstate == stateeditscore)
	    {
	      //
	    }
	}
      if (strcmp(pw->m_name, "main") == 0)
	{
	  if (bcountdown)
	    draw_countdown(pw, countdown);
	}
      pw = m_layout->get_next();
    }
  //draw_fps(m_last_time);
  // Render the cards
  render_cards();
/*
  t_fcoord epos, edim;

  epos.x = epos.y = 200;
  edim.x = 380;
  edim.y = 120;
  if (m_pserver->m_write_completed)
    m_gfxprimitives->rounded_box(epos, edim, 0xFFFF00FF, true, 40);
  else
    {
      dim.x = 80;      
      m_gfxprimitives->rounded_box(epos, edim, 0xFF0000FF, true, 40);
    }
  float radius = 60;
  pos.x = pos.y = 500;
  m_gfxprimitives->circle(pos, radius, radius / 2., 0xFF00EF52, true);
  //m_gfxprimitives->disc(pos, radius, 0xFF00EF52, true);
*/

  // Update the screen
  m_gfxprimitives->RenderPresent();
  //---------------------------------------------------
  wakeup_spectrogram_thread();

#ifdef blu
  pthread_mutex_lock(&pshared_data->datamutex);
  printf("looplimit=%d maxtime=%d\n", g_ct, g_dur);
  g_ct = 0;
  g_tm = 0;
  g_dur = 0;
  pthread_mutex_unlock(&pshared_data->datamutex);
#endif
}

