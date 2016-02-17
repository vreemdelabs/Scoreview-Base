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
#include "tcp_message_receiver.h"
#include "messages_network.h"
#include "card.h"
#include "app.h"

// Plays second to 16th harmonics of a note
void Cappdata::create_filter_play_harmonics_message(CNote *pnote, double playdelay, bool bloopdata, std::list<t_audioOutCmd*> *pcmdlist)
{
  t_shared      *pshared_data = (t_shared*)&m_shared_data;
  std::list<t_notefreq>::iterator iter;
  float          basef;
  t_audioOutCmd  cmd;
  t_audioOutCmd *pcmd;
  float          practicespeed;

  LOCK;
  practicespeed = pshared_data->practicespeed;
  UNLOCK;
  // Note frequency, FIXME chord note
  iter = pnote->m_freq_list.begin();
  while (iter != pnote->m_freq_list.end())
    {
      basef = (*iter).f;
      // Start and stop time
      cmd.start = pnote->m_time;
      cmd.stop  = pnote->m_time + pnote->m_duration;
      cmd.playdelay = playdelay * 1 / practicespeed; // relative to the time after the filtering
      cmd.playstart = -1;
      cmd.bsubstract = false;
      cmd.bloop_data = bloopdata;
      cmd.notestate = state_wait_filtering;
      cmd.pnote = pnote;
      if (fabs(cmd.stop - cmd.start) > 0.005)
	{
	  set_cmd_bands(cmd, basef, m_localfbase, m_localfmax);
	  //printf("pushing f=%f start=%f stop=%f listsize=%d\n", basef, cmd.start, cmd.stop, (int)pshared_data->filter_cmd_list.size());
	  SLOCK;
	  pshared_data->filter_cmd_list.push_front(cmd);
	  pcmd = &(*pshared_data->filter_cmd_list.begin());
	  SUNLOCK;
	  pcmdlist->push_front(pcmd);
	}
      iter++;
    }
  //wakeup_bandpass_thread();
}

bool Cappdata::check_if_notes_just_filtered(std::list<t_audioOutCmd*> *pcmdlist)
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;
  std::list<t_audioOutCmd*>::iterator iter;
  bool res;

  res = pcmdlist->size() > 0;
  SLOCK;
  iter = pcmdlist->begin();
  while (iter != pcmdlist->end())
    {
      if ((*iter)->notestate == state_ready_2play)
	{
	  (*iter)->notestate = state_playing;
	  res = res && true;
	}
      else
	{
	  res = false;
	}
      iter++;
    }
  SUNLOCK;
#ifdef _DEBUG
  if (res)
    printf("Notes just filtered\n");
#endif
  return res;
}

// The bandpass thread will filter again theese commands
void Cappdata::reset_filter_on_loop_commands(t_note_selection *pnote_selection)
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;
  std::list<t_audioOutCmd*> *pcmdlist;
  std::list<t_audioOutCmd*>::iterator cmditer;
  t_audioOutCmd* pcmd;

  pcmdlist = &pnote_selection->cmdlist;
  if (pcmdlist->size() > 0)
    {
      SLOCK;
      assert(pshared_data->filter_cmd_list.size() > 0);
      cmditer = pcmdlist->begin();
      while (cmditer != pcmdlist->end())
	{
	  pcmd = (*cmditer);
	  assert(pcmd->bloop_data);
	  pcmd->notestate = state_wait_filtering; // They will be filtered and played again
	  pcmd->playstart = -1;
	  if (pnote_selection->practice_speed != pshared_data->practicespeed)
	    {
	      pcmd->playdelay = pcmd->playdelay * pnote_selection->practice_speed / pshared_data->practicespeed;
	    }
	  cmditer++;
	}
      SUNLOCK;
      if (pnote_selection->practice_speed != pshared_data->practicespeed)
	{
	  pnote_selection->practice_speed = pshared_data->practicespeed;
	}
      wakeup_bandpass_thread();
    }
}

// Deletes any filtered sound band except note selections if specified
void Cappdata::delete_filter_commands(std::list<t_audioOutCmd*> *pcmdlist, bool bkeep_loop_cmds)
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;
  std::list<t_audioOutCmd>::iterator cmditer;
  t_audioOutCmd                     *pcmd;

  SLOCK;
  cmditer = pshared_data->filter_cmd_list.begin();
  while (cmditer != pshared_data->filter_cmd_list.end())
    {
      pcmd = &(*cmditer);
      if (pcmd->bloop_data && bkeep_loop_cmds)
	cmditer++;
      else
	cmditer = pshared_data->filter_cmd_list.erase(cmditer);
    }
  SUNLOCK;
  if (!bkeep_loop_cmds)
    {
      pcmdlist->clear();
    }
}

