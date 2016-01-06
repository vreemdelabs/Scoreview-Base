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

#include <string>

#include <pthread.h>

#include <event2/dns.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/event.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_File_Icon.H>
#include <FL/Fl_Native_File_Chooser.H>

#include "tcpclient.h"

#include "main.h"

int g_thrlisteningretval;

void* thr_listeningclient(void *p_data)
{
  t_app_data* papp = (t_app_data*)p_data;

  g_thrlisteningretval = 1;
  // It should be stuck here until it is broken
  papp->ptcpclient->EventLoop();
  printf("Dialog: returned from the client loop\n");
  g_thrlisteningretval = 0;
  pthread_exit(&g_thrlisteningretval);
  return NULL;
}

int create_threads(t_app_data *app)
{
  int ret;

  try
    {
      //---------------------------------------------------
      // Server listening to incoming data thread
      //---------------------------------------------------
      ret = pthread_create(&app->threadclient, NULL,
			   thr_listeningclient, (void*)app);
      if (ret)
	throw (ret);
    }
  catch (int e)
    {
      fprintf(stderr, "%s", strerror(e));
      return 1;
    }
  return 0;
}

int release_threads(t_app_data *papp)
{
  int *pretval;

  //---------------------------------------------------
  // Dialog start/stop thread (uses libevent messaging)
  //---------------------------------------------------
  pthread_join(papp->threadclient, (void**)&pretval);
  if (*pretval)
    printf("Something in the Network code failed: %d\n", *pretval);
  return 0;
}

