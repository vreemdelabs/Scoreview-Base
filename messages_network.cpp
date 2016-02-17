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
#include <assert.h>
#include <string.h>
#include <iterator>
#include <list>

#include <errno.h>
#ifdef __LINUX
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <syslog.h>
#include <signal.h>
#else
#include <winsock2.h>
#include <windows.h>
#endif //__LINUX

#include <pthread.h>

#include <SDL2/SDL.h>

#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/event-config.h>
#include <event2/util.h>
#include <event2/thread.h>

#include "audioapi.h"
#include "score.h"
#include "dialogpath.h"
#include "tcp_ip.h"
#include "messages.h"
#include "message_decoder.h"
#include "tcp_message_receiver.h"
#include "messages_network.h"

using namespace std;

//-------------------------------------------------------------------------------------
//               Callbacks
//-------------------------------------------------------------------------------------

// This callback is invoked when there is data to read on bev.
static void network_read_cb(struct bufferevent *pbev, void *puser_data)
{
  struct evbuffer        *input = bufferevent_get_input(pbev);
  size_t                  len   = evbuffer_get_length(input);
  char                    buffer[MESSAGE_BUFFER_SIZE];
  t_internal_message      msg;
  Creceive_callback_data *pcd = (Creceive_callback_data*)puser_data;
  CnetworkMessaging      *pshared_mess = (CnetworkMessaging*)pcd->m_pmess;

  if (len > MESSAGE_BUFFER_SIZE)
    {
      printf("Server error: skiped a message, too big.\n");
      return ;
    }
#ifdef _DEBUG
  printf("Scoreview received netork data size=%d\n", (int)len);
#endif
  if (evbuffer_remove(input, buffer, len) == -1)
    {
      printf("Server warning: failed to recover a network message.\n");
      return ;
    }
  buffer[len] = 0;
  //
  pcd->m_message.add_data(buffer, len);
  // Get all the messages contained in the buffer
  while (pcd->m_message.available_message())
    {
      pcd->m_message.get_message(msg.data, MESSAGE_CONTENT_SIZE, &msg.size);
      if (msg.size)
	{
#ifdef _DEBUG
	  printf("we got some data: %d bytes\n", msg.size);
#endif
	  msg.message_code = -1;
	  msg.passociated_event = pbev;
	  pshared_mess->push_message(&msg);
	  pshared_mess->triger_the_sdl_user_event();
#ifdef _DEBUG
	  printf("message pushed\n");
#endif
	}
    }
}

static void network_write_cb(struct bufferevent *pbev, void *puser_data)
{
  Creceive_callback_data *pcd = (Creceive_callback_data*)puser_data;
  CnetworkMessaging      *pshared_mess = (CnetworkMessaging*)pcd->m_pmess;

  //printf("write buffer completed.\n");
#ifdef _DEBUG
  printf("BUFFER WRITE SET TO TRUE\n");
#endif
  pshared_mess->m_write_completed = true;
}

// This callback is called on non data-oriented events: errors, closed connection...
static void network_event_cb(struct bufferevent *bev, short events, void *puser_data)
{
  Creceive_callback_data *pcd = (Creceive_callback_data*)puser_data;
  CnetworkMessaging      *pshared_mess = (CnetworkMessaging*)pcd->m_pmess;

  if (events & BEV_EVENT_ERROR)
    perror("Error from bufferevent!");
  if (events & BEV_EVENT_EOF || events & BEV_EVENT_ERROR)
    {
      printf("Lib Event network event callback: connection closed (eof) or error\n");
      pshared_mess->lock_datapipe();
      pshared_mess->unregister_dialog_connection(bev);
      assert(bev != NULL);
      bufferevent_free(bev);
      pshared_mess->unlock_datapipe();
    }
}

// Listener connection callback called on a new connection
static void accept_conn_cb(struct evconnlistener *plistener,
			   evutil_socket_t fd,
        		   struct sockaddr *address,
			   int socklen,
			   void *puser_data)
{
  CnetworkMessaging      *pshared_mess = (CnetworkMessaging*)puser_data;
  struct event_base      *pbase;
  struct bufferevent     *pbev;
  t_connection            connection;
  Creceive_callback_data *pcd;

  pbase = evconnlistener_get_base(plistener);
  pbev  = bufferevent_socket_new(pbase, fd, BEV_OPT_CLOSE_ON_FREE);
  // Add to the active connection list
  connection.dialog = unknown_dialog;
  connection.pevent = pbev;
  pshared_mess->m_active_connections.push_back(connection);
  // Create the reception data structure
  pcd = new Creceive_callback_data(pshared_mess);
  pshared_mess->add_cb_data(pcd);
  // Set the callbacks
  bufferevent_setcb(pbev, network_read_cb, network_write_cb, network_event_cb, pcd);
  bufferevent_enable(pbev, EV_READ | EV_WRITE);
  //printf("New connection (ptr=%x)\n", (void*)pbev);
}

