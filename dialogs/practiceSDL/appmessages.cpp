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
#include <vector>
#include <list>
#include <unistd.h>

#include <sys/time.h>

#include <errno.h>
#ifdef __LINUX
//#include <sys/types.h>
//#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include <syslog.h>
//#include <signal.h>
#else
#include <winsock2.h>
#include <windows.h>
#endif //__LINUX

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <GL/gl.h>

#include <tinyxml.h>

#include "audioapi.h"
#include "score.h"
#include "messages.h"
#include "message_decoder.h"
#include "gfxareas.h"
#include "mesh.h"
#include "gl2Dprimitives.h"
#include "pictures.h"
#include "f2n.h"
#include "keypress.h"
#include "tcpclient.h"
#include "shared.h"
#include "fingerender.h"
#include "prenderer.h"
#include "vrender.h"
#include "grender.h"
#include "app.h"
#include "sdlcalls.h"

using namespace std;
using namespace scmsg;

void Cappdata::select_instrument_renderer(string instrument_name)
{
  m_prenderer = m_null_renderer;
  if (instrument_name == string("violin"))
    {
      printf("instrument choice: violin\n");
      m_prenderer = m_violin_renderer;
    }
  if (instrument_name == string("piano"))
    {
      printf("instrument choice: piano\n");
      m_prenderer = m_piano_renderer;
    }
  if (instrument_name == string("guitar"))
    {
      printf("instrument choice: guitar\n");
      m_prenderer = m_guitar_renderer;
    }
}

void Cappdata::decode_message_data(t_internal_message *pmsg)
{
  Cmessage_coding      coder;
  int                  datasize, readsize;
  char                *pdata;

  // Decode the message buffer
  pdata = pmsg->data;
  datasize = pmsg->size;
  while (datasize && ((readsize = coder.get_next_wire_message(pdata, datasize)) != 0))
    {
      switch (coder.message_type())
	{
	case network_message_score_transfer:
	  {
	    printf("Practice: Score reception.\n");
	    if (m_pscore != NULL)
	      {
		delete m_pscore;
	      }
	    m_pscore = new CScore();
	    if (!coder.get_score_message(m_pscore))
	      {
		printf("Warning: Score message transfer failed.\n");
	      }
	  }
	  break;
	  // Notes by groups of 40.
	case network_message_note_transfer:
	  {
	    printf("Practice: Note reception.\n");
	    if (m_pscore != NULL)
	      {
		if (!coder.get_notelist_message(m_pscore))
		  {
		    printf("Warning: Note message transfer failed.\n");
		  }
#ifdef _DEBUG
		else
		  printf("Note message transfer received.\n");
#endif
	      }
	  }
	  break;
	case network_message_practice:
	  {
	    t_practice_params       prac;

	    printf("Practice message: \n");
	    if (coder.get_practice(&prac))
	    {
	      if (prac.start_practice)
		{
		  //printf("Daytime=%f\n", prac.countdownDaytime);
		  if (prac.countdownDaytime > -1)
		    {
		      printf("Receiving countdown reference of %f\n", prac.countdownDaytime);
		      m_start_daytime = prac.countdownDaytime;
		      m_state = state_countdown;
		    }
		  else
		    m_state = state_playing;
		}
	      else
		{
		  if (prac.countdownDaytime > -1)
		    {
		      printf("Receiving ONNOONLLLY countdown reference of %f\n", prac.countdownDaytime);
		      m_start_daytime = prac.countdownDaytime;
		      m_state = state_only_countdown;
		    }
		  else
		    m_state = state_waiting;
		}
	      printf("Daytime is= %f.\n", prac.countdownDaytime);
	      m_play_timecode = prac.curtimecode;
	      m_play_end_timecode = prac.curtimecode + prac.looptime;
	      m_practicespeed = prac.speedfactor;
	      //printf("start stop %f %f\n", m_play_timecode, m_play_end_timecode);
	      m_lof = prac.lofrequency;
	      m_hif = prac.hifrequency;
	      //printf("lo hi frequency %f %f\n", m_lof, m_hif);
	      m_instrument = prac.instru.name;
	      m_instrument_identifier = prac.instru.voice_index;
	      /*
	      if (prac.reload) // Useless, done in network_message_score_transfer
	      {
	      }
	      */
	      // Find the current instrument
	      if (m_pscore != NULL)
		{
		  m_pcurrent_instrument = m_pscore->get_instrument(m_instrument, m_instrument_identifier);
		  if (m_pcurrent_instrument == NULL)
		    m_pcurrent_instrument = m_pscore->get_first_instrument();
		  if (m_pcurrent_instrument == NULL)
		    {
		      printf("Practice error: instrument missing on practice message.\n");
		      exit(1);
		    }
		  if (m_pcurrent_instrument->get_name() != m_instrument &&
		      m_pcurrent_instrument->identifier() != prac.instru.voice_index)
		    {
		      printf("Error: Instrument not found.\n");
		    }
		  select_instrument_renderer(m_instrument);
		  m_prenderer->set_viewtime(prac.viewtime);
		}
	    }
	  }
	  break;
	  // The score basic info like measures and empty instruments come first
	case network_message_remadd_note:
	  {
	    bool   bdelete;
	    CNote *pn;
	    CNote  in_note;
	    bool   bfound;

	    coder.get_remadd_note(&in_note, &bdelete);
	    if (bdelete)
	      {
		pn = m_pcurrent_instrument->get_first_note(0.);
		while (pn != NULL)
		  {
		    if (in_note.identifier() == pn->identifier())
		      {
			//printf("Deleting the note with dientifier=%d.\n", pn->identifier());
			m_pcurrent_instrument->delete_note(pn);
			break ;
		      }
		    pn = m_pcurrent_instrument->get_next_note();
		  }
	      }
	    else
	      {
		bfound = false;
		pn = m_pcurrent_instrument->get_first_note(0.);
		while (pn != NULL)
		  {
		    if (in_note.identifier() == pn->identifier())
		      {
			bfound = true;
			*pn = in_note;
			//printf("Changed the note with dientifier=%d.\n", pn->identifier());
			break ;
		      }
		    pn = m_pcurrent_instrument->get_next_note();
		  }
		if (!bfound)
		  {
		    pn = new CNote(in_note);
		    printf("Adding a note with dientifier=%d.\n", (int)pn->identifier());
		    m_pcurrent_instrument->add_note(pn);
		  }
	      }
	  }
	  break;
	case network_message_remadd_measure:
	  {
	    
	  }
	  break;
	case network_message_close:
	  {
	    printf("Scoreview Practice: closing\n");
	    m_state = state_closed;
	    m_shared_data.bquit = true;
	    m_shared_data.ptcpclient->close_function(string(PRACTICE_DIALOG_CODENAME));
	  }
	  break;
	default:
	  break;
	}
      pdata    += readsize;
      datasize -= readsize;      
    }
}

void Cappdata::get_network_messages()
{
  pthread_mutex_lock(&m_shared_data.data_mutex);
  if (m_shared_data.network2app_message_list.size() > 0)
    {
      decode_message_data(&(*m_shared_data.network2app_message_list.begin()));
      m_shared_data.network2app_message_list.pop_front();
    }
  pthread_mutex_unlock(&m_shared_data.data_mutex);
}

