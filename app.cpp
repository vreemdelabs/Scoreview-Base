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

#include <iterator>
#include <list>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <GL/gl.h>

#include <tinyxml.h>

#include "env.h"
#include "audiodata.h"
#include "audioapi.h"
#include "cfgfile.h"
#include "shared.h"
#include "gfxareas.h"
#include "mesh.h"
#include "gl2Dprimitives.h"
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
#include "scoreedit_piano.h"
#include "scoreedit_guitar.h"
#include "scorerenderer.h"
#include "card.h"
#include "messages.h"
#include "tcp_ip.h"
#include "messages_network.h"
#include "app.h"
#include "sdlcalls.h"

Cappdata::Cappdata(int width, int height):
  m_width(width),
  m_height(height),
  m_empty_note(-1, -1, -1),
  m_appstate(statewait),
  m_pianofrequdisplay(false),
  m_f2n(440., 2.),
  m_brecord_at_start(true),
  m_bdonotappendtoopen(false),
  m_opened_a_file(false)
{
  const int cdft_w = 512;
  int i;

  // Card initial status
  m_card_states.bhelpers_enabled = false;
  m_card_states.autobeam = false;
  m_card_states.chordfuse = false;
  m_shared_data.chs.api_name = string("");
  // read config file
  read_config_data(&m_window_xpos, &m_window_ypos, &m_card_states);
  // Init gfx zones parameters
  create_layout();
  // Init all the SDL functions
  init_SDL(&m_sdlwindow, &m_GLContext, m_window_xpos, m_window_ypos, m_width, m_height);
  m_gfxprimitives = new CGL2Dprimitives(m_sdlwindow, m_GLContext, m_width, m_height);
  m_gfxprimitives->set_clear_color(0xFF000000);
  m_gfxprimitives->init_OpenGL();
  m_gfxprimitives->set_dft_size(2 * cdft_w, cdft_w);
  TTF_Init();
  m_font = TTF_OpenFont(DATAPATH MAINFONT, 14);
  if (m_font == NULL)
    {
      printf("Error: font \"%s\" is missing.\n", DATAPATH MAINFONT);
      exit(1);
    }
  m_bigfont = TTF_OpenFont(DATAPATH BIGFONT, 64);
  if (m_font == NULL)
    {
      printf("Error: font \"%s\" is missing.\n", DATAPATH BIGFONT);
      exit(1);
    }
  m_mesh_list = new CMeshList();
  if (!load_meshes(m_mesh_list))
    {
      printf("Error: polygon drawings not loaded.\n");
      exit (1);
    }
  create_cards_layout();
  m_picturelist = new CpictureList(m_gfxprimitives);
  m_picturelist->open_interface_imgs(string(IMGPATH) + string("images.png"));
  m_picturelist->open_practice_drawings(string(IMGPATH) + string("images.png"));
  m_picturelist->open_instrument_tabs_drawings(string(IMGPATH) + string("instruments.png"));
  m_picturelist->open_skin(string(IMGPATH) + string("skin.png"), string("firstskin"));
  // Init all the mutexes from the shared data
  pthread_mutex_init(&m_shared_data.datamutex, NULL);
  pthread_mutex_init(&m_shared_data.dftmutex, NULL);
  pthread_mutex_init(&m_shared_data.filtermutex, NULL);
  pthread_mutex_init(&m_shared_data.cond_drawspectremutex, NULL);
  pthread_cond_init(&m_shared_data.cond_drawspectre, NULL);
  m_shared_data.bquit = false;
  m_shared_data.bOpenClEnabled = false;
  m_shared_data.zoom_f = 0.1;
  m_shared_data.zoom_t = 7. / MAX_VIEW_TIME;
  m_shared_data.tfprecision = 1.;
  m_shared_data.dft_w = 2 * cdft_w;
  m_shared_data.dft_h = cdft_w;
  m_shared_data.bspectre_img_updated = false;
  m_shared_data.blogdb = false;
  m_shared_data.poutimg = new int[m_shared_data.dft_w * m_shared_data.dft_h];
  memset(m_shared_data.poutimg, 0, m_shared_data.dft_w * m_shared_data.dft_h * sizeof(int));
  m_shared_data.pattackdata = new float[m_shared_data.dft_w];
  for (i = 0; i < m_shared_data.dft_w; i++)
    m_shared_data.pattackdata[i] = 0;
  m_shared_data.circularcut = 0;
  m_shared_data.fbase = F_BASE;
  m_shared_data.fmax = 2000.;
  m_localfbase = m_shared_data.fbase;
  m_localfmax = m_shared_data.fmax;
  m_shared_data.viewtime = 7.;
  m_shared_data.baudioready = false;
  m_shared_data.samplerate = 44100.;
  m_shared_data.timecode = 0.;
  m_shared_data.trackend = 0.;
  m_shared_data.lastimgtimecode = 0.;
  m_shared_data.practice_countdown_tick = 0.;
  m_shared_data.practicespeed = 1.;
  m_shared_data.pad = new Caudiodata();
  if (m_brecord_at_start)
    {
      m_shared_data.play_State = state_record;
      set_record_card(true);
    }
  else
    m_shared_data.play_State = state_stop;
  m_shared_data.audio_cmd_sdlevent = register_sdl_user_event();
  // Sound filtering thread
  pthread_mutex_init(&m_shared_data.soundmutex, NULL);
  pthread_mutex_init(&m_shared_data.cond_soundmutex, NULL);
  pthread_cond_init(&m_shared_data.cond_sound, NULL);
  m_shared_data.filter_cmd_list.clear();
  m_shared_data.filtered_sound_list.clear();
  // Track image
  m_shared_data.trackimgw = 2 * cdft_w;
  m_shared_data.trackimgh = 48;
  m_shared_data.ptrackimg = new int[m_shared_data.dft_w * m_shared_data.dft_h];
  m_shared_data.btrack_img_updated = false;
  m_gfxprimitives->add_empty_texture(m_shared_data.trackimgw, m_shared_data.trackimgh, string("track_bitmap"));
  // Basic SCORE track
  m_pscore = new CScore(std::string("violin"));
  m_pcurrent_instrument = m_pscore->get_first_instrument();
  m_empty_note.set_identifier(-1);
  // Renderer TODO from a plugin
  m_pScorePlacementViolin = new CScorePlacementViolin();
  m_pScorePlacementPiano  = new CScorePlacementPiano();
  m_pScorePlacementGuitar = new CScorePlacementGuitar();
  m_pScorePlacement = get_instrumentPlacement(m_pcurrent_instrument->get_name());
  // Score editor
  m_pScoreEditViolin = new CscoreEdit(&m_kstates);
  m_pScoreEditPiano  = new CscoreEditPiano(&m_kstates);
  m_pScoreEditGuitar = new CscoreEditGuitar(&m_kstates);
  m_pScoreEdit = get_instrumentEdit(m_pcurrent_instrument->get_name());
  m_note_selection.cmdlist.clear();
  m_note_selection.play_timecode = m_note_selection.box.start_tcode = m_note_selection.box.stop_tcode = -1;
  // Pass configuration to the edition
  m_pScorePlacement->set_autobeam(m_card_states.autobeam);
  m_pScoreEdit->set_chord_fuse(m_card_states.chordfuse);
  //
  m_pScoreRenderer = new CScoreRenderer(m_gfxprimitives, m_picturelist, m_bigfont, m_mesh_list);
  //
  setlooptime(0.001 * SDL_GetTicks());
}

