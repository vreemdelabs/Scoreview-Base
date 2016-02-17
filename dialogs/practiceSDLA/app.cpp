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
#include <iterator>
#include <list>
#include <vector>

//#include <ctime>
#include <sys/time.h>

#include <errno.h>
#ifdef __LINUX
#include <netinet/in.h>
#include <arpa/inet.h>
#else
#ifdef __ANDROID__
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <android/log.h>
#else
#include <winsock2.h>
#include <windows.h>
#endif //__ANDROID__
#endif //__LINUX

#ifdef __ANDROID__
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_net.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_net.h>
#include <GL/gl.h>
#endif


#include "env.h"
#include "audioapi.h"
#ifndef __ANDROID__
#include <tinyxml.h>
#include "cfgfile.h"
#endif
#include "score.h"
#include "messages.h"
#include "message_decoder.h"
#include "gfxareas.h"
#include "mesh.h"
#include "gl2Dprimitives.h"
#include "pictures.h"
#include "f2n.h"
#include "keypress.h"
#include "stringcolor.h"
#include "tcp_ip.h"
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

Cappdata::Cappdata(string path, int width, int height):
  m_path(path),
  m_width(width),
  m_height(height),
  m_pscore(NULL),
  m_start_daytime(-1),
  m_viewtime(4.),
  m_play_timecode(0),
  m_play_end_timecode(0),
  m_note_higlight_identifier(-1)
{
#ifdef PRINT_PATH
  const int csize = 4096;
  char      buf[csize];

  if (getcwd(buf, csize) == NULL)
    {
      printf("Path error.\n");
      exit(1);
    }
  else
    {
      printf("App path=\"%s\"\n", buf);
    }
#endif
#ifndef __ANDROID__
  //printf("old w=%d h=%d\n", m_width, m_height);
  read_window_pos(&m_window_xpos, &m_window_ypos);
  //printf("new w=%d h=%d\n", m_width, m_height);
#endif
  // Init gfx zones parameters
  create_layout();
  // Init all the SDL functions
  init_SDL(&m_sdlwindow, &m_GLContext, m_window_xpos, m_window_ypos, m_width, m_height);
  m_gfxprimitives = new CGL2Dprimitives(m_sdlwindow, m_GLContext, m_width, m_height);
  m_gfxprimitives->set_clear_color(0xFF000000);
  m_gfxprimitives->init_OpenGL();
  //
  m_picturelist = new CpictureList(m_gfxprimitives);
  m_picturelist->open_practice_drawings(string(IMGPATH) + string("images.png"));
  TTF_Init();
  m_font = TTF_OpenFont(DATAPATH MAINFONT, 14);
  if (m_font == NULL)
    {
      printf("Error: font \"%s\" is missing.\n", DATAPATH MAINFONT);
      exit(1);
    }
  m_bigfont = TTF_OpenFont(DATAPATH BIGFONT, 64);
  if (m_bigfont == NULL)
    {
      printf("Error: font \"%s\" is missing.\n", DATAPATH BIGFONT);
      exit(1);
    }
  m_mesh_list = new CMeshList();
  if (!m_mesh_list->Load_meshes_from((char*)DATAPATH, (char*)"vfboard.obj"))
    {
      printf("Dialog practice error: %svfboard.obj missing\n", (char*)DATAPATH);
      exit(1);
    }
  if (!m_mesh_list->Load_meshes_from((char*)DATAPATH, (char*)"gfboard.obj"))
    {
      printf("Dialog practice error: %sgfboard.obj missing\n", (char*)DATAPATH);
      exit(1);
    }
  // Init all the mutexes from the shared data
  pthread_mutex_init(&m_shared_data.data_mutex, NULL);
  m_shared_data.ptcpclient = NULL;
  m_shared_data.bquit = false;
  // Empty score
  m_pscore = NULL;
  m_pcurrent_instrument = NULL; //m_pscore->get_first_instrument();
  m_instrument_identifier = -1;
  // Instrument visuals
  m_null_renderer   = new CFingerRenderer(m_gfxprimitives, m_font, m_viewtime);
  m_piano_renderer  = new CpRenderer(m_gfxprimitives, m_font, m_viewtime);
  m_violin_renderer = new CvRenderer(m_gfxprimitives, m_font, m_viewtime, m_mesh_list);
  m_guitar_renderer = new CgRenderer(m_gfxprimitives, m_font, m_viewtime, m_mesh_list);
  m_prenderer = m_null_renderer;
  setlooptime();
  m_state = state_waiting;
}

