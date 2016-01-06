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
 
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h> 

#include <GL/gl.h>
//#include <GL/glu.h>

#include "audiodata.h"
#include "audioapi.h"
#include "shared.h"
#include "gfxareas.h"
#include "mesh.h"
#include "gl2Dprimitives.h"
#include "pictures.h"
#include "score.h"
#include "f2n.h"
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
#include "messages_network.h"
#include "app.h"
#include "sdlcalls.h"

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
  *psdlwindow = SDL_CreateWindow("ScoreView (beta release dec 2015)",
				 x,
				 y,
				 width, height,
				 SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
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
  SDL_GL_SetSwapInterval(1); // Vsync
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

int register_sdl_user_event()
{
  return SDL_RegisterEvents(1);
}

void handle_mouse_down(int button)
{
}

void handle_mouse_up(int button)
{
}

void handle_input_events(bool *pmustquit, Cappdata *app)
{
  t_shared            *pshared_data = (t_shared*)&app->m_shared_data;
  SDL_Event           e;
  SDL_MouseWheelEvent w;
  int                 x, y;
  double              last_time_pulse;
  int                 width;
  int                 height;
  bool                bmousemove;

  inject_time_pulse(last_time_pulse);
  app->setlooptime(last_time_pulse);
  bmousemove = false;
  //SDL_WaitEvent(&event);
  while (SDL_PollEvent(&e))
    {
      if ((int)e.type == app->m_pserver->get_sdl_dialog_user_event_code())
	{
	  // Messages from the netowrk external dialogs
	  app->process_network_messages();
	}
      if ((int)e.type == app->m_shared_data.audio_cmd_sdlevent)
	{
	  app->process_audio_events();
	}
      switch (e.window.event)
	{
/*
        case SDL_WINDOWEVENT_RESIZED:
	  {
	    width  = e.window.data1;
	    height = e.window.data2;
	    printf("window resized to: %d, %d\n", width, height);
	  }
	  break;
        case SDL_WINDOWEVENT_MINIMIZED:
	  printf("window minimised.\n");
	  break;
        case SDL_WINDOWEVENT_MAXIMIZED:
	  printf("window maxiimised.\n");
	  break;
*/
	case SDL_WINDOWEVENT_SIZE_CHANGED:
	  {
	    width  = e.window.data1;
	    height = e.window.data2;
	    app->change_window_size(width, height);
	    //printf("window size changed to: %d, %d\n", width, height);
	    app->m_gfxprimitives->SetLogicalSize(width, height);
	    //SDL_RenderSetLogicalSize( sdlRenderer, width, height);
	    //SDL_RenderGetLogicalSize(app->m_sdlRenderer, &width, &height); // Coordinate bug when maximising on gnome
	    //printf("new size is %d %d\n", width, height);
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
	  bmousemove = true;
	  break;
	case SDL_FINGERMOTION:
	  printf("fingermotion\n");
	  break;
	  // mouse motion handler 
	case SDL_MOUSEMOTION:
	  SDL_GetMouseState(&x, &y);
	  app->mousemove(x, y);
	  bmousemove = true;
	  break;
	  // mouse down handler
	case SDL_MOUSEBUTTONDOWN:
	  SDL_GetMouseState(&x, &y);
	  switch (e.button.button)
	    {
	    case SDL_BUTTON_RIGHT:
	      app->mouseclickright(x, y);
	      break;
	    case SDL_BUTTON_LEFT:
	      app->mouseclickleft(x, y);
	      break;
	    default:
	      break;
	    }
	  bmousemove = true;
	  break;
	  // mouse up handler
	case SDL_MOUSEBUTTONUP:
	  SDL_GetMouseState(&x, &y);
	  switch (e.button.button)
	    {
	    case SDL_BUTTON_RIGHT:
	      app->mouseupright(x, y);
	      break;
	    case SDL_BUTTON_LEFT:
	      app->mouseupleft(x, y);
	      break;
	    default:
	      break;
	    }
	  bmousemove = true;
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
	  printf("Quit event received\n");
	  LOCK;
	  pshared_data->bquit = true;
	  *pmustquit = pshared_data->bquit;
	  UNLOCK;
	  break;
	/*
	case SDL_VIDEORESIZE:
	  printf("Resize video input\n");
	  break;*/
	}
    }
  // Non moving mouse detection
  app->mousedidnotmove(bmousemove);
}

