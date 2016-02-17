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
//#include <iostream>
#include <unistd.h>
#include <string>
#include <list>
#include <vector>

#include <time.h>
#include <pthread.h>

#include <errno.h>
#ifdef __LINUX
//#include <sys/types.h>
//#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//#include <syslog.h>
//#include <signal.h>
#else
#include <winsock2.h>
#include <windows.h>
#endif //__LINUX

#include <event2/dns.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/util.h>
#include <event2/event.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/gl.h>

#include "dialogpath.h"
#include "keypress.h"
#include "gfxareas.h"
#include "mesh.h"
#include "gl2Dprimitives.h"
#include "f2n.h"
#include "score.h"
#include "pictures.h"
#include "fingerender.h"
#include "prenderer.h"
#include "vrender.h"
#include "grender.h"
#include "messages.h"
#include "tcp_ip.h"
#include "tcpclient.h"
#include "shared.h"
#include "app.h"
#include "sdlcalls.h"
#include "main.h"
#include "threads.h"

using namespace std;

void main_loop(Cappdata *app)
{
  while (!app->m_shared_data.bquit)
    {
      //printf("loop\n");
      handle_input_events(app);
      app->get_network_messages();
      app->render_gui();
      usleep(20000);
    }
}

int main(int argc, char **argv)
{
  int            width;
  int            height;
  Cappdata       *app;
  string         path;

  if (argc > 1)
    path = string(argv[1]);
  else
    path = string("./");
  width = 1024;
  height = (float)width * 0.588;
  // Central class
  app = new Cappdata(path, width, height);
  if (app->init_the_network_messaging() == 0)
    {
      // Create threads
      if (create_threads(&app->m_shared_data) == 0)
	{
	  //app->Send_dialog_opened_message(&app->m_shared_data, string(INSTRUMENT_VISUAL_DIALOG_CODENAME));
	  // Just loop
	  main_loop(app);
	  app->m_shared_data.ptcpclient->close_function(string(PRACTICE_DIALOG_CODENAME));
	  // Destroy threads
	  release_threads(&app->m_shared_data);
	}
    }
  delete app;
  return 0;
}

