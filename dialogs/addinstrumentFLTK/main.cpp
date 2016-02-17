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
#include <unistd.h>
#include <string>
#include <list>
#include <vector>

#include <time.h>
#include <tinyxml.h>
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
#include <FL/Fl_Choice.H>
#include <FL/fl_ask.H>

#include "dialogpath.h"
#include "audioapi.h"
#include "score.h"
#include "cfgfile.h"
#include "messages.h"
#include "message_decoder.h"

#include "tcp_ip.h"
#include "tcpclient.h"
#include "main.h"
#include "threads.h"

using namespace std;
using namespace scmsg;

void read_window_pos(t_app_data *papp_data)
{
  int x, y;
  Cxml_cfg_file_decoder *pd;
  string path;

  papp_data->xpos = papp_data->ypos = 0;
  pd = new Cxml_cfg_file_decoder();
  path = pd->get_user_config_path();
  if (pd->open_for_reading(path + string("dialogs/addinstrumentFLTK/") + string(CONFIG_FILE_NAME)))
    {
      if (pd->read_window_position(&x, &y))
	{
	  papp_data->xpos = x;
	  papp_data->ypos = y;
	}
    }
  delete pd;
}

void save_coordinates(t_app_data *pappdata)
{
  int x, y;
  string path;

  x = pappdata->pwindow->x_root();
  y = pappdata->pwindow->y_root();
  //printf("position is %d,%d compared to %d,%d\n", x, y, pappdata->xpos, pappdata->ypos);
  if (x != pappdata->xpos || y != pappdata->ypos)
    {
      Cxml_cfg_file_decoder *pd = new Cxml_cfg_file_decoder();

      pd->open_for_writing();
      pd->write_window_position(x, y);
      path = pd->get_user_config_path();
      pd->write(path + string("dialogs/addinstrumentFLTK/") + string(CONFIG_FILE_NAME));
      delete pd;
    }
}

void close_function(void *pdata)
{
  t_app_data *papp_data = (t_app_data*)pdata;
  int i;

  save_coordinates(papp_data);
  usleep(200000); // ???????? it locks if not present FIXMEa
  papp_data->ptcpclient->close_function(string(ADDINSTRUMENT_DIALOG_CODENAME));
  i = 0;
  while (papp_data->ptcpclient->get_state() != state_closed && i < 8)
    {
      usleep(100000);
      i++;
    }
  release_threads(papp_data);
  delete papp_data->ptcpclient;
  papp_data->pwindow->hide(); // Close the window
}

// Alt+F4 or close cross
void wincall(Fl_Widget *pwi, void *pdata)
{
  //printf("close_callback called\n" );
  close_function(pdata);
}

void add_instru_callback(Fl_Widget *pwi, void *pdata)
{
  t_app_data     *papp_data = (t_app_data*)pdata;
  string          name;
  Cmessage_coding coder;
  string          str;
  int             size;
  const int       cmsgsize = 4096;
  char            msg[cmsgsize];
  t_instrument    instru;
  bool            bdelete = false;

  switch (papp_data->pchoice->value())
    {
    case 0:
      name = string("violin");
      break;
    case 1:
      name = string("piano");
      break;
    case 2:
      name = string("guitar");
      break;
    case 3:
      name = string("guitar_dropD");
      break;
    default:
      name = string("unknown instrument");
      break;
    };
  instru.voice_index = 0; // Not used because new instrument
  instru.name = name;
  str = coder.create_remadd_instrument_message(&instru, bdelete);
  if (coder.build_wire_message(msg, cmsgsize, &size, str))
    {
      printf("-------------------- sent back isntrument add command message\n");
      papp_data->ptcpclient->Send(msg, size);
    }
  else
    {
      printf("Instrument selection error.\n");
    }
  usleep(10000);
  close_function(pdata);
}

void delete_instru_callback(Fl_Widget *pwi, void *pdata)
{
  std::list<t_intrument_voice>::iterator iter;
  t_app_data       *papp_data = (t_app_data*)pdata;
  string            msg, instrnum;
  unsigned int      index;
  t_intrument_voice ins;
  char              numstr[64];

  index = papp_data->pdelchoice->value();
  ins.voice = -1;
  ins.name = "unknown";
  if (index < papp_data->instr_vect.size())
    {
      ins = papp_data->instr_vect[index];
    }
  sprintf(numstr, "%d", ins.voice);
  instrnum = string(numstr);
  msg = string("Do you really want to delete ") + ins.name + " " + instrnum + string("?");
  if (fl_choice(msg.c_str(), fl_no, fl_yes, NULL))
    {
      Cmessage_coding coder;
      string          str;
      int             size;
      const int       cmsgsize = 4096;
      char            msg[cmsgsize];
      t_instrument    instru;
      bool            bdelete = true;

      instru.voice_index = ins.voice;
      instru.name = ins.name;
      str = coder.create_remadd_instrument_message(&instru, bdelete);
      if (coder.build_wire_message(msg, cmsgsize, &size, str))
	{
	  papp_data->ptcpclient->Send(msg, size);
	}
      close_function(pdata);
    }
}

void add_selection_callback(Fl_Widget *pwi, void *pdata)
{
  //t_app_data *papp_data = (t_app_data*)pdata;

  //printf("menselcallback selected is:%d\n", papp_data->pchoice->value());
}

void del_selection_callback(Fl_Widget *pwi, void *pdata)
{
  //t_app_data *papp_data = (t_app_data*)pdata;

  //printf("menselcallback selected is:%d\n", papp_data->pdelchoice->value());
}

void set_button(Fl_Button *pb, void* puser_data)
{
  pb->type(FL_NORMAL_BUTTON);
  pb->labelsize(21);
  pb->user_data(puser_data);
}