Cappdata::~Cappdata()
{
  int xpos, ypos;

  delete m_pserver;
  delete m_pScorePlacementViolin;
  delete m_pScorePlacementPiano;
  delete m_pScorePlacementGuitar;
  delete m_pScoreEditViolin;
  delete m_pScoreEditPiano;
  delete m_pScoreEditGuitar;
  delete m_pScoreRenderer;
  delete m_pscore;
  delete[] m_shared_data.pattackdata;
  delete[] m_shared_data.ptrackimg;
  delete[] m_shared_data.poutimg;
  delete m_shared_data.pad;
  // Release the mutexes
  pthread_mutex_destroy(&m_shared_data.datamutex);
  pthread_mutex_destroy(&m_shared_data.dftmutex);
  pthread_mutex_destroy(&m_shared_data.filtermutex);
  pthread_cond_destroy(&m_shared_data.cond_drawspectre);
  pthread_mutex_destroy(&m_shared_data.cond_drawspectremutex);
  pthread_mutex_destroy(&m_shared_data.soundmutex);
  pthread_cond_destroy(&m_shared_data.cond_sound);
  pthread_mutex_destroy(&m_shared_data.cond_soundmutex);
  // Close everything related to ui
  delete m_mesh_list;
  TTF_CloseFont(m_bigfont);
  TTF_CloseFont(m_font);
  TTF_Quit();
  SDL_GetWindowPosition(m_sdlwindow, &xpos, &ypos);
  save_config_data(xpos, ypos, &m_card_states);
  delete m_picturelist;
  delete m_cardlayout;
  delete m_gfxprimitives;
  close_SDL(m_sdlwindow, m_GLContext);
  delete m_layout;
}

