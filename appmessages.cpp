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
#include <iterator>
#include <list>
#include <vector>
#include <unistd.h>
#include <assert.h>

//#include <ctime>
#include <sys/time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
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
#include "card.h"
#include "cardnames.h"
#include "messages.h"
#include "tcp_message_receiver.h"
#include "messages_network.h"
#include "message_decoder.h"
#include "app.h"
#include "sdlcalls.h"

using namespace std;

void Cappdata::send_instrument_list()
{
  CInstrument                      *pins;
  scmsg::Cmessage_coding            coder;
  scmsg::t_instrument               ins;
  std::list<scmsg::t_instrument>    ilist;
  string                            command;
  const int                         cmsgsize = 4096;
  char                              msg[cmsgsize];
  int                               size;

  pins = m_pscore->get_first_instrument();
  while (pins != NULL)
    {
      ins.name = pins->m_name;
      ins.voice_index = pins->identifier();
      pins = m_pscore->get_next_instrument();
      ilist.push_back(ins);
    }
  command = coder.create_instrument_list_message(&ilist);
  if (coder.build_wire_message(msg, cmsgsize, &size, command))
    {
#ifdef _DEBUG
      printf("sending instrument list.\n");
#endif
      m_pserver->Send_to_dialogs(message_send_instrument_list, msg, size);
    }
}

void Cappdata::send_apidevice_list()
{
  string                 command;
  scmsg::Cmessage_coding coder;
  const int              cmsgsize = 4096;
  char                   msg[cmsgsize];
  int                    size;

  command = coder.create_audioIO_config_message(&m_shared_data.pa_api_list, &m_shared_data.chs);
  if (coder.build_wire_message(msg, cmsgsize, &size, command))
    {
#ifdef _DEBUG
      printf("sending apis list to dialog:\"%s\"", command.c_str());
#endif
      m_pserver->Send_to_dialogs(message_send_pa_devices_list, msg, size);
    }
}

#define BOOLTOSTR(B) (B? "1" : "0")

void Cappdata::send_configuration()
{
  string                 command;
  scmsg::t_appconfig     cfg;
  scmsg::Cmessage_coding coder;
  const int              cmsgsize = 4096;
  char                   msg[cmsgsize];
  int                    size;

  cfg.recordAtStart = m_brecord_at_start;
  cfg.doNotChangeOpenedFiles = m_bdonotappendtoopen;
  command = coder.create_config_message(&cfg);
  if (coder.build_wire_message(msg, cmsgsize, &size, command))
    {
#ifdef _DEBUG
      printf("sending configuration to dialog:\"%s\"", command.c_str());
#endif
      m_pserver->Send_to_dialogs(message_send_configuration, msg, size);
    }
}

// -------------------------------------------------------------------------------------------------------------------
// Where to call the practice messages functions?
//
// Practice dialog started: send everything            gedaan
//
// Note moves:    send note addrem                     update_practice_view (main app) + set_note_changes_list (edit)
// + note:        send note addrem                     gedaan
// - note:        send note addrem                     update_practice_view + ...
// fuse notes:    send note addrem                     update_practice_view + ...
// notes times:   send note addrem                     update_practice_view + ...
// string change: send note addrem                     gedaan
//
// Do not care, only transmited on practice start
// + measure:     send measure addrem
// - measure:     send measure addrem
// measure times: send measure addrem
// move measure:  send measure addrem
//
// Change timecode:   send stop at                     gedaan
// Select instrument: send stop at                     gedaan
// Play area:         practice message but only play   gedaan
// Practice all:      send everything + start practice gedaan
// Practice loop:     send everything + start practice in a loop
//

