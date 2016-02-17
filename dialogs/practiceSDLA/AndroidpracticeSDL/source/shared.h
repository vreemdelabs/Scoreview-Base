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

typedef struct   s_internal_message
{
  int            message_code;
  void          *passociated_event;
  int            size;
  char           data[MESSAGE_CONTENT_SIZE];
}                t_internal_message;

typedef struct                  s_shared_data
{
  pthread_mutex_t               data_mutex;
  CTCP_Client                  *ptcpclient;
  pthread_t                     threadclient;
  std::list<t_internal_message> network2app_message_list;
  bool                          bconnected;
  bool                          bquit;
}                               t_shared_data;

