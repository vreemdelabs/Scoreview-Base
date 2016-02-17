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

#include <ctime>
#include <iterator>
#include <list>
#include <vector>

#ifdef __LINUX
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#ifdef __ANDROID__
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#else
#include <winsock2.h>
#include <windows.h>
#endif //__ANDROID__
#endif //__LINUX

#ifdef __ANDROID__
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <SDL_net.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_net.h>
#include <GL/gl.h>
#endif

#include "messages.h"
#include "gfxareas.h"
#include "mesh.h"
#include "gl2Dprimitives.h"
#include "pictures.h"
#include "score.h"
#include "f2n.h"
#include "keypress.h"
#include "tcp_message_receiver.h"
#include "tcpclientSDLnet.h"
#include "shared.h"
#include "fingerender.h"
#include "prenderer.h"
#include "sirender.h"
#include "vrender.h"
#include "grender.h"
#include "app.h"
#include "sdlcalls.h"

using namespace std;

void Cappdata::handle_weel(int x, int y, int v)
{
}

void Cappdata::key_on(int code)
{
  //t_shared_data   *pshared_data = (t_shared_data*)&m_shared_data;

  //m_kstates.key_on(code);
  // Space bar = play/pause
  //printf("%c on\n", code);
}

void Cappdata::key_off(int code)
{
  //m_kstates.key_off(code);
}

void Cappdata::mouseclickleft(int x, int y)
{
/*
  t_shared_data *pshared_data = (t_shared_data*)&m_shared_data;
  Cgfxarea *pw;
  t_coord  p;
  t_coord  pos;
  t_coord  dim;
  
  printf("clickleft ");
  p.x = x;
  p.y = y;
  pw = m_layout->get_first();
  while (pw != NULL)
    {
      if (strcmp(pw->m_name, "main") == 0 && pw->is_in(p))
	{
	}
      pw = m_layout->get_next();
    }
  printf("\n");
*/
}

void Cappdata::mousemove(int x, int y)
{
/*
  t_shared_data   *pshared_data = (t_shared_data*)&m_shared_data;
  Cgfxarea   *pw;
  t_coord    p;
  double     t1, t2;

  if (m_layout->is_in(x, y, (char*)"main"))
    {
    }
*/
  m_ccurrent.x = x;
  m_ccurrent.y = y;
}

void Cappdata::mirror(int mouse_x, int mouse_y)
{
  std::list<CInstrument*>::iterator iter;
  Cgfxarea *pw;
  t_coord   pos;

  pw = m_layout->get_area((char*)"mirror");
  if (pw != NULL)
    {
      pos.x = mouse_x;
      pos.y = mouse_y;
      m_mirror = pw->is_in(pos)? !m_mirror : m_mirror;
    }
}

void Cappdata::mouseupleft(int x, int y)
{
  //t_shared_data    *pshared_data = (t_shared_data*)&m_shared_data;

  // x, y are global values if the mouse is outside the window
  // therefore use the last values from mous move
  x = m_ccurrent.x;
  y = m_ccurrent.y;
  mirror(x, y);
}