void Cappdata::send_score_to_practice(CScore *pscore)
{
  CInstrument             *pins;
  int                      nbnotes;
  int                      i, pos;
  string                   command;
  scmsg::Cmessage_coding   coder;
  const int                cmsgsize = 4096;
  char                     msg[cmsgsize];
  int                      size;

  if (!m_pserver->is_practice_dialog_enable())
    return;
#ifdef _DEBUG
  printf("Score transfer message: \n");
#endif
  command = coder.create_score_message(pscore);
  if (coder.build_wire_message(msg, cmsgsize, &size, command))
    {
      //printf("sending the score to practice:\"%s\"\n", command.c_str());
      m_pserver->Send_to_dialogs(message_score_transfer, msg, size);
    }
  else
    {
      printf("Failed sending the score to practice:\"%s\"\n", command.c_str());
    }
  pins = pscore->get_first_instrument();
  while (pins != NULL)
    {
      nbnotes = m_pcurrent_instrument->note_count();
      //printf("using instrument= \"%s\"\n", pins->get_name().c_str());
      for (i = 0; i < nbnotes / NOTES_PER_NOTE_TRANSFER; i++)
	{
	  pos = i * NOTES_PER_NOTE_TRANSFER;
	  command = coder.create_notelist_message(pins, pos, pos + NOTES_PER_NOTE_TRANSFER);
	  if (coder.build_wire_message(msg, cmsgsize, &size, command))
	    {
#ifdef _DEBUG
	      printf("sending notes to practice:\"%s\"\n", command.c_str());
#endif
	      m_pserver->Send_to_dialogs(message_note_transfer, msg, size);
	    }	  
	}
      if (nbnotes % NOTES_PER_NOTE_TRANSFER > 0)
	{
	  command = coder.create_notelist_message(pins, nbnotes - (nbnotes % NOTES_PER_NOTE_TRANSFER), nbnotes);
	  if (coder.build_wire_message(msg, cmsgsize, &size, command))
	    {
#ifdef _DEBUG
	      printf("sending notes to practice:\"%s\"\n", command.c_str());
#endif
	      m_pserver->Send_to_dialogs(message_note_transfer, msg, size);
	    }
	}
      pins = pscore->get_next_instrument();
    }
}

void Cappdata::send_practice_message(int mtype, double timecode, double stoptimecode, float hif, float lowf, float viewtime, float practicespeed)
{
  bool                     reload, countdown, practice;
  struct timeval           tv;
  string                   command;
  scmsg::Cmessage_coding   coder;
  scmsg::t_practice_params prac;
  const int                cmsgsize = 4096;
  char                     msg[cmsgsize];
  int                      size;
  epractice_msg_type       type;

  if (!m_pserver->is_practice_dialog_enable())
    return;
  type = (Cappdata::epractice_msg_type)mtype;
  switch (type)
    {
    case practice_stop_at:
      {
	reload = false;
	countdown = false;
	practice = false;
      }
      break;
    case practice_area:
      {
	reload = false;
	countdown = false;
	practice = true;
      }
      break;
    case practice_update:
      {
	reload = true;
	countdown = false;
	practice = false;
      }
      break;
    case practice_area_loop_countdown:
      {
	reload = false;
	countdown = true;
	practice = false;
      }
      break;
    case practice_update_countdown:
      {
	reload = true;
	countdown = true;
	practice = true;
      }
      break;
    default:
	reload = countdown = practice = false;
      break;
    };
  if (countdown)
    {   
      if (gettimeofday(&tv, NULL) != 0)
	{
	  printf("Error: gettimeofday failed.\n");
	  return ;
	}
      prac.countdownDaytime = (double) +  (double)tv.tv_sec + (double)tv.tv_usec / 1000000.;
      //printf("Sending countdown refernce of %f\n", prac.countdownDaytime);
    }
  else
    {
      prac.countdownDaytime = -2;
    }
  // This network message will send the score to the client through TCPIP.
  if (reload)
    {
#ifdef _DEBUG
      printf("* Score message\n");
#endif
      send_score_to_practice(m_pscore);
    }
  // Then the following practice message will pick an instrument in it.
  prac.curtimecode = timecode;
  prac.reload = reload;
  prac.start_practice = practice;
  prac.loop = (mtype == practice_area_loop_countdown);
  prac.speedfactor = practicespeed;
  prac.looptime = stoptimecode - timecode;
  prac.viewtime = viewtime;
  prac.hifrequency = hif;
  prac.lofrequency = lowf;
  prac.instru.name = m_pcurrent_instrument->get_name();
  prac.instru.voice_index = m_pcurrent_instrument->identifier();
  command = coder.create_practice_message(&prac);
#ifdef _DEBUG
  printf("* Practice message\n");
#endif
  if (coder.build_wire_message(msg, cmsgsize, &size, command))
    {
      //printf("sending start command to practice:\"%s\"", command.c_str());
      m_pserver->Send_to_dialogs(message_send_practice, msg, size);
    }
}

