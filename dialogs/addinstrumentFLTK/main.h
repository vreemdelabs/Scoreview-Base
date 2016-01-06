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

void close_function(void *pdata);

enum einstruments_shortcut
  {
    violin = 2420,
    piano,
    guitar,
    bass,
    trumpet,
    sax
  };

typedef struct s_intrument_voice
{
  std::string  name;
  int          voice;
}              t_intrument_voice;

typedef struct                   s_app_data
{
  CTCP_Client                   *ptcpclient;
  bool                           blistreceived;
  std::string                    path;
  int                            xpos;
  int                            ypos;
  Fl_Window                     *pwindow;
  pthread_t                      threadclient;
  pthread_mutex_t                cond_instlistmutex;
  pthread_cond_t                 cond_instlist;
  Fl_Choice                     *pchoice;
  Fl_Choice                     *pdelchoice;
  bool                           bconnected;
  std::vector<t_intrument_voice> instr_vect;
}                                t_app_data;

