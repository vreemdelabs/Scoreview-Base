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

#include <string.h>
#include <math.h>

#include <iterator>
#include <list>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include <GL/gl.h>

#include "audiodata.h"
#include "audioapi.h"
#include "shared.h"
#include "stringcolor.h"
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
#include "scorerenderer.h"
#include "card.h"
#include "messages.h"
#include "tcp_message_receiver.h"
#include "messages_network.h"
#include "app.h"
//#include "sdlcalls.h"

#define ERRTHRSOLD 0.2

void Cappdata::draw_freq_scale(Cgfxarea *pw, int color, float minf, float maxf)
{
  char     text[TXTSZ];
  t_coord  pos;
  t_coord  dim;
  t_fcoord fpos;
  t_fcoord text_dim;
  int      tcolor = TXTCOLOR;
  int      length;
  int      texth;

  //--------------------------------------------
  // Render the top and min freq values
  //--------------------------------------------
  sprintf(text, "%dhz", (int)maxf);
  if (m_gfxprimitives->get_textSize(text, m_font, &text_dim))
    {
      texth = text_dim.y;
    }
  else
    texth = 15;
  pw->get_pos(&pos);
  pos.y -= texth;
  rendertext(pos, tcolor, text);
  sprintf(text, "%dhz", (int)minf);
  pw->get_pos(&pos);
  pw->get_dim(&dim);
  pos.y += dim.y;// - 20;
  rendertext(pos, tcolor, text);
  //--------------------------------------------
  // Render the scale bars
  //--------------------------------------------
  pw->get_pos(&pos);
  length = dim.x;
  draw_h_line(pos, length, TXTCOLOR); // top
  pw->get_pos(&pos);
  pw->get_dim(&dim);
  pos.y += dim.y;
  draw_h_line(pos, length, TXTCOLOR); // Bottom
  draw_scale(pw, TXTCOLOR, minf, maxf, 10);
  draw_scale(pw, TXTCOLOR, minf, maxf, 100);
  draw_scale(pw, TXTCOLOR, minf, maxf, 1000);
}

#define BLACKNOTE(note) (note == 1 || note == 3 || note == 6 || note == 8 || note == 10)

void Cappdata::render_key_texture(int color, t_fcoord pos, t_fcoord dim, string name)
{
#ifdef NOTEXTURES
  if (name == string("blackkey"))
    m_gfxprimitives->box(pos, dim, 0xFF000000, true);
  if (name == string("whitekey"))
    {
      m_gfxprimitives->box(pos, dim, 0xFFFFFFFF, true);
    }
#else    
  m_gfxprimitives->draw_key(name, pos, dim, M_PI / 2);
  //m_gfxprimitives->box(pos, dim, color, filtering);
#endif
}

float Cappdata::getSpectrumFloatY_from_Frequ(float f, t_fcoord pos, t_fcoord dim, float fbase, float fmax)
{
  float y;

  y = pos.y + dim.y - /*floor*/((f - fbase) * (float)dim.y / (fmax - fbase));
  y = (y < pos.y)? pos.y : y;
  y = (y > pos.y + dim.y)? pos.y + dim.y - 1 : y;
  return y;
}