void Cappdata::send_note_to_practice_view(CNote *pn, bool deletenote)
{
  scmsg::Cmessage_coding coder;
  string                 command;
  const int              cmsgsize = 4096;
  char                   msg[cmsgsize];
  int                    size;

  if (!m_pserver->is_practice_dialog_enable())
    return;
  command = coder.create_remadd_note_message(pn, deletenote);
  if (coder.build_wire_message(msg, cmsgsize, &size, command))
    {
#ifdef _DEBUG
      printf("sending note rem add command to practice:%s", command.c_str());
#endif
      if (deletenote)
	m_pserver->Send_to_dialogs(message_send_remadd_note, msg, size);
      else
	m_pserver->Send_to_dialogs_with_delay(message_send_remadd_note, msg, size);
    }
}

void Cappdata::send_note_highlight(int note_id)
{
  scmsg::Cmessage_coding coder;
  string                 command;
  const int              cmsgsize = 4096;
  char                   msg[cmsgsize];
  int                    size;

  if (!m_pserver->is_practice_dialog_enable())
    return;
  command = coder.create_note_highlight_message(note_id);
  if (coder.build_wire_message(msg, cmsgsize, &size, command))
    {
#ifdef _DEBUG
      printf("sending note highlight command to practice:%s", command.c_str());
#endif
      m_pserver->Send_to_dialogs_with_delay(message_note_highlight, msg, size);
    }
}

void Cappdata::send_measure_to_practice_view(CMesure *pm, bool deletemeasure)
{
  scmsg::Cmessage_coding coder;
  string                 command;
  const int              cmsgsize = 4096;
  char                   msg[cmsgsize];
  int                    size;

  if (!m_pserver->is_practice_dialog_enable())
    return;
  command = coder.create_remadd_measure_message(pm, deletemeasure);
  if (coder.build_wire_message(msg, cmsgsize, &size, command))
    {
#ifdef _DEBUG
      printf("sending measure rem add command to practice:%s", command.c_str());
#endif
      m_pserver->Send_to_dialogs_with_delay(message_send_remadd_measure, msg, size);
    }
}

void Cappdata::update_practice_view()
{
  t_edit_state* pe;
  std::list<CNote*>::iterator   iter;
  std::list<CMesure*>::iterator miter;
  bool                          bpractice_enabled;

  bpractice_enabled = m_pserver->is_practice_dialog_enable();
  pe = m_pScoreEdit->get_edit_state();
  if (pe != NULL)
    {
      // Note changes
      if (pe->changed_notes_list.size())
	{
	  pe->changed_notes_list.sort();
	  pe->changed_notes_list.unique();
	  iter = pe->changed_notes_list.begin();
	  while (iter != pe->changed_notes_list.end())
	    {
	      //printf (" pointer = %x\n", (*iter)); 
	      assert(((*iter)->m_freq_list.size() > 0));
		{
		  if (bpractice_enabled)
		    send_note_to_practice_view(*iter, false);
		  iter = pe->changed_notes_list.erase(iter);
		}
	    }
	}
      // Changing the current note and then delete it because it has been inserted in a chord, gives
      // a segfault if deleted first.
      if (pe->deleted_notes_list.size())
	{
	  pe->deleted_notes_list.sort();
	  pe->deleted_notes_list.unique();
	  iter = pe->deleted_notes_list.begin();
	  while (iter != pe->deleted_notes_list.end())
	    {
	      assert((*iter)->m_freq_list.size() > 0);
	      if (bpractice_enabled)
		send_note_to_practice_view(*iter, true);
	      m_pcurrent_instrument->delete_note(*iter);
	      iter = pe->deleted_notes_list.erase(iter);
	    }
	}
      // Measure changes
      if (pe->Measure_delete_list.size())
	{
	  miter = pe->Measure_delete_list.begin();
	  while (miter != pe->Measure_delete_list.end())
	    {
	      if (bpractice_enabled)
		send_measure_to_practice_view(*miter, true);
	      delete(*miter);
	      miter = pe->Measure_delete_list.erase(miter);
	    }
	}
      if (pe->Measure_add_list.size())
	{
	  miter = pe->Measure_add_list.begin();
	  while (miter != pe->Measure_add_list.end())
	    {
	      if (bpractice_enabled)
		send_measure_to_practice_view(*miter, false);
	      delete(*miter);
	      miter = pe->Measure_add_list.erase(miter);
	    }
	}
    }
}

// ------------------------------------------------------------------------------------------------------------------------------