int Cappdata::init_the_network_messaging()
{
  m_pserver = new CnetworkMessaging(SCOREVIEW_PORT);
  return m_pserver->init_the_network_messaging();
}

void Cappdata::wakeup_spectrogram_thread()
{
  t_shared   *pshared_data = &m_shared_data;

  pthread_mutex_lock(&pshared_data->cond_drawspectremutex);
  pthread_cond_signal(&pshared_data->cond_drawspectre);
  pthread_mutex_unlock(&pshared_data->cond_drawspectremutex);
}

void Cappdata::wakeup_bandpass_thread()
{
  t_shared   *pshared_data = &m_shared_data;

  pthread_mutex_lock(&pshared_data->cond_soundmutex);
  pthread_cond_signal(&pshared_data->cond_sound);
  pthread_mutex_unlock(&pshared_data->cond_soundmutex);
}

void Cappdata::read_config_data(int *xpos, int *ypos, t_card_states *pcs)
{
  int                    x, y;
  Cxml_cfg_file_decoder *pd;
  string                 cfgpath;

  *xpos = *ypos = SDL_WINDOWPOS_CENTERED;
  pd = new Cxml_cfg_file_decoder();
  cfgpath = pd->get_user_config_path();
  if (pd->open_for_reading(cfgpath + string(CONFIG_FILE_NAME)))
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
      pd->read_freq_view_mode(m_pianofrequdisplay);
      pd->read_record_params(&m_brecord_at_start, &m_bdonotappendtoopen);
      pd->read_sound_io_params(&m_shared_data.chs);
      pd->read_edit_params(&pcs->autobeam, &pcs->chordfuse);
    }
  delete pd;
}

void Cappdata::save_config_data(int xpos, int ypos, t_card_states *pcs)
{
  Cxml_cfg_file_decoder *pd;
  string                 cfgpath;

  pd = new Cxml_cfg_file_decoder();
  pd->open_for_writing();
  pd->write_window_position(xpos, ypos);
  pd->write_window_size(m_width, m_height);
  pd->write_freq_view_mode(m_pianofrequdisplay);
  pd->write_record_params(m_brecord_at_start, m_bdonotappendtoopen);
  pd->write_edit_params(pcs->autobeam, pcs->chordfuse);
  pd->write_sound_io_params(&m_shared_data.chs);
  cfgpath = pd->get_user_config_path();
  pd->write(cfgpath + string(CONFIG_FILE_NAME));
  delete pd;
}

void Cappdata::change_window_size(int newWidth, int newHeight)
{
  t_coord wsize;

  wsize.x = newWidth;
  wsize.y = newHeight;
  m_width = wsize.x;
  m_height = wsize.y;
  m_layout->resize(wsize);
  m_cardlayout->resize(wsize);
}

void Cappdata::set_coord(t_fcoord *coord, float prx, float pry)
{
  coord->x = prx;
  coord->y = pry;
  coord->z = 0;
}

CScorePlacement* Cappdata::get_instrumentPlacement(std::string instrument_name)
{
  if (instrument_name == string("violin"))
    return m_pScorePlacementViolin;
  if (instrument_name == string("piano"))
    return m_pScorePlacementPiano;
  if (instrument_name == string("guitar"))
    return m_pScorePlacementGuitar;
  return m_pScorePlacementViolin;
}