// Listener error callback
static void accept_error_cb(struct evconnlistener *plistener, void *pappdata)
{
  struct event_base  *pbase = evconnlistener_get_base(plistener);
  int                 err   = -1;

  fprintf(stderr, "Libevent: error %d (%s) on the listener! Shutting down.\n", err, evutil_socket_error_to_string(err));
  event_base_loopexit(pbase, NULL);
}

#ifdef __LINUX
void open_dialog(string dialog_path, string dialog_name)
{
  int    pid;
  const int csize = 4096;
//#define SHOW_NEW_PATH
#ifdef SHOW_NEW_PATH
  char   buf[csize];
#endif
  string command;
  char   appname[csize];
  char   path[csize];
  char  *execArgs[3] = {appname, path, NULL};

  printf("opening dialog (fork)\n");
  pid = fork();
  if (pid == 0)
    {
      printf("opening dialog child thread\n");
      // Child process convert this to the dialog app
      /*
      strcpy(appname, dialog_name.c_str());
      printf("changing dir to %s and path is %s\n", dialog_path.c_str(), dialog_name.c_str());
      if (chdir(dialog_path.c_str()) != 0)
	{
	  printf("Chdir failed\n");
	  exit(errno);
	}
      */
#ifdef SHOW_NEW_PATH
      if (getcwd(buf, csize) == NULL)
	{
	  printf("Path error.\n");
	  exit(1);
	}
      else
	{
	  printf("App path=\"%s\"\n", buf);
	}
#endif
      strcpy(path, dialog_path.c_str());
      command = dialog_path + dialog_name;
      strcpy(appname, command.c_str());
      printf("appname=%s path=%s\n", appname, path);
      if (execvp(appname, execArgs) == -1)
	{
	  printf("Execvp failed.\n");
	  exit(errno);
	}
    }
  printf("opening dialog fork done\n");
}
#else
void open_dialog(string dialog_path, string dialog_name)
{
  int                 res;
  PROCESS_INFORMATION process;
  STARTUPINFO         startup;
  DWORD               code;
  string              app_path_name;
  string              cmd_line;
  TCHAR               cmd_str[4096];

  app_path_name = dialog_path + dialog_name;
  memset(&startup, 0, sizeof(startup));
  startup.cb = sizeof(startup);
  //printf("opening dialog \"%s\"\n", dialog_name.c_str());
  cmd_line = app_path_name + string(" ") + dialog_path;
#ifdef _UNICODE
  FIXME check the second argument type
#endif
  strcpy(cmd_str, cmd_line.c_str());
  res = CreateProcess(app_path_name.c_str(),
		      cmd_str,
		      NULL,
		      NULL,
		      FALSE,
		      NORMAL_PRIORITY_CLASS,
		      NULL,
		      NULL,            // Same directory as the caller
		      &startup,
		      &process);
  GetExitCodeProcess(process.hProcess, &code);
  CloseHandle(process.hThread);
  CloseHandle(process.hProcess);
  if (res == 0)
    {
      printf("opening dialog app failed\n");
    }
  printf("dialog process execution done\n");
}
#endif //__LINUX

// Get one or more messages from the app thread
static void Internal_signal_event_callback(evutil_socket_t, short, void *puser_data)
{
  CnetworkMessaging *pshared_mess = (CnetworkMessaging*)puser_data;

#ifdef _DEBUG
  printf("libevent app message internal event.\n");
#endif
  pshared_mess->process_main_app_messages();
}

#ifdef DEBUG_LIBEVENT
static void event_log_callback(int severity, const char *msg)
{
  printf("Log Callback of severity=%d message=%s.\n", severity, msg);
}
#endif

//-------------------------------------------------------------------------------------
//               Class section
//-------------------------------------------------------------------------------------

Creceive_callback_data::Creceive_callback_data(void *pnm):
  m_pmess(pnm)
{
}

Creceive_callback_data::~Creceive_callback_data()
{
}

CnetworkMessaging::CnetworkMessaging(int port):
  m_server_port(port)
{
  m_write_completed = true;
  m_plisten_socket = new sockaddr_in();
}

