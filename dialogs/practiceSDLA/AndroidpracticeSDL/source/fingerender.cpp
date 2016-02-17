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

#include "gfxareas.h"
#include "mesh.h"
#include "stringcolor.h"
#include "gl2Dprimitives.h"
#include "f2n.h"
#include "score.h"
#include "fingerender.h"

CFingerRenderer::CFingerRenderer(CGL2Dprimitives *pgfxprimitives, TTF_Font *pfont, double viewtime):
  m_viewtime(viewtime),
  m_f2n(440., 2.),
  m_gfxprimitives(pgfxprimitives),
  m_bgcolor(0xFF00EE00),
  m_font(pfont)
{
}

CFingerRenderer::~CFingerRenderer()
{
}

void CFingerRenderer::clear_all()
{
  m_gfxprimitives->set_clear_color(m_bgcolor);
  glClear(GL_COLOR_BUFFER_BIT);
}

// Displays current sample time
void CFingerRenderer::print_current_timecodes(Cgfxarea *pw, int color, double timecode, double viewtime)
{
  char     text[TIMETXTSZ];
  t_coord  pos;
  t_coord  dim;
  t_coord  linepos;
  t_fcoord fpos, fdim;
  double   minutes;
  double   seconds;
  double   mseconds;
  bool     blended;
  int      outlinecolor;
  int      w, h, texth, textw;
  float    outline;

  //--------------------------------------------
  // Render the timecode
  //--------------------------------------------
  minutes = floor(timecode / 60.);
  seconds = timecode - (minutes * 60.);
  mseconds = seconds;
  mseconds -= seconds;
  mseconds *= 100;
  if (minutes == 0)
    sprintf(text, "     %- 2.1fs", seconds);
  else
    sprintf(text, "%- 3.0f:%- 2.1fs", minutes, seconds);
  if (TTF_SizeText(m_font, text, &w, &h) == 0)
    {
      texth = h;
      textw = w;
    }
  else
    {
      texth = 15;
      textw = 80;
    }
  pw->get_posf(&fpos);
  pw->get_dimf(&fdim);
  outline = 0;
  //
  fpos.x = fdim.x - textw;
  fdim.x = textw;
  fdim.y = texth;
  blended = true;
  //color   = WHITE;
  outlinecolor = BLACK;
  m_gfxprimitives->print(text, m_font, fpos, fdim, color, blended, outline, outlinecolor);
}

bool CFingerRenderer::black_note(int octave, int note)
{
  return (note == 1 || note == 3 || note == 6 || note == 8 || note == 10);
}

void CFingerRenderer::render(Cgfxarea *pw, CScore *pscore, std::string instrument_name, int instrument_identifier, t_limits *pl, int hnote_id)
{
  clear_all();
  m_instrument = pscore->get_instrument(instrument_name, instrument_identifier);
}

void CFingerRenderer::set_viewtime(float viewtime)
{
  m_viewtime = viewtime;
}

