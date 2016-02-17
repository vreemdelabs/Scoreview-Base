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

#define TXTSZ 1024

#define COUNTDOWN 3.
#define REWIND    1.

class Cappdata
{
 public:
  Cappdata(std::string path, int width, int height);
  ~Cappdata();

  int  init_the_network_messaging();
  void create_layout();
  void render_gui();
  void get_network_messages();
  void handle_weel(int x, int y, int v);
  void mouseclickleft(int x, int y);
  void mouseupleft(int x, int y);
  void mousemove(int x, int y);
  void key_on(int code);
  void key_off(int code);
  void setlooptime();
  void mirror(int mouse_x, int mouse_y);
  void Send_dialog_opened_message(t_shared_data *papp_data, std::string dialog_id);
  double read_day_time();
  void change_window_size(int newWidth, int newHeight);

 private:
  void set_coord(t_fcoord *coord, float prx, float pry);
  void decode_message_data(t_internal_message *pmsg);
  void draw_countdown(Cgfxarea *pw, double countdown_time);
  void read_window_pos(int *xpos, int *ypos);
  void save_window_coordinates(int xpos, int ypos);
  void select_instrument_renderer(std::string instrument_name);
  void renderTextBlended(TTF_Font *pfont, t_fcoord pos, t_fcoord dim, int color, std::string text, bool blended, float outline, int outlinecolor);

 public:
  std::string     m_path;
  int             m_width;
  int             m_height;
  SDL_Window      *m_sdlwindow;
  int             m_window_xpos;
  int             m_window_ypos;
  SDL_GLContext   m_GLContext;
  CGL2Dprimitives *m_gfxprimitives;
  TTF_Font        *m_font;
  TTF_Font        *m_bigfont;
  CMeshList       *m_mesh_list;

  CgfxareaList    *m_layout;

  t_shared_data    m_shared_data;

  std::string      m_instrument;
  int              m_instrument_identifier;
  CScore          *m_pscore;
  CInstrument     *m_pcurrent_instrument;
  CFingerRenderer *m_prenderer;
  CFingerRenderer *m_null_renderer;
  CvRenderer      *m_violin_renderer;
  CpRenderer      *m_piano_renderer;
  CgRenderer      *m_guitar_renderer;
  CgRendererDropD *m_guitar_renderer_dropD;

  pthread_t        m_threadnetwork;

  t_coord          m_cstart;
  t_coord          m_ccurrent;
  Ckeypress        m_kstates;  

  enum estate
    {
      state_waiting,
      state_countdown,
      state_only_countdown,
      state_playing_count,
      state_playing,
      state_playingloop,
      state_playinglooprewind,
      state_closed
    };

  int              m_state;
  bool             m_mirror;
  double           m_start_daytime;
  double           m_viewtime;
  double           m_start_tick;
  double           m_last_time;
  double           m_play_timecode;
  double           m_play_end_timecode;
  double           m_practicespeed;
  float            m_lof;
  float            m_hif;
  int              m_note_higlight_identifier;
};