CnetworkMessaging::~CnetworkMessaging()
{
  std::list<Creceive_callback_data*>::iterator it;

#ifdef __WINDOWS
  WSACleanup();
#endif
  event_free(m_pinternal_event);
  event_base_free(m_pevt_base);
  pthread_mutex_destroy(&m_message_mutex);
  pthread_mutex_destroy(&m_datapipe_mutex);
  delete m_plisten_socket;
  it = m_rcv_data_list.begin();
  while (it != m_rcv_data_list.end())
    {
      delete *it;
      it++;
    }
}

bool CnetworkMessaging::sdl_register_user_events(int *firstevent, int numevents)
{
  int i;

  i = SDL_RegisterEvents(numevents);
  if (i == -1)
    {
      printf("SDL user event allocation failed.\n");
      exit(1);
      //return false;
    }
  *firstevent = i;
  return true;
}

int CnetworkMessaging::init_the_network_messaging()
{
  unsigned int i;
  int          firstevent;

  pthread_mutex_init(&m_message_mutex, NULL);
  pthread_mutex_init(&m_datapipe_mutex, NULL);
  if (sdl_register_user_events(&firstevent, 1))
    {
      m_sdl_user_event_dialog_message_available = firstevent;
    }
  for (i = 0; i < sizeof(m_bactive_dialogs) / sizeof(bool); i++)
    m_bactive_dialogs[i] = false;
  if (Server_init())
    {
      printf("Something went wrong with the TCP/IP server section.\n");
      return 1;
    }
  return 0;
}

void CnetworkMessaging::triger_the_sdl_user_event()
{
  SDL_Event event;
  
  SDL_zero(event);
  event.type = m_sdl_user_event_dialog_message_available;
  event.user.code = 0;
  event.user.data1 = 0;
  if (SDL_PushEvent(&event) == 0)
    printf("SDL error, pushing the user event failed in \"%s\".\n", __FILE__);
}

int CnetworkMessaging::get_sdl_dialog_user_event_code()
{
  return m_sdl_user_event_dialog_message_available;
}

string CnetworkMessaging::get_dialog_app_name(edialogs_codes code)
{
#ifdef __LINUX
  string ext = string("bin");
#else
  string ext = string("exe");
#endif

  switch (code)
    {
    case load_save_dialog:
      return string(STORAGE_DIALOG_NAME) + ext;
    case add_instrument_dialog:
      return string(ADDINSTRUMENT_DIALOG_NAME) + ext;
    case practice_dialog:
      return string(PRACTICE_DIALOG_NAME) + ext;
    case config_dialog:
      return string(CONFIG_DIALOG_NAME) + ext;
    default:
      break;
    };
  return string("");
}

string CnetworkMessaging::get_dialog_code_name(edialogs_codes code)
{
  switch (code)
    {
    case load_save_dialog:
      return string(STORAGE_DIALOG_CODE_NAME);
    case add_instrument_dialog:
      return string(ADDINSTRUMENT_DIALOG_CODENAME);
    case practice_dialog:
      return string(PRACTICE_DIALOG_CODENAME);
    case config_dialog:
      return string(CONFIG_DIALOG_CODENAME);
    default:
      break;
    };
  return string("");
}

struct bufferevent* CnetworkMessaging::find_open_connection(int dialog_code)
{
  std::list<t_connection>::iterator iter;

  iter = m_active_connections.begin();
  while (iter != m_active_connections.end())
    {
      if ((*iter).dialog == dialog_code && m_bactive_dialogs[dialog_code])
	{
	  return (*iter).pevent;
	}
      iter++;
    }
  return NULL;
}

void CnetworkMessaging::lock_datapipe()
{
  LOCK_DATA_PIPE;
}

void CnetworkMessaging::unlock_datapipe()
{
  UNLOCK_DATA_PIPE;
}

