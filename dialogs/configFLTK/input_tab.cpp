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

#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Choice.H>
#include <FL/Fl_Check_Button.H>

#include "audioapi.h"
#include "score.h"
#include "messages.h"
#include "message_decoder.h"
#include "tcpclient.h"
#include "main.h"

using namespace std;
using namespace scmsg;

#define VERBOSE

void io_ok_callback(Fl_Widget *pwi, void *pdata)
{
  t_app_data                *papp_data = (t_app_data*)pdata;
  const int                  cmsgsize = 4096;
  char                       msg[cmsgsize];
  int                        size;
  Cmessage_coding            coder;
  string                     str;
  std::list<t_portaudio_api> apis; // empty unused

  str = coder.create_audioIO_config_message(&apis, &papp_data->pa_chan_sel);
  if (coder.build_wire_message(msg, cmsgsize, &size, str))
    {
      papp_data->ptcpclient->Send(msg, size);
    }
  close_function(pdata);
}

void fill_devs_list(t_app_data *papp_data, std::list<t_pa_device> *device_list, int *pselvalue, bool bin)
{
  int                              i;
  std::list<t_pa_device>::iterator dev_iter;
  Fl_Choice                       *pchoice;
  std::string                      sel_name;

  *pselvalue = 0;
  if (bin)
    {
      pchoice = papp_data->input_device_choice;
      sel_name = papp_data->pa_chan_sel.in_device_name;
    }
  else
    {
      pchoice = papp_data->output_device_choice;
      sel_name = papp_data->pa_chan_sel.out_device_name;      
    }
  pchoice->clear();
  dev_iter = device_list->begin();
  i = 0;
  while (dev_iter != device_list->end())
    {
      if ((bin && (*dev_iter).inputs > 0) ||
	  (!bin && (*dev_iter).outputs > 0))
	{
#ifdef VERBOSE
	  printf("Adding %s device \"%s\" - %d\n", bin? "input" : "output", (*dev_iter).name.c_str(), i);
#endif
	  pchoice->add((*dev_iter).name.c_str());
	  if ((*dev_iter).name == sel_name)
	    {
	      *pselvalue = i;
	    }
	  i++;
	}
      dev_iter++;
    }
}

void fill_lists(t_app_data *papp_data)
{
  std::list<t_portaudio_api>::iterator api_iter;
  int i;
  int api_value;
  int input_value;
  int output_value;

  api_value = 0;
  input_value = 0;
  output_value = 0;
  i = 0;
  api_iter = papp_data->pa_dev_list.begin();
  while (api_iter != papp_data->pa_dev_list.end())
    {
#ifdef VERBOSE
      printf("Adding api \"%s\"\n", (*api_iter).name.c_str());
#endif
      papp_data->audio_api_choice->add((*api_iter).name.c_str());
      if ((*api_iter).name == papp_data->pa_chan_sel.api_name)
	{
	  api_value = i;
	  fill_devs_list(papp_data, &(*api_iter).device_list, &input_value, true);
	  fill_devs_list(papp_data, &(*api_iter).device_list, &output_value, false);
	}
      i++;
      api_iter++;
    }
  papp_data->audio_api_choice->value(api_value);
  papp_data->input_device_choice->value(input_value);
  papp_data->output_device_choice->value(output_value);
}

t_portaudio_api *get_api_list(t_app_data *papp_data, int api_value)
{
  int  i;
  std::list<t_portaudio_api>::iterator api_iter;

  api_iter = papp_data->pa_dev_list.begin();
  i = 0;
  while (api_iter != papp_data->pa_dev_list.end())
    {
      if (i == api_value)
	return &(*api_iter);
      i++;
      api_iter++;
    }
  return NULL;
}

std::string get_nth_elt_name(t_portaudio_api *apiptr, bool binput, int selval)
{
  int                              i;
  std::list<t_pa_device>::iterator dev_iter;
  std::string                      sel_name;

  sel_name = string("");
  dev_iter = apiptr->device_list.begin();
  i = 0;
  while (dev_iter != apiptr->device_list.end())
    {
      if ((binput && (*dev_iter).inputs > 0) ||
	  (!binput && (*dev_iter).outputs > 0))
	{
	  if (i == selval)
	    {
	      sel_name = (*dev_iter).name;
	    }
	  i++;
	}
      dev_iter++;
    }
  return sel_name;
}

