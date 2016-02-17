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
#include <malloc.h>
#include <memory.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include <iterator>
#include <list>
#include <vector>

#include <tinyxml.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include "audiodata.h"
#include "audioapi.h"
#include "shared.h"
#include "gfxareas.h"
#include "mesh.h"
#include "gl2Dprimitives.h"

#include "cfgfile.h"
#include "score.h"
#include "f2n.h"
#include "pictures.h"
#include "keypress.h"
#include "instrumenthand.h"
#include "scoreplacement.h"
#include "scoreplacement_violin.h"
#include "scoreplacement_piano.h"
#include "scoreplacement_guitar.h"
#include "scoreedit.h"
#include "scorerenderer.h"
#include "card.h"
#include "messages.h"
#include "tcp_message_receiver.h"
#include "messages_network.h"
#include "app.h"
#include "sdlcalls.h"
#include "threads.h"

#ifndef _DEBUG
#define USE_LOG_FILE
#endif

void main_loop(Cappdata *app)
{
  bool must_quit = false;

  while (!must_quit)
    {
      handle_input_events(&must_quit, app);
      app->timed_events();
      app->render_gui();
      usleep(7000);
    }
}

#ifdef USE_LOG_FILE
void open_log_file()
{
  Cxml_cfg_file_decoder cfgfile;
  string                user_home;

  user_home = cfgfile.get_user_config_path() + string("applog.txt");
  freopen(user_home.c_str(), "w", stdout);
}

void close_log_file()
{
#ifndef __LINUX
  freopen("CON", "w", stdout);
#else
  fclose(stdout);
#endif
}
#else
void open_log_file()
{
}
 
void close_log_file()
{
}
#endif

int main(int argc, char **argv)
{
  int            width;
  int            height;
  Cappdata      *app;

  width = 1024 + 12;
  height = 512 + 256 + 64;
  open_log_file();
  // Central class
  app = new Cappdata(width, height);
  if (app->init_the_network_messaging() == 0)
    {
      // Create threads
      if (create_threads(app) == 0)
	{
	  // Just loop
	  main_loop(app);
	  // Destroy threads
	  release_threads(app);
	}
    }
  delete app;
  close_log_file();
  return 0;
}

