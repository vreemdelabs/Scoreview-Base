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
#include <unistd.h>

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
#include "audioapi.h"
#include "score.h"
#include "messages.h"
#include "message_decoder.h"
#include "callbacks.h"
#include "tcp_ip.h"
#include "tcp_message_receiver.h"
#include "tcpclientSDLnet.h"

using namespace std;

//-------------------------------------------------------------------------------
// Networking with the SDL (because it is easier to compile on android than libevent)
//-------------------------------------------------------------------------------

CTCP_Client::CTCP_Client(void *papp_data, std::string address, int port)
{
  m_port = port;
  m_pappdata = papp_data;
  //m_plisten_socket = new sockaddr_in();
  m_bopened = false;
}

CTCP_Client::~CTCP_Client()
{
  if (m_bopened)
    {
      SDLNet_FreeSocketSet(m_socketset);
      SDLNet_TCP_Close(m_sd);
    }
  SDLNet_Quit();
}

bool CTCP_Client::Server_init()
{ 
  const char *host;             // Host name

  if (SDLNet_Init() < 0)
    {
#ifndef __ANDROID__
      fprintf(stderr, "SDLNet_Init: %s\n", SDLNet_GetError());
#endif
      exit(EXIT_FAILURE);
    }
  m_ip.host = inet_addr(SERVER_HOST);
  m_ip.port = SCOREVIEW_PORT;
  //
  if (!(host = SDLNet_ResolveIP(&m_ip)))
    {
#ifndef __ANDROID__
      printf("SDLNet_ResolveIP: %s\n", SDLNet_GetError());
#endif
      exit(1);
    }
  // Resolve the host we are connecting to
  if (SDLNet_ResolveHost(&m_ip, host, SCOREVIEW_PORT) < 0)
    {
#ifndef __ANDROID__
      fprintf(stderr, "SDLNet_ResolveHost: %s\n", SDLNet_GetError());
#endif
      exit(EXIT_FAILURE);
    }
  return true;
}

bool CTCP_Client::Connect()
{
  m_bopened = false;
  // Open a connection with the IP provided (listen on the host's port)
  if (!(m_sd = SDLNet_TCP_Open(&m_ip)))
    {
      printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
      ///exit(EXIT_FAILURE);
    }
  else
    {
      m_socketset = SDLNet_AllocSocketSet(1);
      if (SDLNet_TCP_AddSocket(m_socketset, m_sd) == -1)
	{
	  printf("SDLNet_TCP_AddSocket: %s\n", SDLNet_GetError());
	}
      else
	m_bopened = true;
    }
  return m_bopened;
}

bool CTCP_Client::Send(const void *pbuffer, int length)
{
  if (SDLNet_TCP_Send(m_sd, pbuffer, length) < length)
    {
      printf("SDLNet_TCP_Send Error: %s\n", SDLNet_GetError());
    }
  return true;
}

void CTCP_Client::EventLoop()
{
  int  numready;
  int  timeout_ms;
  int  size;
  char buffer[MESSAGE_CONTENT_SIZE];
  int  message_size;

  timeout_ms = 200;
  // Loop until it must exit
  while (!check_closing_state())
    {
      numready = SDLNet_CheckSockets(m_socketset, timeout_ms);
      // Someting ready to be read
      if ((numready > 0) && SDLNet_SocketReady(m_sd))
	{
	  printf("socket ready\n");
	  size = SDLNet_TCP_Recv(m_sd, buffer, MESSAGE_CONTENT_SIZE);
	  if (size > 0)
	    {
	      printf("socket receivedata\n");
	      m_message.add_data(buffer, size);
	      while (m_message.available_message())
		{
		  printf("full message\n");
		  m_message.get_message(buffer, MESSAGE_CONTENT_SIZE, &message_size);
		  if (message_size > 0)
		    read_cb(buffer, message_size, m_pappdata);
		}
	    }
	}
      if (numready == -1)
	{
	  // Something wrong happened
	  printf("Practice: socket error.\n");
	  exit(EXIT_FAILURE);
	}
    }
}

void CTCP_Client::Exit_EventLoop()
{
  set_to_closed_state();
}

bool CTCP_Client::check_closing_state()
{
  //printf("state=%d\n", m_state);
  return (m_state == state_closing);
}

void CTCP_Client::set_to_closed_state()
{
  m_state = state_closed;
}

client_state CTCP_Client::get_state()
{
  return m_state;
}

void CTCP_Client::set_state(client_state state)
{
  //printf("setstate=%d\n", m_state);
  m_state = state;
}

using namespace scmsg;

void CTCP_Client::send_dialog_opened_message(string dialog_id)
{
  string          str;
  Cmessage_coding coder;
  int             size;
  const int       cmsgsize = 4096;
  char            msg[cmsgsize];

  str = coder.create_dialog_opened_message(dialog_id);
  if (coder.build_wire_message(msg, cmsgsize, &size, str))
    {
      Send(msg, size);
      sleep(1);
    }
}

void CTCP_Client::close_function(string dialog_id)
{
  string          str;
  Cmessage_coding coder;
  int             size;
  const int       cmsgsize = 4096;
  char            msg[cmsgsize];

  if (m_state == state_closed ||
      m_state == state_closing ||
      !m_bopened)
    return ;
  str = coder.create_dialog_closed_message(dialog_id);
  set_state(state_closing);
  if (coder.build_wire_message(msg, cmsgsize, &size, str))
    {
      Send(msg, size);
      sleep(1);
    }
}

