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
#include <unistd.h>

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
#include "audioapi.h"
#include "score.h"
#include "messages.h"
#include "message_decoder.h"
#include "tcp_ip.h"
#include "tcpclient.h"

using namespace std;

//-------------------------------------------------------------------------------
// Libevent
//-------------------------------------------------------------------------------

CTCP_Client::CTCP_Client(void *papp_data, std::string address, int port)
{
  m_port = port;
  m_pappdata = papp_data;
  m_plisten_socket = new sockaddr_in();
  m_pbuffer_event = NULL;
  m_bopened = false;
}

CTCP_Client::~CTCP_Client()
{
  if (m_bopened)
  bufferevent_free(m_pbuffer_event);
  delete m_plisten_socket;
}

bool CTCP_Client::Server_init()
{ 
#ifdef __WINDOWS
  WSADATA wsaData;
  int     code;

  code = WSAStartup(0x0202, &wsaData);
  if (code != 0)
    {
      printf("ERROR: Windows sockets initialization failure");
      switch (code)
	{
	case WSASYSNOTREADY:
	  printf(": the underlying network subsystem is not ready for network communication.\n");
	  break;
	case WSAEINPROGRESS:
	  printf(": this socket version (2.2) is not provided\n");
	  break;
	case WSAEPROCLIM:
	  printf(": blocking 1.1 socket in progress\n");
	  break;
	default:
	  printf(": unknown error\n");
	  break;
	}
      WSACleanup();
      return false;
    }
#endif
  // Important, because the class instance is used both in the main app thread, and in the server thread
#ifdef __LINUX
  if (evthread_use_pthreads() < 0)
#else
  if (evthread_use_windows_threads() < 0)
#endif
    {
      printf("Libevent could not initialise it's thread safe components.\n");
      return false;
    }
  // Create new event base
  memset(m_plisten_socket, 0, sizeof(struct sockaddr_in));
  m_plisten_socket->sin_family = AF_INET;
  m_plisten_socket->sin_addr.s_addr = inet_addr(SERVER_HOST);
  m_plisten_socket->sin_port = m_port;
  m_pbase = event_base_new();
  m_pbuffer_event = bufferevent_socket_new(m_pbase, -1, BEV_OPT_CLOSE_ON_FREE);
  // Set callbacks
  bufferevent_setcb(m_pbuffer_event, read_cb, write_cb, event_cb, m_pappdata);
  bufferevent_enable(m_pbuffer_event, EV_READ|EV_WRITE);
  m_state = state_ready_for_writing;
  return true;
}

bool CTCP_Client::Connect()
{
  if (bufferevent_socket_connect(m_pbuffer_event,
				 (struct sockaddr*)m_plisten_socket,
				 sizeof(struct sockaddr)) < 0)
    m_bopened = false;
  else
    m_bopened = true;
  return m_bopened;
}

struct event_base* CTCP_Client::get_base_evt()
{
  return m_pbase;
}

bool CTCP_Client::wait_for_write_completion()
{
  int i;

  // Wait for the write completion callback
  i = 0;
  while (m_state == state_writing_message && i < 200)
    {
      usleep(10000);
      i++;
    }
  return (m_state == state_writing_message);
}

bool CTCP_Client::Send(const void *pbuffer, int length)
{
  wait_for_write_completion();
  if (m_state == state_writing_message)
    printf("Dialog: Warning sending over last message!\n");
  //printf("Dialog sending:\n%s\n", (char*)pbuffer);
  //printf("Message size is:%d\n", length);
  if (!check_closing_state())
    set_state(state_writing_message);
  bufferevent_write(m_pbuffer_event, pbuffer, length);
  //event_base_dispatch(m_pbase);
  return true;
}

void CTCP_Client::EventLoop()
{
  // A callback or a button will break the loop
  event_base_dispatch(m_pbase);
}

void CTCP_Client::Exit_EventLoop()
{
  // This will make Server_loop(); return
  if (event_base_loopbreak(m_pbase) == -1)
    printf("Critical error in a libevent call: loppbreak failed.\n");
}

bool CTCP_Client::check_waiting_connection_state()
{
  return (m_state == state_wainting_connection);
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
      wait_for_write_completion(); // And then quit
    }
}

