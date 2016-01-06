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

#define BACKGROUND 0xFF505040

#define DEFAULT_COLOR_W     0x00FFFFFF
#define DEFAULT_COLOR_B     0x00FFFFFF
#define RIGHT_HAND_COLOR_W  0xFF00E3EB
#define RIGHT_HAND_COLOR_B  0xFF17B1D0
#define LEFT_HAND_COLOR_W   0xFF11D044
#define LEFT_HAND_COLOR_B   0xFF239842

#define LINECOLOR           0xFF2F2F2F
#define BARCOLOR            0x8F5F5F5F

#define PIANO_NOTE_WIDTHS   52 
//(7 * 7 + 3)

class CpRenderer : public CFingerRenderer
{
 public:
  CpRenderer(CGL2Dprimitives *pgfxprimitives, TTF_Font *pfont, double viewtime);
  ~CpRenderer();

 public:
  void render(Cgfxarea *pw, CScore *pscore, std::string instrument_name, int instru_identifier, t_limits *pl);

 private:
  //void render_texture(int color, t_coord pos, float width, float height, SDL_Texture *ptexture);
  void render_texture(t_fcoord pos, t_fcoord dim, int color, std::string name);
  void render_key(int color, t_coord pos, float sizex, float sizey, std::string name);
  void render_white_key(int color, t_coord pos);
  void render_black_key(int color, t_coord pos);
  void render_white_key_pressed(int color, t_coord pos);
  void render_black_key_pressed(int color, t_coord pos);
  void render_black_key_to_be_pressed(int color, t_coord pos);
  void set_render_color(int color);
  void Draw_the_keys_notes_limit(t_coord pos, t_coord dim);
  void Draw_the_measure_bars(CScore *pscore, t_coord pos, t_coord dim, double timecode);
  void Draw_the_octave_limits(t_coord pos, t_coord dim);
  void fill_note_segments_list(CScore *pscore, t_coord pos, t_coord dim, t_limits *pl);
  void Draw_the_notes(t_coord pos, t_coord dim, bool bblack);
  void check_played(CScore *pscore, t_limits *pl);
  void Draw_the_keys_being_played(t_coord dim, bool bblack);
  int  time_to_y(double note_time, double timecode);
  int  reduce_color(int color, float coef);
  void print_current_timecodes(Cgfxarea *pw, int color, double timecode, double viewtime);
  void Draw_the_practice_end(t_coord pos, t_coord dim, t_limits *pl);

 private:
  int    m_keylimity;
  int    m_keylimitsizey;
  int    m_keystarty;
  float  m_key_sizex;
  float  m_key_sizey;
  float  m_bkey_sizex;
  float  m_bkey_sizey;
  CpictureList *m_picturelist;

  typedef struct s_keyp
  {
    bool bplayed;
    bool brighthand;
    bool btobeplayed;
  }              t_keyp;

  t_keyp m_keyboard[PIANO_NOTE_NUMBER];

  typedef struct s_tobeplayed
  {
    int  abs_note;
    int  brighthand;
    int  ystart;
    int  ystop;
    int  x;
    int  w;
    int  h;
  }              t_tobeplayed;

  std::list<t_tobeplayed> m_playlist;
};

