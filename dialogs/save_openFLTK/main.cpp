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
#include <unistd.h>
#include <string>
#include <iterator>
#include <list>

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

void read_window_pos_n_music_path(t_app_data *papp_data)
{
  Cxml_cfg_file_decoder *pd;
  int    x, y;
  string path;

  papp_data->xpos = papp_data->ypos = 0;
  pd = new Cxml_cfg_file_decoder();
  path = pd->get_user_config_path();
  if (pd->open_for_reading(path + string("dialogs/save_openFLTK/") + string(CONFIG_FILE_NAME)))
    {
      if (pd->read_window_position(&x, &y))
	{
	  papp_data->xpos = x;
	  papp_data->ypos = y;
	}
      if (!pd->read_music_path(&papp_data->music_path))
	{
	  papp_data->music_path = string("./");
	}
      if (!pd->read_audio_files_path(&papp_data->audio_files_path))
	{
	  papp_data->audio_files_path = string("./");
	}
    }
  delete pd;
}

void save_coordinates_n_music_path(t_app_data *pappdata)
{
  int    x, y;
  string path;

  x = pappdata->pwindow->x_root();
  y = pappdata->pwindow->y_root();
  //printf("position is %d,%d compared to %d,%d\n", x, y, pappdata->xpos, pappdata->ypos);
  if (x != pappdata->xpos || y != pappdata->ypos || pappdata->bpath_changed)
    {
      Cxml_cfg_file_decoder *pd = new Cxml_cfg_file_decoder();

      pd->open_for_writing();
      pd->write_window_position(x, y);
      pd->write_music_path(pappdata->music_path);
      pd->write_audio_files_path(pappdata->audio_files_path);
      path = pd->get_user_config_path();
      pd->write(path + string("dialogs/save_openFLTK/") + string(CONFIG_FILE_NAME));
      delete pd;
    }
}

void close_function(void *pdata)
{
  t_app_data *papp_data = (t_app_data*)pdata;
  int i;

  save_coordinates_n_music_path(papp_data);
  usleep(200000); // Wait for the last transmisisn to be completed or something linke that??????????????????????
  papp_data->ptcpclient->close_function(string(STORAGE_DIALOG_CODE_NAME));
  usleep(100000);
  i = 0;
  while (papp_data->ptcpclient->get_state() != state_closed && i < 20)
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
#ifdef _DEBUG
  printf("close_callback called\n" );
#endif
  close_function(pdata);
}

void show_file_selection_dialog(t_app_data *papp_data, int type, bool& bcancel, Fl_Native_File_Chooser *pfnfc, string& path)
{
  const int cstrsz = 4096;
  char      str[cstrsz];
  int       i;

  bcancel = false;
  pfnfc->type(type);
  switch (pfnfc->show())
    {
    case -1:
      printf("ERROR: %s\n", pfnfc->errmsg());
      break; // ERROR
    case 1:
      //printf("CANCEL\n" );
      bcancel = true;
      break; // CANCEL
    default:
      strncpy(str, pfnfc->filename(), cstrsz);
      for (i = strlen(str) - 1; i > 0; i--)
	{
	  if (str[i] == '\\' || str[i] == '/')
	    {
	      str[i] = 0;
	      i = 0;
	    }
	}
      papp_data->bpath_changed = true;
      path = string(str);
      papp_data->filename = string(pfnfc->filename());
#ifdef _DEBUG
      printf("PICKED: %s\n" , papp_data->filename.c_str());
#endif
      break; // FILE CHOSEN
    }
}

using namespace scmsg;

void quicksave_callback(Fl_Widget *pwi, void *pdata)
{
  t_app_data     *papp_data = (t_app_data*)pdata;
  string          message;
  bool            bcancel;
  Cmessage_coding coder;
  int             size;
  const int       cmsgsize = 4096;
  char            msg[cmsgsize];
  string          path("");

  if (papp_data->filename.length() == 0)
    {
      show_file_selection_dialog(papp_data, Fl_Native_File_Chooser::BROWSE_FILE, bcancel, papp_data->pfnfc, papp_data->music_path);
    }
  if (!bcancel)
    {
      message = coder.create_file_os_message(path, papp_data->filename, efilesave);
      if (coder.build_wire_message(msg, cmsgsize, &size, message))
	{
	  papp_data->ptcpclient->Send(msg, size);
	}
    }
}

void save_callback(Fl_Widget *pwi, void *pdata)
{
  t_app_data     *papp_data = (t_app_data*)pdata;
  string          message;
  bool            bcancel;
  Cmessage_coding coder;
  int             size;
  const int       cmsgsize = 4096;
  char            msg[cmsgsize];
  string          path("");

  //papp_data->pfnfc->options(Fl_Native_File_Chooser::NEW_FOLDER);
  show_file_selection_dialog(papp_data, Fl_Native_File_Chooser::BROWSE_SAVE_FILE, bcancel, papp_data->pfnfc, papp_data->music_path);
  if (!bcancel)
    {
      message = coder.create_file_os_message(path, papp_data->filename, efilesave);
      if (coder.build_wire_message(msg, cmsgsize, &size, message))
	{
	  papp_data->ptcpclient->Send(msg, size);
	}
      close_function(pdata);
    }
}

void load_callback(Fl_Widget *pwi, void *pdata)
{
  t_app_data     *papp_data = (t_app_data*)pdata;
  string          message;
  bool            bcancel;
  Cmessage_coding coder;
  int             size;
  const int       cmsgsize = 4096;
  char            msg[cmsgsize];
  string          path("");

  show_file_selection_dialog(papp_data, Fl_Native_File_Chooser::BROWSE_FILE, bcancel, papp_data->pfnfc, papp_data->music_path);
  if (!bcancel)
    {
      message = coder.create_file_os_message(path, papp_data->filename, efileopen);
      if (coder.build_wire_message(msg, cmsgsize, &size, message))
	{
	  papp_data->ptcpclient->Send(msg, size);
	}
      close_function(pdata);
    }
}

