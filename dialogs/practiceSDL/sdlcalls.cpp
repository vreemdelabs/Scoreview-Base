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
#include <iterator>
#include <list>
#include <vector>

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

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h> 

#include <GL/gl.h>

#include "messages.h"
#include "gfxareas.h"
#include "mesh.h"
#include "gl2Dprimitives.h"
#include "pictures.h"
#include "score.h"
#include "f2n.h"
#include "keypress.h"
#include "tcpclient.h"
#include "fingerender.h"
#include "prenderer.h"
#include "vrender.h"
#include "grender.h"
#include "shared.h"
#include "app.h"
#include "sdlcalls.h"

#define MIN_WINDOW_W 320
#define MIN_WINDOW_H 200

int init_SDL_image()
{
  int imgFlags = IMG_INIT_PNG;

  if (!(IMG_Init(imgFlags) & imgFlags))
    {
      printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
      exit(EXIT_FAILURE);
      //return 0;
    }
  return 0;
}

int init_SDL(SDL_Window **psdlwindow, SDL_GLContext *GLContext, int x, int y, int width, int height)  
{
  atexit(SDL_Quit);
  SDL_Init(SDL_INIT_VIDEO);
  SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
  //Use OpenGL 2.1
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  //
  // Multisaple
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
  SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
  //
  *psdlwindow = SDL_CreateWindow("ScoreView Practice",
				 x,
				 y,
				 width, height,
				 /*SDL_WINDOW_FULLSCREEN | */SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
  if (*psdlwindow == NULL)
    {
      fprintf(stderr, "SDL video initialization failed: %s\n", SDL_GetError( ));
      exit(EXIT_FAILURE);
    }
  SDL_SetWindowMinimumSize(*psdlwindow, MIN_WINDOW_W, MIN_WINDOW_H);
  //SDL_ShowCursor(SDL_DISABLE);
  SDL_ShowCursor(SDL_ENABLE);
  // OpenGL cotext creation thanks to the SDL
  *GLContext = SDL_GL_CreateContext(*psdlwindow);
  if (SDL_GL_MakeCurrent(*psdlwindow, *GLContext) != 0)
    {
      fprintf(stderr, "OpenGL context initialisation failed: %s\n", SDL_GetError( ));
      exit(EXIT_FAILURE);
    }
  //SDL_ShowCursor(SDL_DISABLE);
  SDL_ShowCursor(SDL_ENABLE);
  return (init_SDL_image());
}

int close_SDL(SDL_Window *psdlwindow, SDL_GLContext GLContext)
{
  SDL_GL_DeleteContext(GLContext);
  SDL_DestroyWindow(psdlwindow);
  SDL_Quit();
  return 0;
}

void inject_time_pulse(double& last_time_pulse)
{
  // get current "run-time" in seconds
  double t = 0.001 * SDL_GetTicks();

  // store the new time as the last time
  last_time_pulse = t;
}

void handle_mouse_down(int button)
{
}

void handle_mouse_up(int button)
{
}

void handle_input_events(Cappdata *app)
{
  t_shared_data      *pshared_data = (t_shared_data*)&app->m_shared_data;
  SDL_Event           e;
  SDL_MouseWheelEvent w; 
  int                 x, y;

  app->setlooptime();
  //SDL_WaitEvent(&event);
  while (SDL_PollEvent(&e))
    {
      switch (e.window.event)
	{
	case SDL_WINDOWEVENT_SIZE_CHANGED:
	  {
	    int width  = e.window.data1;
	    int height = e.window.data2;
	    app->change_window_size(width, height);
	    //printf("window size changed to: %d, %d\n", width, height);
            app->m_gfxprimitives->SetLogicalSize(width, height);
	  }
	  break;
	default:
	  break;
	};
      switch (e.type)
	{
	case SDL_MOUSEWHEEL:
	  // Find if it is on a window
	  SDL_GetMouseState(&x, &y);
	  memcpy(&w, &e, sizeof(w));
	  //printf("mousewheel x=%d y=%d px %d py %d\n", w.x, w.y, x, y);
	  app->handle_weel(x, y, w.y);
	  break;
	case SDL_FINGERMOTION:
	  printf("fingermotion\n");
	  break;
	  // mouse motion handler 
	case SDL_MOUSEMOTION:
	  SDL_GetMouseState(&x, &y);
	  app->mousemove(x, y);
	  break;
	  // mouse down handler
	case SDL_MOUSEBUTTONDOWN:
	  SDL_GetMouseState(&x, &y);
	  switch (e.button.button)
	    {
	    case SDL_BUTTON_RIGHT:
	      break;
	    case SDL_BUTTON_LEFT:
	      app->mouseclickleft(x, y);
	      break;
	    default:
	      break;
	    }
	  break;
	  // mouse up handler
	case SDL_MOUSEBUTTONUP:
	  SDL_GetMouseState(&x, &y);
	  switch (e.button.button)
	    {
	    case SDL_BUTTON_RIGHT:
	      break;
	    case SDL_BUTTON_LEFT:
	      app->mouseupleft(x, y);
	      break;
	    default:
	      break;
	    }
	  break;
	  // key down
	case SDL_KEYDOWN:
	  //printf("%c %c\n", e.key.keysym.sym, SDL_GetScancodeFromKey(e.key.keysym.sym));
	  //switch (e.key.keysym.scancode)
	  app->key_on(e.key.keysym.sym);
	  break;
	  // key up
	case SDL_KEYUP:
	  app->key_off(e.key.keysym.sym);
	  break;
	  // WM quit event occured
	case SDL_QUIT:
	  printf("Quit event receives\n");
	  pthread_mutex_lock(&pshared_data->data_mutex);
	  pshared_data->bquit = true;
	  pthread_mutex_unlock(&pshared_data->data_mutex);
	  break;
	/*
	case SDL_VIDEORESIZE:
	  printf("Resize video input\n");
	  break;*/
	}
    }
}