Cappdata::~Cappdata()
{
  int xpos, ypos;

  delete m_null_renderer;
  delete m_piano_renderer;
  delete m_violin_renderer;
  delete m_guitar_renderer;
  delete m_shared_data.ptcpclient;
  delete m_pscore;
  // Release the mutexes
  pthread_mutex_destroy(&m_shared_data.data_mutex);
  // Close everything related to ui
  delete m_mesh_list;
  TTF_CloseFont(m_font);
  TTF_Quit();
  SDL_GetWindowPosition(m_sdlwindow, &xpos, &ypos);
#ifndef __ANDROID__
  save_window_coordinates(xpos, ypos);
#endif
  close_SDL(m_sdlwindow, m_GLContext);
  delete m_layout;
  delete m_picturelist;
  delete m_gfxprimitives;
}

#ifndef __ANDROID__
void Cappdata::read_window_pos(int *xpos, int *ypos)
{
  int                    x, y;
  Cxml_cfg_file_decoder *pd;
  string                 path, file;

  *xpos = *ypos = SDL_WINDOWPOS_CENTERED;
  pd = new Cxml_cfg_file_decoder();
  path = pd->get_user_config_path();
  file = path + string("dialogs/practiceSDL/") + string(CONFIG_FILE_NAME);
  printf("opening \"%s\".\n", file.c_str());
  if (pd->open_for_reading(file))
    {
      if (pd->read_window_position(&x, &y))
	{
	  *xpos = x;
	  *ypos = y;
	}
      if (pd->read_window_size(&x, &y))
	{
	  m_width = x;
	  m_height = y;
	}
    }
  delete pd;
}

void Cappdata::save_window_coordinates(int xpos, int ypos)
{
  Cxml_cfg_file_decoder *pd;
  string                 path;

  //printf("position is %d,%d compared to %d,%d\n", m_window_xpos, m_window_ypos, xpos, ypos);
  pd = new Cxml_cfg_file_decoder();
  path = pd->get_user_config_path();
  pd->open_for_writing();
  pd->write_window_position(xpos, ypos);
  pd->write_window_size(m_width, m_height);
  pd->write(path + string("dialogs/practiceSDL/") + string(CONFIG_FILE_NAME));
  delete pd;
}
#endif //__ANDROID__

void Cappdata::setlooptime()
{
  m_last_time = (double)0.001 * SDL_GetTicks();
}

int Cappdata::init_the_network_messaging()
{
  m_shared_data.ptcpclient = new CTCP_Client((void*)&m_shared_data, std::string(SERVER_HOST), SCOREVIEW_PORT);
  if (!m_shared_data.ptcpclient->Server_init())
    {
      printf("Dialog error: the tcp client side failed to initialise.\n");
      return 1;
    }
  if (!m_shared_data.ptcpclient->Connect())
    {
#ifdef __ANDROID__
      __android_log_print(ANDROID_LOG_ERROR, "Scoreview", "Dialog error: connection to the main app failed.\n");
#else
      printf("Dialog error: connection to the main app failed.\n");
#endif
      return 1;
    }
  Send_dialog_opened_message(&m_shared_data, string(PRACTICE_DIALOG_CODENAME));
  return 0;
}

void Cappdata::set_coord(t_fcoord *coord, float prx, float pry)
{
  coord->x = prx;
  coord->y = pry;
}

void Cappdata::create_layout()
{
  Cgfxarea *pw;
  t_fcoord  pos;
  t_fcoord  dimensions;
  t_coord   wsize;

  wsize.x = m_width;
  wsize.y = m_height;
  m_layout = new CgfxareaList();
  // Mirror "button"
  set_coord(&pos, 94., 2.);
  set_coord(&dimensions, 5., 5.);
  pw = new Cgfxarea(pos, dimensions, wsize, (char*)"mirror");
  m_layout->add(pw);
  // Main window
  set_coord(&pos, 0., 0.);
  set_coord(&dimensions, 100., 100.);
  pw = new Cgfxarea(pos, dimensions, wsize, (char*)"main");
  m_layout->add(pw);
  //
  change_window_size(m_width, m_height);
}