// To be called inside the dialog box connection event
bool CnetworkMessaging::Send_to_network_client(edialogs_codes dialog_code, const char *pbuffer, int length)
{
  struct bufferevent *pevent;
  int                i;

  pevent = find_open_connection(dialog_code);
  if (pevent != NULL)
    {
      i = 0;
      // FIXME do a write complete per connection
      while (m_write_completed == false && i < 200)
	{
	  usleep(10000);
	  i++;
	}
      //assert(pbuffer[length - 1] == 0); does not pass
      // To be sure that the pipe is opened, and therefore avoid SIGPIPE
      // retest for the connexion to avoid a broken pipe
      LOCK_DATA_PIPE;
      pevent = find_open_connection(dialog_code);
      if (pevent != NULL)
	{
#ifdef _DEBUG
	  printf("sending to dialog:\n\"%s\" of %d bytes\n", pbuffer, length);
#endif
	  //printf("Sending to connection (ptr=%x)\n", (void*)pevent);
	  //printf("BUFFER WRITE SET TO FALSE\n");
	  // FIXME m_write_completed does not go back to true all the time. Find out why?
	  //m_write_completed = false;
	  if (bufferevent_write(pevent, pbuffer, length) == -1)
	    {
	      printf("Libevent \"bufferevent_write\" failed.\n");
	    }
	}
      UNLOCK_DATA_PIPE;
    }
  else
    {
      printf("Libevent, no dialog with code=%d.\n", dialog_code);
      return true;
    }
  return false;
}

// Stores the message and triggers the event
bool CnetworkMessaging::Send_to_dialogs(edialog_message internal_message_code, const char *pdata, int size, void *pbevt)
{
  t_internal_message msg;
  int                i, obsolete_arg;

  if (size > MESSAGE_CONTENT_SIZE)
    {
      printf("Message error: too big.\n");
      return false;
    }
  msg.passociated_event = pbevt;
  msg.message_code = internal_message_code;
  msg.size = size;
  for (i = 0; i < size; i++)
    msg.data[i] = pdata[i];
  LOCK_MSG;
  m_app2network_message_list.push_back(msg);
  UNLOCK_MSG;
#ifdef _DEBUG
  printf("Activating the libevent internal event\n");
#endif
  obsolete_arg = 0;
  event_active(m_pinternal_event, EV_WRITE, obsolete_arg);
  return false;
}

// Stores the messages for later, this is for messages sent too fast like note messages
bool CnetworkMessaging::Send_to_dialogs_with_delay(edialog_message internal_message_code, const char *pdata, int size)
{
  std::list<t_internal_message>::iterator iter;
  t_internal_message msg;
  int                i;

  if (size > MESSAGE_CONTENT_SIZE)
    {
      printf("Message error: too big.\n");
      return false;
    }
  msg.message_code = internal_message_code;
  msg.size = size;
  for (i = 0; i < size; i++)
    msg.data[i] = pdata[i];
  LOCK_MSG;
  // Delete the previous not sent messages of the same type
  iter = m_app2network_message_delayed_list.begin();
  while (iter != m_app2network_message_delayed_list.end())
    {
      if ((*iter).message_code == internal_message_code)
	{
	  iter = m_app2network_message_delayed_list.erase(iter);
	}
      else
	iter++;
    }
  // Push the last message into the list
  m_app2network_message_delayed_list.push_front(msg);
  UNLOCK_MSG;
  return false;
}

// Send the previously stored messages
bool CnetworkMessaging::Send_to_dialogs_delayed_messages(double last_time)
{
  static double                           exp_last_time = -1;
  std::list<t_internal_message>::iterator iter;
  t_internal_message                      msg;
  int                                     obsolete_arg;
  bool                                    bsend;

  if (last_time - exp_last_time > SEND_DELAY)
    {
      bsend = false;
      LOCK_MSG;
      if (m_app2network_message_delayed_list.size())
	{
	  msg = *(m_app2network_message_delayed_list.begin());
	  m_app2network_message_delayed_list.pop_front();
	  m_app2network_message_list.push_back(msg);
	  bsend = true;
	}
      UNLOCK_MSG;
      if (bsend)
	{
	  exp_last_time = last_time;
	  obsolete_arg = 0;
	  event_active(m_pinternal_event, EV_WRITE, obsolete_arg);
	}
    }
  return false;
}

bool CnetworkMessaging::Server_init()
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
      return true;
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
      return true;
    }
  // Create new event base
  m_pevt_base = event_base_new();
  if (m_pevt_base == NULL)
    {
      printf("Couldn't open event base");
      return true;
    }
  // An Internal event
  m_pinternal_event = event_new(m_pevt_base, -1, 0, Internal_signal_event_callback, this);
  // Network buffer events
  memset(m_plisten_socket, 0, sizeof(struct sockaddr_in));
  m_plisten_socket->sin_family = AF_INET;
  m_plisten_socket->sin_addr.s_addr = inet_addr(SERVER_HOST);
  m_plisten_socket->sin_port = SCOREVIEW_PORT;
  // Create a new listener
  m_plistener = evconnlistener_new_bind(m_pevt_base, accept_conn_cb, this,
				       LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, MAX_PENDING_CONNECTIONS,
				       (struct sockaddr *)m_plisten_socket, sizeof(struct sockaddr_in));
  if (m_plistener == NULL)
    {
      perror("Couldn't create listener.\n");
      return true;
    }