void get_choices(t_app_data *papp_data, bool bonly_api)
{
  t_portaudio_api *apiptr;
  int api;
  int in;
  int out;

  api = papp_data->audio_api_choice->value();
  in  = papp_data->input_device_choice->value();
  out = papp_data->output_device_choice->value();
  apiptr = get_api_list(papp_data, api);
  if (apiptr)
    {
      papp_data->pa_chan_sel.api_name = apiptr->name;
      if (bonly_api)
	{
	  return;
	}
      papp_data->pa_chan_sel.in_device_name = get_nth_elt_name(apiptr, true, in);
      papp_data->pa_chan_sel.out_device_name = get_nth_elt_name(apiptr, false, out);
#ifdef VERBOSE
      printf("Selected \"%s\":\n", papp_data->pa_chan_sel.api_name.c_str());
      printf("In  =\"%s\"\n", papp_data->pa_chan_sel.in_device_name.c_str());
      printf("Out =\"%s\"\n", papp_data->pa_chan_sel.out_device_name.c_str());
#endif
    }
  else
    printf("Error: could not retreive the audio api and device interfaces names.\n");
}

void audio_api_callback(Fl_Widget *pwi, void *pdata)
{
  t_app_data      *papp_data = (t_app_data*)pdata;
  t_portaudio_api *apiptr;

  printf("api choice == %d\n", papp_data->audio_api_choice->value());
  get_choices(papp_data, true);
  apiptr = get_api_list(papp_data, papp_data->audio_api_choice->value());
  if (apiptr)
    {
      fill_lists(papp_data);
    }
  get_choices(papp_data, false);
}

void output_device_callback(Fl_Widget *pwi, void *pdata)
{
  t_app_data *papp_data = (t_app_data*)pdata;

  get_choices(papp_data, false);
}

void input_device_callback(Fl_Widget *pwi, void *pdata)
{
  t_app_data *papp_data = (t_app_data*)pdata;

  get_choices(papp_data, false);
}

void create_sound_input_tab(int x, int y, int w, int h, void *pdata)
{
  t_app_data *papp_data = (t_app_data*)pdata;
  int         border;
  int         buttonw;
  int         buttonh;
  int         text_width;

  Fl_Group *group1 = new Fl_Group(x, y, w, h, "sound io");
  {
    border = 10;
    buttonw = 300;
    buttonh = 50;
    text_width = 120;
    papp_data->audio_api_choice = new Fl_Choice(x + text_width + border, y + border, buttonw, 3 * border,                   "Audio API       ");
    papp_data->audio_api_choice->value(papp_data->bstartrecord);
    papp_data->audio_api_choice->user_data(papp_data);
    papp_data->audio_api_choice->callback(audio_api_callback);
    papp_data->input_device_choice = new Fl_Choice(x + text_width + border, y + border + 2 * buttonh, buttonw, 3 * border,  "Input Device    ");
    papp_data->input_device_choice->value(papp_data->bstartrecord);
    papp_data->input_device_choice->user_data(papp_data);
    papp_data->input_device_choice->callback(input_device_callback);
    papp_data->output_device_choice = new Fl_Choice(x + text_width + border, y + border + 4 * buttonh, buttonw, 3 * border, "Output Device   ");
    papp_data->output_device_choice->value(papp_data->bstartrecord);
    papp_data->output_device_choice->user_data(papp_data);
    papp_data->output_device_choice->callback(output_device_callback);
    //
    fill_lists(papp_data);
    //
    buttonw = 90;
    buttonh = 50;
    papp_data->but_ok = new Fl_Button(x + w - buttonw - border, y + h - buttonh - 2 * border, buttonw, buttonh, "Ok");
    set_button(papp_data->but_ok, papp_data);
    papp_data->but_ok->callback(io_ok_callback);
    //
    papp_data->but_cancel = new Fl_Button(x + w - 2 * (buttonw + border), y + h - buttonh - 2 * border, buttonw, buttonh, "Cancel");
    set_button(papp_data->but_cancel, papp_data);
    papp_data->but_cancel->callback(cancel_callback);
  }
  group1->end();
}