void new_empty_project_callback(Fl_Widget *pwi, void *pdata)
{
  t_app_data     *papp_data = (t_app_data*)pdata;
  string          message;
  Cmessage_coding coder;
  int             size;
  const int       cmsgsize = 4096;
  char            msg[cmsgsize];
  string          path("");
  string          file_name("");

  if (fl_choice("Do you really want to restart from scratch?", fl_no, fl_yes, 0))
    {
      message = coder.create_file_os_message(path, file_name, enewfile);
      if (coder.build_wire_message(msg, cmsgsize, &size, message))
	{
	  papp_data->ptcpclient->Send(msg, size);
	}
      close_function(pdata);
    }
}

void new_project_from_audio_file_callback(Fl_Widget *pwi, void *pdata)
{
  t_app_data     *papp_data = (t_app_data*)pdata;
  string          message;
  Cmessage_coding coder;
  int             size;
  const int       cmsgsize = 4096;
  char            msg[cmsgsize];
  string          path("");
  bool            bcancel;

  show_file_selection_dialog(papp_data, Fl_Native_File_Chooser::BROWSE_FILE, bcancel, papp_data->pfnfc_audiofile, papp_data->audio_files_path);
  if (!bcancel)
    {
      message = coder.create_file_os_message(path, papp_data->filename, enewfromfile);
      if (coder.build_wire_message(msg, cmsgsize, &size, message))
	{
	  papp_data->ptcpclient->Send(msg, size);
	}
      close_function(pdata);
    }
}

void set_button(Fl_Button *pb, void* puser_data)
{
  pb->type(FL_NORMAL_BUTTON);
  pb->labelsize(21);
  pb->user_data(puser_data);
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
  int         wx, wy;
  int         yoffset;
  int         xoffset;
  int         buttonh;
  int         buttonoffset;
  Fl_Window  *window;
  Fl_Button  *but_save;
  Fl_Button  *but_load;
  Fl_Button  *but_quicksave;
  Fl_Button  *but_new;
  Fl_Button  *but_new_from_file;
  Fl_Native_File_Chooser *pfnfc;
  Fl_Native_File_Chooser *pfnfc_audiofile;
  t_app_data  app_data;

  if (argc > 1)
    app_data.path = string(argv[1]);
  else
    app_data.path = string("./");
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
  Fl_File_Icon::load_system_icons();
  //
  read_window_pos_n_music_path(&app_data);
  wx = 300;
  wy = 400;
  window = new Fl_Window(app_data.xpos, app_data.ypos, wx, wy, "load/save audio and score");
  //window = new Fl_Window(wx, wy, "load/save audio and score");
  pfnfc = new Fl_Native_File_Chooser;
  pfnfc->title("Pick a score file");
  pfnfc->filter("scores\t*.xml\n"
		"Score Files\t*.{xml}");
  pfnfc->directory(app_data.music_path.c_str());
  app_data.bpath_changed = false;
  app_data.pfnfc = pfnfc;
  // Used to load wav, flac
  pfnfc_audiofile = new Fl_Native_File_Chooser;
  pfnfc_audiofile->title("Pick a file");
  pfnfc_audiofile->filter("wav files\t*.wav\t flac files 16bits 44.1khz\t*.flac");
  pfnfc_audiofile->directory(app_data.audio_files_path.c_str());
  app_data.pfnfc_audiofile = pfnfc_audiofile;
  //
  app_data.pwindow = window;
  app_data.filename = string("");
  window->begin();
  xoffset = 10;
  yoffset = 20;
  buttonh = (wy - 2 * yoffset) / 6;
  buttonoffset = yoffset;
  //
  but_save      = new Fl_Button(xoffset, buttonoffset, wx - xoffset * 2, buttonh, "Save as");
  buttonoffset += buttonh + 10; 
  set_button(but_save, &app_data);
  but_save->callback(save_callback);
  //
  but_quicksave = new Fl_Button(xoffset, buttonoffset, wx - xoffset * 2, buttonh, "Quick Save");
  buttonoffset += buttonh + 10;
  set_button(but_quicksave, &app_data);
  but_quicksave->callback(quicksave_callback);
  //
  but_load      = new Fl_Button(xoffset, buttonoffset, wx - xoffset * 2, buttonh, "Load");
  buttonoffset += buttonh + 10 + 20;
  set_button(but_load, &app_data);
  but_load->callback(load_callback);
  //
  but_new       = new Fl_Button(xoffset, buttonoffset, wx - xoffset * 2, buttonh, "New and Empty project");
  buttonoffset += buttonh + 10;
  set_button(but_new, &app_data);
  but_new->callback(new_empty_project_callback);
  //
  but_new_from_file = new Fl_Button(xoffset, buttonoffset, wx - xoffset * 2, buttonh, "New from audio file");
  buttonoffset += buttonh + 10;
  set_button(but_new_from_file, &app_data);
  but_new_from_file->callback(new_project_from_audio_file_callback);
  //
  window->end();
  //Fl::add_handler(wincall);
  //Fl::user_data(&app_data);
  window->callback(wincall);
  window->user_data(&app_data);
  argc = 1;
  window->show(argc, argv);
  Fl::scheme("gtk+");
  create_threads(&app_data);
  Send_dialog_opened_message(&app_data, string(STORAGE_DIALOG_CODE_NAME));
  return(Fl::run());
}

