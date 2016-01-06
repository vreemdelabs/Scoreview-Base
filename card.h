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

#define INSPECT_TIMEOUT   20
#define INSPECT_THRSHOLD  0.480
//#define INSPECT_THRSHOLD  0.190
#define REFLEXTIME        0.180
#define DISPTIME          0.100

#define CARDANGLE         0  //(M_PI / 128)
#define CARDLIGHTC        0xFFFFEFEF

#define MAX_CARD_WIDTH    280

using namespace std;

enum erclick
  {
    erclickstart,
    erclickstop,
    erclickunknown
  };

class Ccard
{
 public:
  Ccard(Cgfxarea area, std::string title, std::string cardimg_filename, CGL2Dprimitives *pr, bool helper = false);
  ~Ccard();

  void display_card_reduced(int w, int h, bool helpers_enabled, double curenttimecode);
  void display_card_big(int w, int h, bool helpers_enabled, double curenttimecode);

  void mouse_over(bool bover, double timecode, erclick rclick = erclickunknown, bool bmove = false);

  std::string get_name();
  bool same_name(char *name);
  bool is_in(t_coord c);
  void activate_card(bool active);
  bool is_activated();
  bool is_helper();
  void add_info_line(string i);

  void create_texture(TTF_Font *title_font, TTF_Font *font, SDL_Surface* contour_texture);
  void resize_keep_ratio(t_coord new_window_size);

 private:
  t_coord renderText(SDL_Surface *dest, std::string text, TTF_Font *font, t_coord pos, int color);
  t_coord renderOutlinedText(SDL_Surface *dest, std::string text, TTF_Font *font, t_coord pos, t_coord dim, int color, int outcolor);
  void display_inspect(double timeover, int w, int h);
  void get_dest(int scrw, int scrh, t_fcoord *start, t_fcoord *vect, float *pszfactor);
  bool translate_n_scale(double time, int scrw, int scrh, t_fcoord *ptrans, t_fcoord *pscale);

 private:
  bool                   m_bactive;
  bool                   m_binspect;
  double                 m_mouseovertimecode;
  double                 m_mouseofftimecode;
  double                 m_start_display_timecode;
  double                 m_disptime;
  bool                   m_bhelper;
  std::list<std::string> m_textlist;
  // Layout
  Cgfxarea               m_area;
  // Rendering tool
  CGL2Dprimitives       *m_gfxprimitives;
  std::string            m_cardimg_filename;
  std::string            m_card_texture_name;

  enum ecard_state
    {
      ewait,
      eforceview,
      ewaitshow,
      eshow
    };
  ecard_state           m_state;
};

class CCardList
{
 public:
  CCardList();
  ~CCardList();

  void add(Ccard *pc);
  Ccard *get_first();
  Ccard *get_next();
  Ccard *is_in(int x, int y);
  Ccard *get_card(char *gfxareaname);
  void  enable_card(char *name, float en);
  void  resize(t_coord new_window_size);

 private:
  std::list<Ccard*> m_card_list;
  std::list<Ccard*>::iterator m_iter;
};

