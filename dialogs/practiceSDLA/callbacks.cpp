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

// common includes
#include "messages.h"
#include "tcp_message_receiver.h"
#include "tcpclientSDLnet.h"
#include "shared.h"
#include "main.h"

//-------------------------------------------------------------------------------
// Callbacks
//-------------------------------------------------------------------------------

// Used to receive the messages ping and close from the server
void read_cb(char *pbuffer, int buffer_size, void *pappdata)
{
  t_shared_data       *papp = (t_shared_data*)pappdata;
  t_internal_message   imsg;

  imsg.size = buffer_size;
  memcpy(imsg.data, pbuffer, buffer_size);
  imsg.data[imsg.size] = 0;
#ifdef _DEBUG
  printf("Practice dialog received:\n%s\n", imsg.data);
#endif
  pthread_mutex_lock(&papp->data_mutex);
  papp->network2app_message_list.push_back(imsg);
  pthread_mutex_unlock(&papp->data_mutex);
}
