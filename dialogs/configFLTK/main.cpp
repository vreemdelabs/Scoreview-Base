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
#include <FL/Fl_Tabs.H>
#include <FL/Fl_Check_Button.H>

#include "dialogpath.h"
#include "audioapi.h"
#include "cfgfile.h"
#include "score.h"
#include "messages.h"
#include "message_decoder.h"
#include "config_tab.h"
#include "input_tab.h"
#include "info_tab.h"
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
  string                 path;

  papp_data->xpos = papp_data->ypos = 0;
  pd = new Cxml_cfg_file_decoder();
  path = pd->get_user_config_path();
  if (pd->open_for_reading(path + string("dialogs/configFLTK/") + string(CONFIG_FILE_NAME)))
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
  int    x, y;
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
      pd->write(path + string("dialogs/configFLTK/") + string(CONFIG_FILE_NAME));
      delete pd;
    }
}

void close_function(void *pdata)
{
  t_app_data *papp_data = (t_app_data*)pdata;

  save_coordinates(papp_data);
  papp_data->ptcpclient->close_function(string(CONFIG_DIALOG_CODENAME));
  usleep(100000);
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

void set_button(Fl_Button *pb, void* puser_data)
{
  pb->type(FL_NORMAL_BUTTON);
  pb->labelsize(21);
  pb->user_data(puser_data);
}

void wait_for_portaudio_devices_list(t_app_data *papp_data)
{
  struct timespec   ts;

  pthread_mutex_lock(&papp_data->cond_devlistmutex);
  clock_gettime(CLOCK_REALTIME, &ts);
  ts.tv_sec += 6;
  pthread_cond_timedwait(&papp_data->cond_devlist, &papp_data->cond_devlistmutex, &ts);
  //pthread_cond_wait(&papp_data->cond_instlist, &papp_data->cond_instlistmutex);
  pthread_mutex_unlock(&papp_data->cond_devlistmutex);
}

int main(int argc, char **argv)
{
  int         wx, wy;
  int         border;
  Fl_Window  *window;
  Fl_Tabs    *tabs;
  t_app_data  app_data;

  if (argc > 1)
    app_data.path = string(argv[1]);
  else
    app_data.path = string("./");
  app_data.bconnected = true;
  app_data.pwindow    = NULL;
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
  pthread_mutex_init(&app_data.cond_devlistmutex, NULL);
  pthread_cond_init(&app_data.cond_devlist, NULL);
  Send_dialog_opened_message(&app_data, string(CONFIG_DIALOG_CODENAME));
  wait_for_portaudio_devices_list(&app_data);
  printf("Return from the wait for message ------------------------------------------\n");
  if (app_data.bconnected)
    {
      Fl_File_Icon::load_system_icons();
      read_window_pos(&app_data);
      //read_main_app_params(&app_data);
      wx = 500;
      wy = 600;
      window = new Fl_Window(app_data.xpos, app_data.ypos, wx, wy, "Settings");
      app_data.pwindow = window;
      window->begin();
      //
      border = 10;
      tabs = new Fl_Tabs(border, border, wx - 2 * border, wy - 2 * border);
      create_config_tab(20, 30, wx - 30, wy - 30, &app_data);
      create_sound_input_tab(20, 30, wx - 30, wy - 30, &app_data);
      create_info_tab(20, 30, wx - 30, wy - 30);
      tabs->end();
      //
      window->end();
      window->callback(wincall);
      window->user_data(&app_data);
      argc = 1;
      window->show(argc, argv);
      Fl::scheme("gtk+");
      return Fl::run();
    }
  return 0;
}

