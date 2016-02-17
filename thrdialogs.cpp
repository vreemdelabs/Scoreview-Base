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
#include <math.h>
#include <unistd.h>
#include <iterator>
#include <list>

#include <pthread.h>

#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include "messages.h"
#include "tcp_message_receiver.h"
#include "messages_network.h"
#include "thrdialogs.h"

//---------------------------------------------------------------------------------------
// This requires explaining
//
// The main app event loop sends events using libevent to wake up the dialog thread.
// The dialog thread is executing the event_base_loop. In this thread, application dialog
// boxes apps are started.
// Those dialogs connect to the event_base_loop via TCP/IP. The loop then responds to 
// network xml coded events and in it's turn sends SDL_user_events to the main app SDLevent
// handling.
// (the first idea was to get rid of QT in the main app, but the dialogs use FLTK now)
// The main app then catches the SDL user events and as instance will save the score or load
// or disable the card wich started the dialog box.
//
// Note that this thread only does message sorting and asynchronous data dispatching.
// All the shit is done in the main app.
//
//---------------------------------------------------------------------------------------

int g_thrdialogsretval;

void* thr_dialogs(void *p_data)
{
  CnetworkMessaging* pserver = (CnetworkMessaging*)p_data;

  g_thrdialogsretval = 0;
#ifdef _DEBUG
  printf("Initialising the dialogs thread\n");
#endif
  // It should be stuck here until a quit event is revceived
  pserver->Server_loop();
#ifdef _DEBUG
  printf("Returned from the server loop\n");
#endif
  return &g_thrdialogsretval;
}