#ifdef DEBUG_LIBEVENT
  event_set_log_callback(event_log_callback);
#endif
  evconnlistener_set_error_cb(m_plistener, accept_error_cb);
  return false;
}

void CnetworkMessaging::Server_loop()
{
  event_base_dispatch(m_pevt_base);
}

// Note: called in an already locked section
void CnetworkMessaging::remote_close_all_the_dialogs()
{
  std::list<t_connection>::iterator  iter;
  scmsg::Cmessage_coding             coder;
  string                             str;
  const int                          cmsgsize = 4096;
  char                               msg[cmsgsize];
  int                                size;

  str = coder.create_close_message();
  if (coder.build_wire_message(msg, cmsgsize, &size, str))
    {
      // Wait for the write callback of the last "close" message
      iter = m_active_connections.begin();
      while (iter != m_active_connections.end())
	{
	  printf("sending close msg to %d.\n", (*iter).dialog);
	  // Close after the last write completes
	  Send_to_network_client((*iter).dialog, msg, size);
	  iter++;
	}
    }
  else
    printf("Error: remote close message could not be built.\n");
}

void CnetworkMessaging::remote_close_dialog(edialogs_codes code)
{
  std::list<t_connection>::iterator  iter;
  scmsg::Cmessage_coding             coder;
  string                             str;
  const int                          cmsgsize = 4096;
  char                               msg[cmsgsize];
  int                                size;

  str = coder.create_close_message();
  if (coder.build_wire_message(msg, cmsgsize, &size, str))
    {
      // Wait for the write callback of the last "close" message
      iter = m_active_connections.begin();
      while (iter != m_active_connections.end())
	{
	  if (code == (*iter).dialog)
	    {
#ifdef _DEBUG
	      printf("sending close msg to %d.\n", (*iter).dialog);
#endif
	      // Close after the last write completes
	      Send_to_network_client((*iter).dialog, msg, size);
	      //m_bactive_dialogs[code] = false;
	    }
	  iter++;
	}
    }
  else
    printf("Error: remote close message could not be built.\n");
}

// Wakes up with the internal event callback
// Gets the messages from the list
// Send them if the dialog is opened
void CnetworkMessaging::process_main_app_messages()
{
  std::list<t_internal_message>::iterator  iter;
  t_internal_message                      *pmsg;

#ifdef _DEBUG
  printf("process main app messages\n");
#endif
  LOCK_MSG;
  iter = m_app2network_message_list.begin();
  while (iter != m_app2network_message_list.end())
    {
      pmsg = &(*iter);
      switch ((*iter).message_code)
	{
	case message_close_dialogs:
	  {
	    remote_close_all_the_dialogs();
	  }
	case message_quit_its_over:
	  {
	    Exit_EventLoop();
	  }
	  break;
	case message_open_storage_dialog:
	  {
	    if (m_bactive_dialogs[load_save_dialog])
	      {
		printf("Warning: the storage dialog is already opened.\n");
		break;
	      }
	    // The message part is empty...
	    open_dialog(string(STORAGE_DIALOG_PATH), get_dialog_app_name(load_save_dialog));
	  }
	  break;
	case message_open_addinstrument_dialog:
	  {
	    if (m_bactive_dialogs[add_instrument_dialog])
	      {
		printf("the instrument dialog should not be opened\n");
		break;
	      }
	    // The message part is empty...
	    open_dialog(string(ADDINSTRUMENT_DIALOG_PATH), get_dialog_app_name(add_instrument_dialog));
	  }
	  break;
	case message_open_practice_dialog:
	  {
	    if (m_bactive_dialogs[practice_dialog])
	      {
		printf("Practice dialog opened, sending the message.\n");
		Send_to_network_client(practice_dialog, pmsg->data, pmsg->size);
		break;
	      }
	    open_dialog(string(PRACTICE_DIALOG_PATH), get_dialog_app_name(practice_dialog));
	  }
	  break;
	case message_open_config_dialog:
	  {
	    if (m_bactive_dialogs[config_dialog])
	      {
		printf("Config dialog opened, sending the message.\n");
		Send_to_network_client(config_dialog, pmsg->data, pmsg->size);
		break;
	      }
	    open_dialog(string(CONFIG_DIALOG_PATH),  get_dialog_app_name(config_dialog));
	  }
	  break;
	case message_close_storage_dialog:
	  {
	    remote_close_dialog(load_save_dialog);
	  }
	  break;
	case message_close_addinstrument_dialog:
	  {
	    remote_close_dialog(add_instrument_dialog);
	  }
	  break;
	case message_close_practice_dialog:
	  {
	    remote_close_dialog(practice_dialog);
	  }
	  break;
	case message_close_config_dialog:
	  {
	    remote_close_dialog(config_dialog);    
	  }
	  break;
	case message_send_instrument_list:
	  {
	    Send_to_network_client(add_instrument_dialog, pmsg->data, pmsg->size);
	  }
	  break;
	case message_send_pa_devices_list:
	case message_send_configuration:
	  {
	    Send_to_network_client(config_dialog, pmsg->data, pmsg->size);
	  }
	  break;
	case message_send_practice:
	  {
#ifdef _DEBUG
	    printf("event send practice-----------------------------------\n");
#endif
	    Send_to_network_client(practice_dialog, pmsg->data, pmsg->size);
	  }
	  break;
	case message_score_transfer:
	  {
#ifdef _DEBUG
	    printf("event send score to practice--------------------------------------\n");
#endif
	    Send_to_network_client(practice_dialog, pmsg->data, pmsg->size);
	  }
	  break;
	case message_note_highlight:
	case message_send_remadd_note:
	case message_send_remadd_measure:
	case message_note_transfer:
	  {
	    Send_to_network_client(practice_dialog, pmsg->data, pmsg->size);
	  }
	  break;
	default:
	  printf("unknown event message\n");
	  break;
	};
      iter = m_app2network_message_list.erase(iter);
    }
  UNLOCK_MSG;
}

