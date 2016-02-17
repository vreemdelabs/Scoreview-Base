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

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_File_Icon.H>
#include <FL/Fl_Native_File_Chooser.H>

#include <event2/dns.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/thread.h>
#include <event2/listener.h>

// common includes
#include "messages.h"
#include "tcpclient.h"
#include "shared.h"
#include "main.h"

using namespace std;

//-------------------------------------------------------------------------------
// Callbacks
//-------------------------------------------------------------------------------

// Used to receive the messages ping and close from the server
void read_cb(struct bufferevent *pbev, void *pappdata)
{
  t_shared_data       *papp = (t_shared_data*)pappdata;
  struct evbuffer     *input = bufferevent_get_input(pbev);
  t_internal_message   imsg;

  imsg.size = evbuffer_get_length(input);
  if (imsg.size >= MESSAGE_CONTENT_SIZE)
    {
      printf("Message overflows\n");
      exit(1);
    }
  evbuffer_remove(input, imsg.data, imsg.size);
  imsg.data[imsg.size] = 0;
#ifdef _DEBUG
  printf("Practice dialog received:\n%s\n", imsg.data);
#endif
  pthread_mutex_lock(&papp->data_mutex);
  papp->network2app_message_list.push_back(imsg);
  pthread_mutex_unlock(&papp->data_mutex);
}

// Called when the data was sent, close the conection here
void write_cb(struct bufferevent *pbev, void *pappdata)
{
  t_shared_data   *papp = (t_shared_data*)pappdata;
  CTCP_Client     *pclient = papp->ptcpclient;

  printf("write ready\n");
  // Check if it was the last message
  if (pclient->check_waiting_connection_state())
    {
      printf("ready to connect\n");
      pclient->send_dialog_opened_message(string(PRACTICE_DIALOG_CODENAME));
      pclient->set_state(state_ready_for_writing);
    }
  else
    {
      if (pclient->check_closing_state())
	{
	  pclient->set_to_closed_state();
	  pclient->Exit_EventLoop();
	}
      else
	pclient->set_state(state_ready_for_writing);
    }
}

void event_cb(struct bufferevent *bev, short events, void *pappdata)
{
  //t_shared_data      *papp = (t_shared_data*)pappdata;

  if (events & BEV_EVENT_ERROR)
    {
      perror("Dialog: Error from bufferevent");
    }
  if (events & BEV_EVENT_EOF)
    {
      printf("Dialog: Lib Event callback: eof\n");
    }
}
