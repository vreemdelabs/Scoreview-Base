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

typedef struct s_limits
{
  double       current;
  double       start;
  double       stop;
  float        lof;
  float        hif;
}              t_limits;

#define TIMETXTSZ  256

class CFingerRenderer
{
 public:
  CFingerRenderer(CGL2Dprimitives *pgfxprimitives, TTF_Font *pfont, double viewtime);
  virtual ~CFingerRenderer();

 public:
  void clear_all();
  void set_viewtime(float viewtime);
  virtual void print_current_timecodes(Cgfxarea *pw, int color, double timecode, double viewtime);
  virtual void render(Cgfxarea *pw, CScore *pscore, std::string instrument_name, int instrument_identifier, t_limits *pl);
  bool black_note(int octave, int note);

 protected:
  double           m_viewtime;
  Cf2n             m_f2n;
  CGL2Dprimitives *m_gfxprimitives;
  CInstrument     *m_instrument;
  int              m_bgcolor;
  TTF_Font        *m_font;
};

