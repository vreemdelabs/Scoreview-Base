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

extern "C"
{ 
  void read_cb(struct bufferevent *pbev, void *pappdata);
  void write_cb(struct bufferevent *pbev, void *pappdata);
  void event_cb(struct bufferevent *bev, short events, void *pappdata);
}

enum client_state
  {
    state_wainting_connection,
    state_writing_message,
    state_ready_for_writing,
    state_closing,
    state_closed
  };

class CTCP_Client
{
 public:
  CTCP_Client(void *papp_data, std::string address, int port);
  ~CTCP_Client();

  bool Server_init();
  bool Connect();
  void EventLoop();
  void Exit_EventLoop();
  bool Send(const void *pbuffer, int size);
  struct event_base* get_base_evt();

  bool check_closing_state();
  bool check_waiting_connection_state();
  void set_to_closed_state();
  client_state get_state();
  void set_state(client_state state);
  void send_dialog_opened_message(std::string dialog_id);
  void close_function(std::string dialog_codename);

 private:
  bool wait_for_write_completion();

 private:
  int                    m_port;
  void                  *m_pappdata;
  struct sockaddr_in    *m_plisten_socket;
  struct event_base     *m_pbase;
  struct bufferevent    *m_pbuffer_event;
  struct sockaddr_in     m_addr_socket;
  bool                   m_bopened;
  client_state           m_state;
};
