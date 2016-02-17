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
#include <list>
#include <vector>
#include <string>

#include <errno.h>
#ifdef __LINUX
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#include <winsock2.h>
#include <windows.h>
#endif //__LINUX

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_File_Icon.H>
//#include <FL/Fl_Native_File_Chooser.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Check_Button.H>

#include <event2/dns.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/thread.h>
#include <event2/listener.h>

#include <tinyxml.h>

#include "audioapi.h"
#include "score.h"
#include "messages.h"
#include "message_decoder.h"
#include "tcpclient.h"
#include "main.h"

using namespace std;
using namespace scmsg;

//-------------------------------------------------------------------------------
// Callbacks
//-------------------------------------------------------------------------------

void wakeup_main(t_app_data *papp_data)
{
  pthread_mutex_lock(&papp_data->cond_devlistmutex);
  pthread_cond_signal(&papp_data->cond_devlist);
  pthread_mutex_unlock(&papp_data->cond_devlistmutex);
}

// Used to receive the messages ping and close from the server
void read_cb(struct bufferevent *pbev, void *pappdata)
{
  t_app_data          *papp = (t_app_data*)pappdata;
  struct evbuffer     *input = bufferevent_get_input(pbev);
  size_t               len = evbuffer_get_length(input);
  char                 data[MESSAGE_CONTENT_SIZE];
  char                *pdata;
  int                  datasize, readsize;
  Cmessage_coding      coder;

  if (len >= MESSAGE_CONTENT_SIZE)
    return ;
  evbuffer_remove(input, data, len);
  data[len] = 0;
  //printf("received buffer \"\"%s\"\"\n", data);
  // Analyse the data
  pdata = data;
  datasize = len;
  while (datasize && ((readsize = coder.get_next_wire_message(pdata, datasize)) != 0))
    {
      switch (coder.message_type())
	{
	case network_message_pa_dev_selection:
	  {
	    coder.get_audioIO_config(&papp->pa_dev_list, &papp->pa_chan_sel);
	    printf("received channel list\n");
	  }
	  break;
	case network_message_configuration:
	  {
	    t_appconfig cfg;

	    coder.get_config(&cfg);
	    papp->bstartrecord = cfg.recordAtStart;
	    papp->bnotappendtoopen = cfg.doNotChangeOpenedFiles;
	    printf("received config %d %d\n", papp->bstartrecord, papp->bnotappendtoopen);
	    wakeup_main(papp); // Here because the2 mus be received and general config is sent after audio api IO config
	  }
	  break;
	case network_message_close:
	  {
	    printf("Config app: closing\n");
	    close_function(papp);
	  }
	  break;
	default:
	  printf("Dialog has received a unknown message:%s\n", data);
	};
      pdata    += readsize;
      datasize -= readsize;
    }
}

// Called when the data was sent, close the conection here
void write_cb(struct bufferevent *pbev, void *pappdata)
{
  t_app_data      *papp = (t_app_data*)pappdata;
  CTCP_Client     *pclient = papp->ptcpclient;

  //printf("write ready\n");
  // Check if it was the last message
  if (pclient->check_closing_state())
    {
      //printf("close\n");
      pclient->set_to_closed_state();
      pclient->Exit_EventLoop();
    }
  else
    pclient->set_state(state_ready_for_writing);
}

void event_cb(struct bufferevent *bev, short events, void *pappdata)
{
  t_app_data      *papp = (t_app_data*)pappdata;

  if (events & BEV_EVENT_ERROR)
    {
      perror("Dialog: Error from bufferevent");
      if (papp->pwindow)
	papp->pwindow->hide(); // Close the window
      else
	{
	  papp->bconnected = false;
	  wakeup_main(papp);
	}
    }
  if (events & BEV_EVENT_EOF)
    {
      printf("Dialog: Lib Event callback: eof\n");
      //bufferevent_free(bev);
      wakeup_main(papp);
    }
}
