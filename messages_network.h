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

#define MAX_PENDING_CONNECTIONS 8
#define SEND_DELAY              0.1

//#define DEBUG_LIBEVENT

typedef struct        s_connection
{
  edialogs_codes      dialog;
  struct bufferevent *pevent;
}                     t_connection;

typedef struct   s_internal_message
{
  int            message_code;
  void          *passociated_event;
  int            size;
  char           data[MESSAGE_CONTENT_SIZE];
}                t_internal_message;

class CnetworkMessaging
{
 public:
  CnetworkMessaging(int port);
  ~CnetworkMessaging();

  int  init_the_network_messaging();
  void remote_close_all_the_dialogs();

  bool Send_to_network_client(edialogs_codes dialog_code, const char *pdata, int length);
  bool Send_to_dialogs(edialog_message internal_message_code, const char *pdata, int size);
  bool Send_to_dialogs_with_delay(edialog_message internal_message_code, const char *pdata, int size);
  bool Send_to_dialogs_delayed_messages(double last_time);
  void Server_loop();
  void process_main_app_messages();
  void triger_the_sdl_user_event();
  int  get_sdl_dialog_user_event_code();
  void push_message(t_internal_message *pmsg);
  bool pop_message(t_internal_message *pmsg);
  void register_dialog_connection(t_internal_message *pmsg, edialogs_codes dialog_type);
  void unregister_dialog_connection(struct bufferevent *pevt);
  bool is_practice_dialog_enable();
  void Exit_EventLoop();

 private:
  bool                Server_init();
  std::string         get_dialog_app_name(edialogs_codes code);
  std::string         get_dialog_code_name(edialogs_codes code);
  struct bufferevent* find_open_connection(int dialog_code);
  bool                sdl_register_user_events(int *firstevent, int numevents);

 private:
  pthread_mutex_t               m_message_mutex;
  int                           m_sdl_user_event_dialog_message_available;
  std::list<t_internal_message> m_app2network_message_list;
  std::list<t_internal_message> m_app2network_message_delayed_list;
  std::list<t_internal_message> m_network2app_message_list;
  bool                          m_bactive_dialogs[10];

 public:
  std::list<t_connection>       m_active_connections;
  bool                          m_write_completed;

 private:
  int                    m_server_port;
// public:
  struct event_base     *m_pevt_base;
  struct event          *m_pinternal_event;
  struct evconnlistener *m_plistener;
  struct sockaddr_in    *m_plisten_socket;
};

#define LOCK_MSG   pthread_mutex_lock(&this->m_message_mutex);
#define UNLOCK_MSG pthread_mutex_unlock(&this->m_message_mutex);

