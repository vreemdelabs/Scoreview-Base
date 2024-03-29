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
#include <list>
#include <string>

#include <pthread.h>

#ifdef __LINUX
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#ifdef __ANDROID__
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#else
#include <winsock2.h>
#include <windows.h>
#endif //__ANDROID__
#endif //__LINUX

#ifdef __ANDROID__
#include <SDL_net.h>
#else
#include <SDL2/SDL_net.h>
#endif

#include "messages.h"
#include "tcp_message_receiver.h"
#include "tcpclientSDLnet.h"
#include "shared.h"

#include "main.h"

static int g_thrlisteningretval;

void* thr_listeningclient(void *p_data)
{
  t_shared_data* papp = (t_shared_data*)p_data;

  g_thrlisteningretval = 0;
  // It should be stuck here until it is broken
  papp->ptcpclient->EventLoop();
  printf("Dialog: returned from the client loop\n");
  pthread_exit(&g_thrlisteningretval);
  return NULL;
}

int create_threads(t_shared_data *app)
{
  int ret;

  //---------------------------------------------------
  // Server listening to incoming data thread
  //---------------------------------------------------
  ret = pthread_create(&app->threadclient, NULL,
		       thr_listeningclient, (void*)app);
  if (ret)
    {
      fprintf(stderr, "%s", strerror(ret));
      return 1;
    }
  return 0;
}

int release_threads(t_shared_data *papp)
{
  int*         pretval;

  //---------------------------------------------------
  // Dialog start/stop thread (uses libevent messaging)
  //---------------------------------------------------
  pthread_join(papp->threadclient, (void**)&pretval);
  if (*pretval)
    printf("Something in the Network code failed %d\n", *pretval);
  return 0;
}

