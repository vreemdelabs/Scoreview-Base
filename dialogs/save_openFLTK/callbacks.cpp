/*
 Scoreview (R)
 Copyright (C) 2015-2016 Patrick Areny
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
#include <FL/Fl_Native_File_Chooser.H>

#include <event2/dns.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/event.h>
#include <event2/thread.h>
#include <event2/listener.h>

#include "messages.h"
#include "tcpclient.h"
#include "main.h"

using namespace std;

//-------------------------------------------------------------------------------
// Callbacks
//-------------------------------------------------------------------------------

// Used to receive the messages ping and close from the server
void read_cb(struct bufferevent *pbev, void *pappdata)
{
  t_app_data      *papp = (t_app_data*)pappdata;

  // The only message should be close...
  printf("Save/Load app: closing\n");
  close_function(papp);
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
      //printf("client closed\n");
      pclient->set_to_closed_state();
      pclient->Exit_EventLoop();
    }
  else
    pclient->set_state(state_ready_for_writing);
}

void event_cb(struct bufferevent *bev, short events, void *pappdata)
{
  //t_app_data      *papp = (t_app_data*)pappdata;

  if (events & BEV_EVENT_ERROR)
    {
      perror("Dialog: Error from bufferevent");
    }
  if (events & BEV_EVENT_EOF)
    {
      printf("Dialog: Lib Event callback: eof\n");
      //papp->pwindow->hide(); // Close the window
      //bufferevent_free(bev);
    }
}