void CnetworkMessaging::push_message(t_internal_message *pmsg)
{
  LOCK_MSG;
  m_network2app_message_list.push_back(*pmsg);
  UNLOCK_MSG;
}

bool CnetworkMessaging::pop_message(t_internal_message *pmsg)
{
  bool  bret;

  bret = true;
  LOCK_MSG;
  if (m_network2app_message_list.size() == 0)
    {
      bret = false;
    }
  else
    {
      *pmsg = *m_network2app_message_list.begin();
      m_network2app_message_list.pop_front();
    }
  UNLOCK_MSG;
  return bret;
}

void CnetworkMessaging::register_dialog_connection(t_internal_message *pmsg, edialogs_codes dialog_type)
{
  std::list<t_connection>::iterator  iter;

  printf("resgitering opened dialog\n");
  LOCK_MSG;
  iter = m_active_connections.begin();
  while (iter != m_active_connections.end())
    {
      if ((*iter).pevent == pmsg->passociated_event)
	{
	  if ((*iter).dialog == unknown_dialog)
	    {
	      (*iter).dialog = dialog_type;
	      m_bactive_dialogs[dialog_type] = true;
	      UNLOCK_MSG;
	      return ;
	    }
	  else
	    printf("Error, the dialog message type is already known!\n");
	}
      iter++;
    }
  UNLOCK_MSG;
}

void CnetworkMessaging::unregister_dialog_connection(struct bufferevent *pevt)
{
  std::list<t_connection>::iterator  iter;

  printf("unregistering dialog\n");
  //LOCK_MSG;
  iter = m_active_connections.begin();
  while (iter != m_active_connections.end())
    {
      if ((*iter).pevent == pevt)
	{
	  m_bactive_dialogs[(*iter).dialog] = false;
	  iter = m_active_connections.erase(iter);
	}
      iter++;
    }
  //UNLOCK_MSG;
}

bool CnetworkMessaging::is_practice_dialog_enable()
{
  std::list<t_connection>::iterator  iter;

  iter = m_active_connections.begin();
  while (iter != m_active_connections.end())
    {
      if ((*iter).dialog == practice_dialog)
	{
	  return true;
	}
      iter++;
    }
  return false;
}

void CnetworkMessaging::Exit_EventLoop()
{
  // This will make Server_loop(); return
  if (event_base_loopexit(m_pevt_base, NULL) == -1) // Waits for the last registered events to be completed
    //if (event_base_loopbreak(m_pevt_base) == -1)
    printf("Critical error in a libevent call: loopbreak failed.\n");
}

// Adds the allocated callback data structure to a list to be able to free it later
void CnetworkMessaging::add_cb_data(Creceive_callback_data *pcbdata)
{
  m_rcv_data_list.push_back(pcbdata);
}