void Cappdata::Send_dialog_opened_message(t_shared_data *papp_data, string dialog_id)
{
  using namespace scmsg;
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

void Cappdata::draw_countdown(Cgfxarea *pw, double countdown_time)
{
  t_coord     pos, dim;
  t_fcoord    fpos, fdim;
  char        text[64];
  int         outline;

  pw->get_pos(&pos);
  pw->get_dim(&dim);
  sprintf(text, "%1.0f", ceil(fabs(COUNTDOWN - countdown_time)));
  fdim.x = m_width / 20;
  fdim.y = m_height / 20;
  fpos.x = (dim.x / 2) - (fdim.x / 2); // FIXMe middle of the screen
  fpos.y = (dim.y / 2) - (fdim.y / 2);
  fdim.x = m_width / 20;
  fdim.y = m_height / 20;
  outline = 2;
  renderTextBlended(m_bigfont, fpos, fdim, WHITE, text, true, outline, BLACK);
}

double Cappdata::read_day_time()
{
  struct timeval  tv;

  if (gettimeofday(&tv, NULL) != 0)
    {
      printf("Error: gettimeofday failed.\n");
      return -1;
    }
  return ((double)tv.tv_sec + (double)tv.tv_usec / 1000000.);
}

void Cappdata::change_window_size(int newWidth, int newHeight)
{
  t_coord wsize;

  wsize.x = newWidth;
  wsize.y = newHeight;
  m_width = newWidth;
  m_height = newHeight;
  m_layout->resize(wsize);
}

void Cappdata::renderTextBlended(TTF_Font *pfont, t_fcoord pos, t_fcoord dim, int color, string text, bool blended, float outline, int outlinecolor)
{
  char str[4096];

  strcpy(str, text.c_str());
  m_gfxprimitives->print(str, pfont, pos, dim, color, blended, outline, outlinecolor);
}

void Cappdata::render_gui()
{
  Cgfxarea   *pw;
  string      practice_msg;
  string      gmt_time;
  double      countdown_time;
  t_limits    tl;

  m_gfxprimitives->Clear();
  tl.start = m_play_timecode;
  tl.stop = m_play_end_timecode;
  tl.lof = m_lof;
  tl.hif = m_hif;
  tl.current = m_play_timecode;
  countdown_time = -1;
  switch (m_state)
    {
    case state_waiting:
      {
	//printf("state=waiting\n");
	//m_play_timecode = 0;
	m_start_tick = m_last_time;
      }
      break;
    case state_only_countdown:
    case state_countdown:
      {
	//printf("state=countdown\n");
	double time = read_day_time();

	//printf("daytime is=%f compared to %f. Diff = %f\n", time, m_start_daytime, time - m_start_daytime);
	if (time - COUNTDOWN >= m_start_daytime)
	  {
	    m_state = (m_state == state_countdown)? state_playing_count : state_waiting; // Next state
	    m_start_tick = m_last_time;
	  }
	else
	  {
	    countdown_time = time - m_start_daytime;
	    //printf("countdown is=%f\n", countdown_time);
	  }
      }
      break;
    case state_playing_count:
      countdown_time = COUNTDOWN + (m_last_time - m_start_tick);
    case state_playing:
      {
	//printf("state=playing\n");
	tl.current = tl.start + (m_last_time - m_start_tick) * m_practicespeed;
	//printf("start is=%f stop is %f\n", tl.current, tl.stop);
	if (tl.current > tl.stop)
	  {
	    m_state = state_waiting;
	    tl.lof = m_lof = F_BASE;
	    tl.hif = m_hif = F_MAX;
	    m_play_timecode = tl.start;
	    return ;
	  }
      }
      break;
    case state_closed:
      {
	//printf("state=closed\n");
	tl.current = m_play_timecode = 0;
	return ;
      }
      break;
    default:
      break;
    }
  pw = m_layout->get_area((char*)"main");
  if (pw != NULL && m_pscore != NULL)
    {
      //m_prenderer/*m_pviolin_renderer*/->render(pw, m_pscore, &tl);
      m_prenderer->render(pw, m_pscore, m_instrument, m_instrument_identifier, &tl, m_note_higlight_identifier);
      if (m_state == state_countdown || (m_state == state_only_countdown))
	{
	  draw_countdown(pw, countdown_time);
	}
    }
  // Update the screen
  m_gfxprimitives->RenderPresent();
}