void Cappdata::Draw_keys(t_fcoord pos, t_fcoord dim, bool bblack, int curoctave, int curnote, float fmin, float fmax)
{
  char     text[TXTSZ];
  int      abs_note;
  int      octave, note, sidenote, sideoctave;
  int      color;
  float    y, freq, hi, lo, sidefreq;
  t_fcoord kpos, kdim;
  t_coord  lpos;

  y = pos.y;
  color = BLACK;
  for (abs_note = 9; abs_note < 9 + PIANO_NOTE_NUMBER; abs_note++)
    {
      kdim.x = dim.x;
      kpos.x = pos.x;
      //
      m_f2n.noteoctave(abs_note, &note, &octave);
      m_f2n.note_frequ_boundary(&lo, &hi, note, octave);
      kpos.y = getSpectrumFloatY_from_Frequ(hi, pos, dim, fmin, fmax);
      y = getSpectrumFloatY_from_Frequ(lo, pos, dim, fmin, fmax);
      if (bblack)
	{
	  if (BLACKNOTE(note))
	    {
	      kdim.y = fabs(kpos.y - y); // / TWELVTH_ROOT_OF_2;
	      kdim.x *= 0.5;
	      render_key_texture(color, kpos, kdim, std::string("blackkey"));
	    }
	}
      else
	{
	  if (!BLACKNOTE(note))
	    {
	      kdim.y = fabs(kpos.y - y); // / TWELVTH_ROOT_OF_2;
	      m_f2n.noteoctave(abs_note + 1, &sidenote, &sideoctave);
	      if (BLACKNOTE(sidenote))
		{
		  sidefreq = m_f2n.note2frequ(sidenote, sideoctave);
		  y = getSpectrumFloatY_from_Frequ(sidefreq, pos, dim, fmin, fmax);
		  kdim.y += fabs(y - kpos.y);
		  kpos.y = y;
		}
	      m_f2n.noteoctave(abs_note - 1, &sidenote, &sideoctave);
	      if (BLACKNOTE(sidenote))
		{
		  sidefreq = m_f2n.note2frequ(sidenote, sideoctave);
		  y = getSpectrumFloatY_from_Frequ(sidefreq, pos, dim, fmin, fmax);
		  kdim.y += fabs(y - (kpos.y + kdim.y));
		}
	      render_key_texture(color, kpos, kdim, std::string("whitekey"));
	    }
	}
      // Cursor note
      if (note == curnote && octave == curoctave)
	{
	  if ((BLACKNOTE(note) && bblack) ||
	      (!BLACKNOTE(note) && !bblack))
	    {
	      // Note sel box
	      color = 0xFF8080F4;
	      m_gfxprimitives->box(kpos, kdim, color);
	      // Text
	      color = 0xFFFFFFFF;
	      lpos.x = pos.x + dim.x + 4;
	      lpos.y = kpos.y - kdim.y / 2.;
	      freq = m_f2n.note2frequ(curnote, curoctave);
	      sprintf(text, "%4.0f", freq);
	      rendertext(lpos, color, text);
	      //m_gfxprimitives->print(text, m_font, lpos, color);
	      m_f2n.notename(curoctave, curnote, text, TXTSZ);
	      lpos.y += 15;
	      rendertext(lpos, color, text);
	      //m_gfxprimitives->print(text, m_font, lpos, color);
	    }
	}
      // Darken if odd octaves
      if ((octave & 1) == 1)
	{
	  kpos.x = pos.x + dim.x - 4; 
	  kpos.y = kpos.y; 
	  kdim.x = 4;
	  color = 0xFF00AFFF;
	  m_gfxprimitives->box(kpos, kdim, color);
	}
    }
}

void Cappdata::draw_piano_scale(Cgfxarea *pw, int color, float fmin, float fmax, t_coord mousepos)
{
  char     text[TXTSZ];
  t_coord  pos, dim;
  t_fcoord fpos, fdim;
  t_fcoord text_dim;
  int      tcolor = TXTCOLOR;
  float    curfrequ;
  float    frequ_err;
  int      curoctave, curnote;
  int      texth;

  //--------------------------------------------
  // Render the top and min freq values
  //--------------------------------------------
  sprintf(text, "%dhz", (int)fmax);
  if (m_gfxprimitives->get_textSize(text, m_font, &text_dim))
    {
      texth = text_dim.y;
    }
  else
    texth = 15;
  pw->get_pos(&pos);
  pos.y -= texth;
  rendertext(pos, tcolor, text);
  sprintf(text, "%dhz", (int)fmin);
  pw->get_pos(&pos);
  pw->get_dim(&dim);
  pos.y += dim.y;// - 20;
  rendertext(pos, tcolor, text);
  //--------------------------------------------
  // Render the piano keys
  //--------------------------------------------
  if (m_layout->is_in(mousepos.x, mousepos.y, (char*)"spectre")) // Ack to get the note when rendering
    {
      pw->get_pos(&pos);
      pw->get_dim(&dim);
      curfrequ = getSpectrum_freq_fromY(mousepos.y, pos, dim, fmin, fmax);
      m_f2n.convert(curfrequ, &curoctave, &curnote, &frequ_err);
      //printf("note=%d octave=%d\n", curnote, curoctave);
    }
  else
    {
      curoctave = curnote = curfrequ = -1;
    }
  pw->get_posf(&fpos);
  pw->get_dimf(&fdim);
  Draw_keys(fpos, fdim, false, curoctave, curnote, fmin, fmax);
  Draw_keys(fpos, fdim, true, curoctave, curnote, fmin, fmax);
}