CscoreEdit* Cappdata::get_instrumentEdit(std::string instrument_name)
{
  if (instrument_name == string("violin"))
    return m_pScoreEditViolin;
  if (instrument_name == string("piano"))
    return m_pScoreEditPiano;
  if (instrument_name == string("guitar"))
    return m_pScoreEditGuitar;
  return m_pScoreEditViolin;
}

bool Cappdata::load_meshes(CMeshList *pml)
{
  bool ret;

  ret = true;
  ret &= pml->Load_meshes_from((char*)DATAPATH, (char*)"bemol.obj");
  ret &= pml->Load_meshes_from((char*)DATAPATH, (char*)"cleffa.obj");
  ret &= pml->Load_meshes_from((char*)DATAPATH, (char*)"diese.obj");
  ret &= pml->Load_meshes_from((char*)DATAPATH, (char*)"clefsol.obj");
  ret &= pml->Load_meshes_from((char*)DATAPATH, (char*)"flag.obj");
  ret &= pml->Load_meshes_from((char*)DATAPATH, (char*)"qnrest.obj");
  ret &= pml->Load_meshes_from((char*)DATAPATH, (char*)"enrest.obj");
  ret &= pml->Load_meshes_from((char*)DATAPATH, (char*)"snrest.obj");
  ret &= pml->Load_meshes_from((char*)DATAPATH, (char*)"tsnrest.obj");
  ret &= pml->Load_meshes_from((char*)DATAPATH, (char*)"sfnrest.obj");
  return ret;
}

void Cappdata::create_layout()
{
  Cgfxarea *pw;
  t_fcoord  pos;
  t_fcoord  dimensions;
  t_coord   window_dim;
  float    leftborder;
  float    horiz_size;
  float    hspectre;
  float    spectrey;

  window_dim.x = m_width;
  window_dim.y = m_height;
  m_layout = new CgfxareaList();
  leftborder = 1.;
  spectrey = 7.5;
  horiz_size = 93.;
  hspectre = 44.;
  // Spectrogram window
  set_coord(&pos, leftborder, spectrey);
  set_coord(&dimensions, horiz_size, hspectre);
  pw = new Cgfxarea(pos, dimensions, window_dim, (char*)"spectre");
  m_layout->add(pw);
/*
  pos.x = 0;
  pos.y = 0;
  dimensions.x = 1024;
  dimensions.y =  512;
*/
  // Frequency scale
  set_coord(&pos, horiz_size + leftborder + 0.25, spectrey);
  set_coord(&dimensions, 2., hspectre);
  pw = new Cgfxarea(pos, dimensions, window_dim, (char*)"freqscale");
  m_layout->add(pw);
  // Time scale
  set_coord(&pos, leftborder, hspectre + spectrey + 0.184);
  set_coord(&dimensions, horiz_size, 2.76);
  pw = new Cgfxarea(pos, dimensions, window_dim, (char*)"timescale");
  m_layout->add(pw);
  // Samples
  set_coord(&pos, leftborder, hspectre + spectrey + 3.7);
  set_coord(&dimensions, horiz_size, 6.5);
  pw = new Cgfxarea(pos, dimensions, window_dim, (char*)"track");
  m_layout->add(pw);
  // Score view
  set_coord(&pos, leftborder, hspectre + spectrey + 11);
  set_coord(&dimensions, horiz_size, 29.5);
  pw = new Cgfxarea(pos, dimensions, window_dim, (char*)"score");
  m_layout->add(pw);
  // Score instrument selection
  set_coord(&pos, leftborder + horiz_size, hspectre + spectrey + 11);
  set_coord(&dimensions, 5., 29.5);
  pw = new Cgfxarea(pos, dimensions, window_dim, (char*)"instrument");
  m_layout->add(pw);
  // Main window
  set_coord(&pos, 0., 0.);
  set_coord(&dimensions, 100., 100.);
  pw = new Cgfxarea(pos, dimensions, window_dim, (char*)"main");
  m_layout->add(pw);
}