void wait_for_instrument_list(t_app_data *papp_data)
{
  int i;

  i = 0;
  while (i < 20)
    {
      usleep(100000);
      pthread_mutex_lock(&papp_data->cond_instlistmutex);
      if (papp_data->blistreceived);
      {
	pthread_mutex_unlock(&papp_data->cond_instlistmutex);
	return;
      }
      pthread_mutex_unlock(&papp_data->cond_instlistmutex);
      i++;
    }
/*
  pthread_mutex_lock(&papp_data->cond_instlistmutex);
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 3;
  pthread_cond_timedwait(&papp_data->cond_instlist, &papp_data->cond_instlistmutex, &ts);
  //pthread_cond_wait(&papp_data->cond_instlist, &papp_data->cond_instlistmutex);
  pthread_mutex_unlock(&papp_data->cond_instlistmutex);
*/
}

void Send_dialog_opened_message(t_app_data *papp_data, string dialog_id)
{
  string          message;
  Cmessage_coding coder;
  int             size;
  const int       cmsgsize = 4096;
  char            msg[cmsgsize];

  message = coder.create_dialog_opened_message(dialog_id);
  if (coder.build_wire_message(msg, cmsgsize, &size, message))
    {
      papp_data->ptcpclient->Send(msg, size);      
    }
}

int main(int argc, char **argv)
{
  int          wx, wy;
  unsigned int i;
  int          yoffset;
  int          xoffset;
  int          buttonh;
  int          buttonoffset;
  Fl_Window   *window;
  Fl_Button   *but_add;
  Fl_Button   *but_delete;
  t_app_data   app_data;
  Fl_Choice   *pchoice;
  Fl_Choice   *pdelchoice;
  t_intrument_voice v;
  std::list<t_intrument_voice>::iterator iter;

  if (argc > 1)
    app_data.path = string(argv[1]);
    //app_data.path = string(ADDINSTRUMENT_DIALOG_PATH);
  else
    app_data.path = string("./");
  printf("argc=%d argO=%s arg1=%s arg2=%s\n", argc, argv[0], argv[1]? argv[1]: "rien", argv[2]? argv[2]: "rien");
  app_data.bconnected = true;
  app_data.pwindow    = NULL;
  app_data.blistreceived = false;
  app_data.ptcpclient = new CTCP_Client((void*)&app_data, std::string(SERVER_HOST), SCOREVIEW_PORT);
  if (!app_data.ptcpclient->Server_init())
    {
      printf("Dialog error: the tcp client side failed to initialise.\n");
      return 1;
    }
  if (!app_data.ptcpclient->Connect())
    {
      printf("Dialog error: connection to the main app failed.\n");
      return 1;
    }
  create_threads(&app_data);
  pthread_mutex_init(&app_data.cond_instlistmutex, NULL);
  pthread_cond_init(&app_data.cond_instlist, NULL);
  Send_dialog_opened_message(&app_data, string(ADDINSTRUMENT_DIALOG_CODENAME));
  wait_for_instrument_list(&app_data);
  if (app_data.bconnected)
    {
      Fl_File_Icon::load_system_icons();
      read_window_pos(&app_data);
      wx = 300;
      wy = 360;
      window = new Fl_Window(app_data.xpos, app_data.ypos, wx, wy, "+/-  instruments");
      app_data.pwindow = window;
      window->begin();
      xoffset = 10;
      yoffset = 20;
      buttonh = (wy - 1 * yoffset) / 6;
      buttonoffset = yoffset;
      //
      pchoice = new Fl_Choice(xoffset, buttonoffset, wx - xoffset * 2, buttonh, "");
      pchoice->add(" Violin");
      pchoice->add(" Piano");
      pchoice->add(" Guitar");
      pchoice->add(" Guitar drop D tuning");
      pchoice->value(0);
      //pchoice->add("Violin", violin, menu_selection_callback, &app_data);
      //pchoice->add("Grand piano", piano, menu_selection_callback, &app_data);
      pchoice->callback(add_selection_callback);
      pchoice->user_data(&app_data);
      buttonoffset += buttonh + 10;
      //
      but_add      = new Fl_Button(xoffset, buttonoffset, wx - xoffset * 2, buttonh, "Add");
      buttonoffset += buttonh + 10;
      buttonoffset += buttonh + 10;
      set_button(but_add, &app_data);
      but_add->callback(add_instru_callback);
      pdelchoice = NULL;
      if (app_data.instr_vect.size() > 1) // At least 1 instrument must be kept
	{
	  pdelchoice = new Fl_Choice(xoffset, buttonoffset, wx - xoffset * 2, buttonh, "");
	  buttonoffset += buttonh + 10;
	  for (i = 0; i < app_data.instr_vect.size(); i++)
	    {
	      const int csize = 128;
	      char      section[csize];

	      snprintf(section, csize, "%s %d", app_data.instr_vect[i].name.c_str(), app_data.instr_vect[i].voice);
	      //printf("section==%s\n", section);
	      pdelchoice->add(section);
	    }
	  pdelchoice->value(0);
	  pdelchoice->callback(del_selection_callback);
	  pdelchoice->user_data(&app_data);
	  //
	  but_delete   = new Fl_Button(xoffset, buttonoffset, wx - xoffset * 2, buttonh, "Delete");
	  buttonoffset += buttonh + 10;
	  set_button(but_delete, &app_data);
	  but_delete->callback(delete_instru_callback);
	  //
	}
      window->end();
      app_data.pchoice = pchoice;
      app_data.pdelchoice = pdelchoice;
      window->callback(wincall);
      window->user_data(&app_data);
      argc = 1;
      window->show(argc, argv);
      Fl::scheme("gtk+");
      return Fl::run();
    }
  return 0;
}