// If anythin is playing, send a command to clear it to the sound thread
void Cappdata::flush_filtered_strips()
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;
  t_audio_track_cmd acmd;
  bool              bempty;

  FLOCK;
  bempty = pshared_data->filtered_sound_list.size() == 0;
  FUNLOCK;
  if (bempty)
    return ;
  LOCK;
  // Send a stop command to the main app
  acmd.v = 0;
  acmd.command = ac_flush_strips;
  acmd.direction = ad_app2threadaudio;
  pshared_data->audio_cmds.push_back(acmd);
  UNLOCK;
  // Wait for the flush
  FLOCK;
  bempty = pshared_data->filtered_sound_list.size() == 0;
  FUNLOCK;
  while (!bempty)
    {
      FLOCK;
      bempty = pshared_data->filtered_sound_list.size() == 0;
      FUNLOCK;
      usleep(5000);
    }
}

// Clears any playing note
void Cappdata::stop_note_selection(t_note_selection *psel)
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;
  std::list<t_audioOutCmd*>::iterator cmditer;
  t_audioOutCmd* pcmd;

  if (psel->cmdlist.size() > 0 && psel->state != loopstate_stop)
    {
      delete_filter_commands(&psel->cmdlist, true);
      flush_filtered_strips();
      psel->play_timecode = -1;
      psel->state = loopstate_stop;
      SLOCK;
      cmditer = psel->cmdlist.begin();
      while (cmditer != psel->cmdlist.end())
	{
	  pcmd = (*cmditer);
	  assert(pcmd->bloop_data);
	  pcmd->notestate = state_done; // Nothing to do with it
	  pcmd->playstart = -1;
	  cmditer++;
	}
      SUNLOCK;
    }
}

void Cappdata::remove_note_selection(t_note_selection *psel)
{
  if (psel->cmdlist.size() > 0)
    {
      delete_filter_commands(&psel->cmdlist, false);
      flush_filtered_strips();
      psel->play_timecode = psel->box.start_tcode = psel->box.stop_tcode = -1;
      psel->state = loopstate_stop;
    }
}

// Transfers a note selection in the editor to the application note selection and play the notes
void Cappdata::create_note_box(t_note_selbox *pnsb, t_note_selection *pappsel)
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;
  std::list<CNote*>::iterator Note_iter;
  CNote* pnote;
  double note_delay_time;
  bool   bloop = true;

  if (pnsb->bupdate)
    {
      delete_filter_commands(&pappsel->cmdlist, false);
      pappsel->play_timecode = pappsel->box.start_tcode = pappsel->box.stop_tcode = -1;
      flush_filtered_strips();
      if (pnsb->Note_list.size() > 0)
	{
	  pappsel->box = pnsb->nsb;
	  LOCK;
	  pappsel->practice_speed = pshared_data->practicespeed;
	  UNLOCK;
	  // Check if they need to be played
	  Note_iter = pnsb->Note_list.begin();
	  while (Note_iter != pnsb->Note_list.end())
	    {
	      pnote = *Note_iter;
	      note_delay_time = pnote->m_time - pnsb->nsb.start_tcode;
	      create_filter_play_harmonics_message(pnote, note_delay_time, bloop, &pappsel->cmdlist);
	      Note_iter = pnsb->Note_list.erase(Note_iter);
	    }
	  wakeup_bandpass_thread();
	}
      pnsb->bupdate = false;
    }
}

void Cappdata::play_edition_sel_notes(t_edit_state *pedit_state)
{
  if (pedit_state != NULL)
    { 
      // The send_practice_message will be sent only when filetring is finished, cause filtering takes noticeable time
      create_note_box(&pedit_state->note_sel, &m_note_selection);
    }
}

bool Cappdata::queued_filtered_band()
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;
  std::list<t_audioOutCmd>::iterator cmditer;
  t_audioOutCmd* pcmd;
  bool           bresult;

  bresult = false;
  SLOCK;
  if (pshared_data->filter_cmd_list.size() > 0)
    {
      cmditer = pshared_data->filter_cmd_list.begin();
      while (cmditer != pshared_data->filter_cmd_list.end())
	{
	  pcmd = &(*cmditer);
	  bresult = bresult && (pcmd->notestate <= state_playing);
	  cmditer++;
	}
    }
  SUNLOCK;
  return bresult;
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
  delete_filter_commands(&m_note_selection.cmdlist, true); // keep the note selction bands
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
  cmd.bloop_data = false;
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
