
/*
 Scoreview (R)
 Copyright (C) 2015-2016 Patrick Areny
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

// String instrument common class, works for violin, cello, guitars and every other string instrument with a neck and strings

#define MAX_STRINGS 64

#define STRING_COLOR        0xFFFFFFFF
#define PLAYED_STRING_COLOR 0xFFFF007F
#define PRESSED_COLOR       0xFF44D011
#define NEXT_NOTE_COLOR     0xFF429823
#define EBONY_COLOR         0xFF1A201A
#define EBONY_SIDE_COLOR    0xFF080808
#define TABLE_COLOR         0xFF092058

#define VBARCOLOR           0x8F5F5F5F

#define BORDEAUX            0xFF04049F
#define STRING_WHIT         0xFFCFCFCF
#define STRING0_WHIT        0xFFAFAFAF
#define STRING_GRAY         0xFF4F4F4F

typedef struct s_string_equation
{
  t_fcoord     start;
  t_fcoord     stop;
  float        xdiff;
}              t_string_equation;

enum erlvl
  {
    lvl_background,
    lvl_fingerboard,
    lvl_top
  };

enum estring_type
  {
    elaser = 1, // For Jean Michel
    ebowel, // More serioulsy
    enylon, // Even more
    esinglewire, // Metal string
    ewound
  };

class CsiRenderer : public CFingerRenderer
{
 public:
  CsiRenderer(CGL2Dprimitives *pgfxprimitives, TTF_Font *pfont, double viewtime, CMeshList *pmesh_list, int string_number);
  ~CsiRenderer();

 public:
  virtual void render(Cgfxarea *pw, CScore *pscore, std::string instrument_name, int instru_identifier, t_limits *pl, int hnote_id);

 protected:

  t_fcoord resize(t_coord wsize, t_fcoord pos, t_fcoord dim);
  virtual void draw_finger_board(Cgfxarea *pw, erlvl level);
  void draw_strings(Cgfxarea *pw);
  void render_string(Cgfxarea *pw, int string_number, int played_note);
  int  blend(float xpos, int color, bool left);
  int  string_to_color(t_notefreq *pnf);
  virtual void add_drawings(t_coord dim, int y, CNote *pn, t_notefreq *pnf, t_limits *pl, int hnote_id);
  int  note_2_y(t_coord dim, t_notefreq *pnotef);
  t_string_equation get_string(t_coord dim, int string_number);
  float string_x(t_coord dim, int string_number, int y);
  int  get_time0_x_limit_from_y(t_coord dim, int y, bool left);
  void check_visible_notes(Cgfxarea *pw, CScore *pscore, t_limits *pl, int hnote_id);
  virtual void draw_note_drawings(erlvl level, int hnote_id);
  virtual int  string_width(int string_number);
  int  reducecolor(int color);
  void Draw_the_measure_bars(CScore *pscore, t_coord pos, t_coord dim, double timecode);
  void print_current_timecodes(Cgfxarea *pw, int color, double timecode, double viewtime);
  void Draw_the_practice_end(t_coord pos, t_coord dim, t_limits *pl);

 protected:
  int                *m_baseAbsNote;
  int                *m_playedy;
  int                *m_string_type;
  int                 m_string_number;
  int                 m_wire_string_index;   // single wire strings are from 0 to this variable

  t_fcoord            m_string_center;  // The point where the strings would join
  double              m_s0angle;        // Angle of the string 0

  typedef struct s_note_drawing
  {
    erlvl lvl;
    bool  bcircle;
    int   color;
    int   string;
    float y;
    float x;
    float w;
    float h;
    float rad;
    int   note_id;
  }              t_note_drawing;

  std::list<t_note_drawing> m_drawlist;

  CMeshList *m_mesh_list;
};