void Cappdata::update_practice_stop_time(bool reload)
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;
  double    tref, viewtime, practicespeed;
  double    tracklen;
  int       mtype;

  LOCK;
  viewtime = pshared_data->viewtime;
  tref = pshared_data->timecode - viewtime + (viewtime / 4.); // Do not show the notes from the key of it will be difficult to edit the first note
  tref = tref < 0? pshared_data->timecode : tref;
  tracklen = pshared_data->trackend;
  practicespeed = pshared_data->practicespeed;
  UNLOCK;
  mtype = reload? practice_update : practice_stop_at;
  send_practice_message(mtype, tref, tracklen, F_MAX, F_BASE, viewtime, practicespeed);
}

void Cappdata::printfscore()
{
  CInstrument* pins;

  pins = m_pscore->get_first_instrument();
  while (pins != NULL)
    {
      printf("instrument name=%s\n", pins->get_name().c_str());
      printf("instrument notes=%d\n", pins->note_count());
      pins = m_pscore->get_next_instrument();
    }
}

// Handles the messages from the network to the main app
void Cappdata::process_network_messages()
{
  t_shared *pshared_data = (t_shared*)&m_shared_data;
  scmsg::Cmessage_coding coder;
  t_internal_message     msg;
  int                    cur_size;
  int                    msg_size;
  char                  *msg_ptr;

  while (m_pserver->pop_message(&msg))
    {
#ifdef _DEBUG
      printf("Network message poped in user event processing\n");
      printf("message=\"%s\"\n", msg.data);
#endif
      // Decode the message
      msg_ptr = msg.data;
      msg_size = msg.size;
      while ((cur_size = coder.get_next_wire_message(msg_ptr, msg_size)) > 0) // Many messages in one buffer reception
	{
	  msg_ptr += cur_size;
	  msg_size -= cur_size;
	  switch (coder.message_type())
	    {
	    case network_message_dialog_opened:
	      {
#ifdef _DEBUG
		printf("Dialog openened\n");
#endif
		// Update the opened dialog box type in the server connection list
		m_pserver->register_dialog_connection(&msg, coder.app_code());
		switch (coder.app_code())
		  {
		  case add_instrument_dialog:
		    {
		      send_instrument_list();
		    }
		    break;
		  case config_dialog:
		    {
		      send_apidevice_list();
		      send_configuration();
		    }
		    break;
		  case practice_dialog:
		    {
		      update_practice_stop_time(true);
		    }
		    break;
		  default:
		    break;
		  }
	      }
	      break;
	    case network_message_closed_dialog:
	      {
#ifdef _DEBUG
		printf("Dialog closed\n");
#endif
		// Connection status done in the server thread
		// Flip the coresponding card to disabled
		switch (coder.app_code())
		    {
		    case load_save_dialog:
		      toggle_card(CARD_OPENSAVE);
		      break;
		    case add_instrument_dialog:
		      toggle_card(CARD_ADDINSTR);
		      break;
		    case practice_dialog:
		      {
			Ccard *pc;
			
			toggle_card(CARD_PLACEMENT);
			pc = m_cardlayout->get_card((char*)CARD_PRACTICE);
			if (pc != NULL)
			  {
			    LOCK;
			    // If it is recording then let it record
			    if (pshared_data->play_State == state_practice || pshared_data->play_State == state_practiceloop)
			      play_practice_enabled_in_locked_area(false); // FIXME should be a mesaging system
			    UNLOCK;
			    //pc->activate_card(false);
			  }
		      }
		      break;
		    case config_dialog:
		      toggle_card(CARD_CONFIG);
		      break;
		    default:
		      break;
		  };
		//m_pserver->unregister_dialog_connection(&msg, xmldeco.app_code());
	      }
	      break;
	    case network_message_file:
	      {
		string                path, file_name;
		scmsg::efileOperation fo;
		
		if (!coder.get_fileIO_message(&path, &file_name, &fo))
		  break;
		switch (fo)
		  {
		  case scmsg::efileopen:
		    {
		      // Open the score and sound file
		      load_score(path + file_name);
		      set_score_renderer(m_pcurrent_instrument->get_name());
		      update_practice_stop_time(true);
		    }
		    break;
		  case scmsg::efilesave:
		    {
		      // Save the score and the sound
		      save_score(path + file_name);
		    }
		    break;
		  case scmsg::enewfile:
		    {
		      // Clear all the score and sound data
#ifdef _DEBUG
		      printf("New project, clearing everything.\n");
#endif
		      clear_sound();
		      delete m_pscore;
		      m_pscore = new CScore(std::string("violin"));
		      m_pcurrent_instrument = m_pscore->get_first_instrument();
		      set_score_renderer(m_pcurrent_instrument->get_name());
		      update_practice_stop_time(true);
		      printfscore();
		    }
		    break;
		  case scmsg::enewfromfile:
		    {
		      // Clear all the score and sound data
#ifdef _DEBUG
		      printf("New project from a sound file, clearing everything.\n");
#endif
		      clear_sound();
		      delete m_pscore;
		      m_pscore = new CScore(std::string("violin"));
		      m_pcurrent_instrument = m_pscore->get_first_instrument();
		      set_score_renderer(m_pcurrent_instrument->get_name());
		      if (load_sound_buffer(path + file_name))
			{
			  printf("Critical error loading the sound file \"%s\".\n", file_name.c_str());
			}
		      update_practice_stop_time(true);
		      printfscore();
		    }
		    break;
		  default:
		    break;
		  }
	      }
	      break;
	    case network_message_remadd_instrument:
	      {
		scmsg::t_instrument instru;
		bool                bdelete;
	
#ifdef _DEBUG	
		printf("Received remadinstrument message\n");
#endif
		if (!coder.get_remadd_instrument(&instru, &bdelete))
		  break;
		if (bdelete)
		  {
		    // Remove an instrument tab
		    CInstrument* pins;
		    CInstrument* pold;

		    pins = m_pscore->get_first_instrument();
		    pold = NULL;
		    while (pins != NULL)
		      {
			if (pins->get_name() == instru.name &&
			    pins->identifier() == instru.voice_index)
			  {
			    pold = pins;
			  }
			pins = m_pscore->get_next_instrument();
		      }
		    if (pold != NULL)
		      {
			//printf("deleteing %s - %d.\n", instru.name.c_str(), instru.voice_index);
			pins = m_pscore->remove_instrument(pold);
			assert(m_pscore->instrument_number() > 0);
			if (pold == m_pcurrent_instrument)
			  m_pcurrent_instrument = m_pscore->get_first_instrument();
			delete pold;
		      }
		  }
		else
		  {
		    // Add an instrument tab
		    CInstrument *pins;
		    
		    if (m_pscore->instrument_number() >= MAX_INSTRUMENTS)
		      break;
		    pins = new CInstrument(instru.name);
		    m_pscore->add_instrument(pins); // Gives an identifier to the instrument
		  }
		set_score_renderer(m_pcurrent_instrument->get_name());
	      }
	      break;
	    case network_message_pa_dev_selection:
	      {
		std::list<t_portaudio_api> outapis;
		t_channel_select_strings   chs;

		if (coder.get_audioIO_config(&outapis, &chs))
		  reopen_stream(&chs);
	      }
	      break;
	    case network_message_configuration:
	      {
		scmsg::t_appconfig outcfg;
		
		if (coder.get_config(&outcfg))
		  {
		    m_brecord_at_start = outcfg.recordAtStart;
		    m_bdonotappendtoopen = outcfg.doNotChangeOpenedFiles;
#ifdef _DEBUG
		    printf("Configuration received start=%d donotappend=%d\n", m_brecord_at_start, m_bdonotappendtoopen);
#endif
		  }
	      }
	      break;
	    default:
	      break;
	    };
	}
    }
}

void Cappdata::process_audio_events()
{
  t_shared                              *pshared_data = &m_shared_data;
  std::list<t_audio_track_cmd>::iterator iter;

  LOCK;
  if (pshared_data->audio_cmds.size() > 0)
    {
      iter = pshared_data->audio_cmds.begin();
      while (iter != pshared_data->audio_cmds.end())
	{
	  if ((*iter).direction == ad_thread2app)
	    {
	      switch ((*iter).command)
		{
		case ac_stop:
		  {
		    // Flip the practice card if needed FIXME shoudl be a message to itself somewhere else
		    play_practice_enabled_in_locked_area(false);
		  }
		  break;
		case ac_practice_loop:
		  {
		    // Restart on the selected practice area
		    reset_filter_on_loop_commands(&m_note_selection);
		  }
		  break;
		default:
		  printf("Critical error: unknown internal audio cmd.\n");
		  exit(EXIT_FAILURE);
		  break;
		}
	      iter = pshared_data->audio_cmds.erase(iter);
	    }
	  else
	    iter++;
	}
    }
  UNLOCK;
}

