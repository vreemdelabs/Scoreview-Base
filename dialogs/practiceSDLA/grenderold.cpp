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
#include <assert.h>
#include <vector>

#ifdef __ANDROID__
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <GL/gl.h>
#endif

#include "stringcolor.h"
#include "gfxareas.h"
#include "mesh.h"
#include "gl2Dprimitives.h"
#include "score.h"
#include "pictures.h"
#include "f2n.h"
#include "stringcolor.h"
#include "fingerender.h"
#include "vrender.h"
#include "grender.h"

// This is just a loosely related remake of synthesia visuals
using namespace std;

CgRenderer::CgRenderer(CGL2Dprimitives *pgfxprimitives, TTF_Font *pfont, double viewtime, CMeshList *pmesh_list):CFingerRenderer(pgfxprimitives, pfont, viewtime),
  m_mesh_list(pmesh_list)
{
  int octave;
  int note;
  //
  // E4
  octave = 4;
  note = 4;
  m_baseAbsNote[0] = m_f2n.notenum(octave, note);
  // B3
  octave = 3;
  note = 11;
  m_baseAbsNote[1] = m_f2n.notenum(octave, note);
  // G3
  octave = 3;
  note = 7;
  m_baseAbsNote[2] = m_f2n.notenum(octave, note);
  // D3
  octave = 3;
  note = 2;
  m_baseAbsNote[3] = m_f2n.notenum(octave, note);
  // A2
  octave = 2;
  note = 9;
  m_baseAbsNote[4] = m_f2n.notenum(octave, note);
  // E2
  octave = 2;
  note = 4;
  m_baseAbsNote[5] = m_f2n.notenum(octave, note);
  //
  m_bgcolor = GBACKGROUND;
}

CgRenderer::~CgRenderer()
{
  
}

t_fcoord CgRenderer::resize(t_coord wsize, t_fcoord pos, t_fcoord dim)
{
  t_fcoord res;

  res.x = (pos.x + dim.x) * (float)wsize.x / 100.;
  res.y = (pos.y + dim.y) * (float)wsize.y / 100.;
  return res;
}

void CgRenderer::draw_finger_board(Cgfxarea *pw, erlvl level)
{
  bool       bantialiased;
  t_fcoord   offset;
  t_fcoord   pos, dim;
  t_coord    idim;
  CMesh     *pmesh;
  t_notefreq notef;
  int        i, absnote;
  float      prevy, radius;
  t_fcoord   fretdim;
  t_fcoord   center;

  pw->get_dim(&idim);
  absnote = m_f2n.notenum(4, 4); // E4
  // Board
  bantialiased = false;
  pw->get_dimf(&dim);
  pos.x = dim.x / 2.975;
  pos.y = 0;
  dim.x = dim.x * 0.13;
  notef.string = 0;
  notef.f = m_f2n.note2frequ(absnote + 24);
  dim.y = note_2_y(idim, &notef);
  if (level == lvl_fingerboard)
    {
      pmesh = m_mesh_list->get_mesh(string("gfboard"));
      m_gfxprimitives->triangle_mesh2D(pos, dim, pmesh, EBONY_COLOR, bantialiased);
    }
  // Frets
  bantialiased = true;
  radius = 0.004 * dim.y;
  //
  prevy = 0;
  for (i = 0; i < GUITAR_NOTES_PER_STRING; i++)
    {
      notef.string = 0;
      notef.f = m_f2n.note2frequ(absnote + i);
      pos.y = note_2_y(idim, &notef);
      pos.x = get_time0_x_limit_from_y(idim, (int)pos.y, true); // Fret board's left side
      fretdim.x = get_time0_x_limit_from_y(idim, (int)pos.y, false) - pos.x; // Fret board's right side
      fretdim.y = 0.001 * dim.y;
      m_gfxprimitives->rectangle(pos, fretdim, FRET_COLOR, bantialiased);
      // Inserts
      if (((i & 1) == 1 && (i > 1) && (i != 11 && i!= 13 && i != 23))
	  || i == 12)
	{
	  center.y = (pos.y + prevy) / 2;
	  if (i == 12)
	    {
	      center.x = pos.x + fretdim.x / 3.;
	      m_gfxprimitives->disc(center, radius, WHITE, bantialiased);
	      center.x = pos.x + fretdim.x * (1. - 1 / 3.);
	      m_gfxprimitives->disc(center, radius, WHITE, bantialiased);
	    }
	  else
	    {
	      center.x = pos.x + fretdim.x / 2.;
	      m_gfxprimitives->disc(center, radius, WHITE, bantialiased);
	    }
	}
      prevy = pos.y;
    }
}

int CgRenderer::blend(float xpos, int color, bool bleft)
{
  int   r, g, b, a;
  float rest, coef;

  r = (color >> 16) & 0xFF;
  g = (color >>  8) & 0xFF;
  b = (color >>  0) & 0xFF;
  //a = color >> 24;
  rest = xpos - floor(xpos);
  coef = bleft? (1 - rest) : rest;
  r = (float)r * coef;
  g = (float)g * coef;
  b = (float)b * coef;
  a = 0xFF000000 | ((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF);
  return a;
}

t_string_equation CgRenderer::get_string(t_coord dim, int string_number)
{
  t_string_equation eq;
  float    ydiff;

  eq.start.y = 0;
  eq.start.x = string_x(dim, string_number, eq.start.y);
  eq.stop.y = dim.y;
  eq.stop.x = string_x(dim, string_number, eq.stop.y);
  //
  ydiff = (float)(eq.stop.y - eq.start.y);
  eq.xdiff = (float)(eq.stop.x - eq.start.x) / ydiff;
  return eq;
}

int CgRenderer::get_time0_x_limit_from_y(t_coord dim, int y, bool left)
{
  int         string_number;
  int         x;
  float       fret_width;
  const float fretbordercoef = 0.07;

  fret_width = string_x(dim, 0, y) - string_x(dim, 5, y);
  if (left)
    {
      string_number = 5;
      x = string_x(dim, string_number, y) - fret_width * fretbordercoef;
    }
  else
    {
      string_number = 0;
      x = string_x(dim, string_number, y) + fret_width * fretbordercoef;
    }
  return x;
}

int CgRenderer::string_width(int string_number)
{
  if (string_number < 2)
    return string_number + 1;
  if (string_number == 2)
    return 2;
  return string_number - 1;
}

void CgRenderer::render_string(Cgfxarea *pw, int string_number, int played_note)
{
  t_string_equation eq; 
  t_coord  dim;
  int      i;
  int      color, colorbl;
  int      ydiff, ystart;
  float    stringwidth, startx;
  t_fcoord start, stop;

  // uses a circle with a center past the strings attachments and angles to calculate string an frets coordinates.
  pw->get_dim(&dim);
  eq = get_string(dim, string_number);
  ydiff = eq.stop.y - eq.start.y;
  ystart = dim.y / 40;
  for (i = ystart; i < ydiff; i++)
    {
      stringwidth = string_width(string_number);
      stop.y = start.y = eq.start.y + i;
      startx = string_x(dim, string_number, start.y) - (stringwidth / 2.);
      start.x = floor(startx);
      stop.x = start.x + stringwidth;
      if (string_number < 3) // Mi
	{
	  color = start.y > m_playedy[string_number] && (m_playedy[string_number] != -1)? 0xFF0000FF : 0xFFAFAFAF;
	}
      else
	{
	  color = i & 1? (start.y > m_playedy[string_number] && (m_playedy[string_number] != -1)? 0xFF0000FF : 0xFFCFCFCF) : 0xFF8F8F8F;
	}
      m_gfxprimitives->hline(start.x, start.y, stop.x - start.x, color);
      // Handmade blending
      colorbl = blend(startx, color, true);
      // Pixels with boxes...
      m_gfxprimitives->box(start.x - 1, start.y, start.x, start.y + 1, colorbl);
      colorbl = blend(startx, color, false);
      m_gfxprimitives->box(stop.x, stop.y, stop.x + 1, stop.y + 1, colorbl);
    }
}

void CgRenderer::draw_strings(Cgfxarea *pw)
{
  render_string(pw, 0, -1);
  render_string(pw, 1, -1);
  render_string(pw, 2, -1);
  render_string(pw, 3, -1);
  render_string(pw, 4, -1);
  render_string(pw, 5, -1);
}

int CgRenderer::note_2_y(t_coord dim, t_notefreq *pnotef)
{
  int   yoffset;
  float string_length;
  float constant;
  float freq;
  float y;

  yoffset = (4. * (float)dim.y / 100.);
  // The finger board makes 88% of the screen, say it is a Guarneri del Gesu then the string length
  // is (88% of the screen) * (1 + (2.2 / 10.3)). Even if Guarneri did not make electric guitars.
  string_length = (97. * (float)dim.y / 100.) * (1. + (2.2 / 10.3));
  // The full string is say A4, half string is A5, if we have f = Constant / L
  // (nearly constant: sqrt(T/µ) / 2L)
  constant = string_length * m_f2n.note2frequ(m_baseAbsNote[pnotef->string]);
  freq = pnotef->f;
  y = constant / freq;
  //printf("string_length=%f ", string_length);
  //printf("notelen=%f freq=%f\n", y, freq);
  y = yoffset + string_length - y;
  return (int)y;
}

int CgRenderer::string_to_color(t_notefreq *pnf)
{
  int color;
  int string_number;
  int abs_note, note, octave;

  string_number = pnf->string;
  switch (string_number)
    {
    case 0:
      color = STRING0_COLOR;
      break;
    case 1:
      color = STRING1_COLOR;
      break;
    case 2:
      color = STRING2_COLOR;
      break;
    case 3:
      color = STRING3_COLOR;
      break;
    case 4:
      color = STRING4_COLOR;
      break;
    case 5:
    default:
      color = STRING5_COLOR;
      break;
    }
  abs_note = m_f2n.frequ2note(pnf->f);
  m_f2n.noteoctave(abs_note, &note, &octave);
  if (black_note(octave, note))
    {
      color = reducecolor(color);
    }
  return color;
}

float CgRenderer::string_x(t_coord dim, int string_number, int y)
{
  float    x;
  double   interstring_angle;

  interstring_angle = m_s0angle / 2.5;
  x = (((float)y - m_string_center.y) * atan(m_s0angle - interstring_angle * (double)string_number) + m_string_center.x);
  return x;
}

void CgRenderer::add_drawings(t_coord dim, int y, CNote *pn, t_notefreq *pnf, t_limits *pl, int hnote_id)
{
  int            noteheigth;
  int            xt0l, xt0r;
  int            newx;
  int            absnote;
  const int      cmarg = 25;
  double         timecode, proxi;
  t_note_drawing nd;
  float          pix_s;
  float          passed_viewtime;
  t_notefreq     notef;
  t_note_segment segment;

  if ((pl->lof >= pnf->f || pnf->f >= pl->hif))
    {
      return;
    }
  timecode = pl->current;
  // Get the note info
  notef = *pnf;
  noteheigth = note_2_y(dim, &notef);
  absnote = m_f2n.frequ2note(notef.f);
  notef.f = m_f2n.note2frequ(absnote + 1);
  noteheigth = (note_2_y(dim, &notef) - noteheigth) * 0.8;
  //
  xt0r = get_time0_x_limit_from_y(dim, y, false);
  xt0l = get_time0_x_limit_from_y(dim, y, true);
  pix_s = (dim.x - xt0r) / m_viewtime;
  passed_viewtime = xt0l * pix_s;
  if (pn->m_time + pn->m_duration >= timecode && pn->m_time < timecode + m_viewtime)
    {
      //
      // Coming note, add them to the segment list
      nd.w = pix_s * pn->m_duration;
      pix_s = (dim.x - xt0r) / m_viewtime;
      nd.x = xt0r + pix_s * (pn->m_time - timecode);
      if (nd.x < xt0r - cmarg)
	{
	  newx = xt0r - cmarg;
	  nd.w = nd.w - abs(nd.x - newx);
	  nd.x = newx;
	}
      segment.bselected_highlight = (hnote_id == pn->identifier());
      segment.xbegin = nd.x;
      segment.xend   = nd.x + nd.w;
      segment.y = y - noteheigth;
      segment.height = noteheigth;
      segment.nf = *pnf;
      segment.fret = m_f2n.frequ2note(pnf->f) - m_baseAbsNote[pnf->string];
      m_segments.push_front(segment);
#ifdef CLOF
      nd.lvl = lvl_background;
      nd.bcircle = false;
      nd.string = pnf->string;
      nd.color = string_to_color(pnf);
      nd.h = noteheigth;
      nd.y = y - noteheigth;
      nd.w = pix_s * pn->m_duration;
      pix_s = (dim.x - xt0r) / m_viewtime;
      nd.x = xt0r + pix_s * (pn->m_time - timecode);
      if (nd.x < xt0r - cmarg)
	{
	  newx = xt0r - cmarg;
	  nd.w = nd.w - abs(nd.x - newx);
	  nd.x = newx;
	}
      //nd.rad = nd.h < nd.w? nd.h : nd.w;
      if (nd.w < nd.h) // Add a circle instead
	{
	  nd.bcircle = true;
	  nd.rad = nd.h / 2.;
	  nd.y += nd.rad;
	}
      else
	nd.rad = nd.h / 2.5;
      m_drawlist.push_back(nd);
#endif
      //printf("adding %s note x=%d y=%d w=%d h=%d dimw=%d dimh=%d\n", "coming", nd.x, nd.y, nd.w, nd.h, dim.x, dim.y);
      // Add a gray little circle
      if (fabs(m_f2n.note2frequ(m_baseAbsNote[pnf->string]) - pnf->f) > 0.5)
	{
	  nd.lvl = lvl_fingerboard;
	  nd.bcircle = true;
	  proxi = pn->m_time < timecode? timecode : pn->m_time;
	  proxi = timecode + m_viewtime - proxi;
	  proxi = proxi < 0? 0 : proxi / 2.;
	  nd.rad = (float)noteheigth * (proxi / m_viewtime);
	  nd.string = pnf->string;
	  nd.color = STRING_GRAY;
	  nd.y = y - noteheigth / 2.;
	  nd.x = string_x(dim, nd.string, nd.y);
	  m_drawlist.push_back(nd);
	}
    }
  if (pn->m_time <= timecode && pn->m_time + pn->m_duration >= timecode)
    {
      // Note being played
      if (fabs(m_f2n.note2frequ(m_baseAbsNote[pnf->string]) - pnf->f) > 0.01)
	{
	  nd.lvl = lvl_top;
	  nd.bcircle = true;
	  nd.rad = noteheigth / 2;
	  nd.string = pnf->string;
	  nd.color = string_to_color(pnf);
	  nd.y = y - noteheigth + nd.rad;
	  //nd.y = y - noteheigth / 2;
	  // x pos of the string
	  nd.x = string_x(dim, nd.string, nd.y);
	  m_drawlist.push_back(nd);
	  //printf("adding %s note x=%d y=%d w=%d h=%d dimw=%d dimh=%d\n", "in", nd.x, nd.y, nd.w, nd.h, dim.x, dim.y);
	  m_playedy[pnf->string] = m_playedy[pnf->string] > nd.y? m_playedy[pnf->string] : nd.y;
	}
      else
	m_playedy[pnf->string] = m_playedy[pnf->string] > y? m_playedy[pnf->string] : y;
    }
  if (false && pn->m_time < timecode && pn->m_time + pn->m_duration > timecode - passed_viewtime)
    {
      // Passed note segment
    }
}

void CgRenderer::check_visible_notes(Cgfxarea *pw, CScore *pscore, t_limits *pl, int hnote_id)
{
  std::list<t_notefreq>::iterator     notefreq_iter;
  CInstrument                        *pi;
  CNote                              *pn;
  t_coord                             pos, dim;
  double                              timecode;
  int                                 y;
  int                                 i;

  m_drawlist.clear();
  m_segments.clear();
  for (i = 0; i < GUITAR_STRINGS; i++)
    m_playedy[i] = -1;
  pw->get_pos(&pos);
  pw->get_dim(&dim);
  timecode = pl->current;
  pi = m_instrument;
  pn = pi->get_first_note_reverse(timecode + m_viewtime);
  while (pn != NULL)
    {
      notefreq_iter = pn->m_freq_list.begin();
      while (notefreq_iter != pn->m_freq_list.end())
	{
	  y = note_2_y(dim, &(*notefreq_iter));
	  add_drawings(dim, y, pn, &(*notefreq_iter), pl, hnote_id);
	  notefreq_iter++;
	}
      if (pn->m_time >= timecode + m_viewtime)
	break;
      pn = pi->get_next_note_reverse();
    }
  concat_segments();
}

bool CgRenderer::overlaping(t_note_segment *p1, t_note_segment *p2)
{
  bool boverlaping;

  boverlaping = ((p2->xbegin >= p1->xbegin) && (p2->xbegin <= p1->xend));
  boverlaping = boverlaping || ((p2->xend >= p1->xbegin) && (p2->xend <= p1->xend));
  boverlaping = boverlaping || ((p1->xend >= p2->xbegin) && (p1->xend <= p2->xend));
  boverlaping = boverlaping && (p1->y == p2->y);
  return boverlaping;
}

bool CgRenderer::compare_note_segments(const t_note_segment first, const t_note_segment second)
{
  return (first.xbegin < second.xbegin);
}

bool CgRenderer::compare_xpos(const int first, const int second)
{
  return (first < second);
}

void CgRenderer::count_played_strings(t_string_segment *pstrsiter, int fret)
{
  std::list<t_note_segment>::iterator  iter;
  t_note_segment                      *pit;

  pstrsiter->string_lst.clear();
  iter = m_segments.begin();
  while (iter != m_segments.end())
    {
      pit = &(*iter);
      if (pit->xbegin <= pstrsiter->x0 && pit->xend >= (pstrsiter->x0 + pstrsiter->length) && pit->fret == fret)
	{
	  pstrsiter->string_lst.push_back(pit->nf);
	}
      iter++;
    }
}

std::list<t_note_segment>::iterator CgRenderer::get_first_fret(int fret)
{
  std::list<t_note_segment>::iterator iter;

  iter = m_segments.begin();
  while (iter != m_segments.end())
    {
      if ((*iter).fret == fret)
	return iter;
      iter++;
    }
  return iter;
}

std::list<t_note_segment>::iterator CgRenderer::get_next_on_fret(std::list<t_note_segment>::iterator iter, int fret)
{
  iter++;
  while (iter != m_segments.end())
    {
      if ((*iter).fret == fret)
	return iter;
      iter++;
    }
  return iter;
}

void CgRenderer::concat_segments()
{
  std::list<t_note_segment>::iterator   iter;
  std::list<t_note_segment>::iterator   next;
  std::list<t_chord_segment>::iterator  citer;
  std::list<t_string_segment>::iterator strsiter;
  std::list<int>                        section_list;
  std::list<int>::iterator              section_iter;
  t_note_segment  *pit, *pnext;
  t_chord_segment  s;
  t_chord_segment *ps;
  t_string_segment stringsegment;
  int              i;

  // Sort the t_note_segment list by x
  m_segments.sort(CgRenderer::compare_note_segments);
  // Find intersecting spaces
  m_chords.clear();
  for (i = 0; i < GUITAR_NOTES_PER_STRING; i++)
    {
      iter = get_first_fret(i);
      if (iter != m_segments.end())
	{
	  pit = &(*iter);
	  s.string_segment_list.clear();
	  s.bselected_highlight = pit->bselected_highlight;
	  s.chord_segment.xbegin = pit->xbegin;
	  s.chord_segment.xend = pit->xend;
	  s.chord_segment.y = pit->y;
	  s.chord_segment.height = pit->height;
	  s.chord_segment.nf.string = 0;
	  s.chord_segment.fret = i;
	}
      while (iter != m_segments.end())
	{
	  pit = &(*iter);
	  next = get_next_on_fret(iter, i);
	  if (next == m_segments.end())
	    {
	      m_chords.push_back(s);
	    }
	  else
	    {
	      pnext = &(*next);
	      if (overlaping(&s.chord_segment, pnext))
		{
		  s.chord_segment.xend = s.chord_segment.xend > pnext->xend? s.chord_segment.xend : pnext->xend;
		  s.bselected_highlight = s.bselected_highlight || pnext->bselected_highlight;
		}
	      else
		{
		  // Push the current chord segment
		  m_chords.push_back(s);
		  // Prepare the next chord segment
		  s.bselected_highlight = pnext->bselected_highlight;
		  s.chord_segment.xbegin = pnext->xbegin;
		  s.chord_segment.xend = pnext->xend;
		  s.chord_segment.y = pnext->y;
		  s.chord_segment.fret = i;
		  s.chord_segment.height = pnext->height;
		}
	    }
	  iter = next;
	}
    }
  // Find the subidvisions of the enclosing segments
  citer = m_chords.begin();
  while (citer != m_chords.end())
    {
      ps = &(*citer);
      section_list.clear();
      iter = m_segments.begin();
      while (iter != m_segments.end())
	{
	  pit = &(*iter);
	  if (pit->xbegin >= ps->chord_segment.xbegin && pit->xend <= ps->chord_segment.xend && pit->fret == ps->chord_segment.fret)
	    {
	      section_list.push_back(pit->xbegin);
	      section_list.push_back(pit->xend);
	    }
	  iter++;
	}
      section_list.sort(compare_xpos);
      section_iter = section_list.begin();
      if (section_iter != section_list.end())
	{
	  stringsegment.x0 = *section_iter;
	  section_iter++;
	}
      while (section_iter != section_list.end())
	{
	  if (*section_iter != stringsegment.x0)
	    {
	      stringsegment.length = *section_iter - stringsegment.x0;
	      ps->string_segment_list.push_back(stringsegment);
	      stringsegment.x0 = *section_iter;
	    }
	  section_iter++;
	}
      citer++;
    }
  // Count the strings per subsegment
  citer = m_chords.begin();
  while (citer != m_chords.end())
    {
      ps = &(*citer);
      strsiter = ps->string_segment_list.begin();
      while (strsiter != ps->string_segment_list.end())
	{
	  count_played_strings(&(*strsiter), ps->chord_segment.fret);
	  strsiter++;
	}
      citer++;
    }
  // Add the rectagles of the coming notes.
  add_coming_notes_rectangles();
}

void CgRenderer::add_coming_notes_rectangles()
{
  std::list<t_chord_segment>::iterator  citer;
  std::list<t_string_segment>::iterator strsiter;
  std::list<t_notefreq>::iterator       numiter;
  int               height;
  t_chord_segment  *ps;
  t_string_segment *pstrsgmt;
  t_note_drawing    nd;
  float             string_number;
  float             h, offset;

  // Count the strings per subsegment
  citer = m_chords.begin();
  while (citer != m_chords.end())
    {
      ps = &(*citer);
      height = ps->chord_segment.height;
      // Show the chord surounding box
      nd.lvl = lvl_background;
      nd.bcircle = false;
      nd.string = 0;
      nd.color = ps->bselected_highlight? WHITE : NOTE_COLOR;
      nd.h = ps->chord_segment.height;
      nd.y = ps->chord_segment.y;
      nd.w = ps->chord_segment.xend - ps->chord_segment.xbegin;
      nd.x = ps->chord_segment.xbegin;
      offset = nd.h / 16;
      nd.w += 2 * offset;
      nd.h += 2 * offset;
      nd.x -= offset;
      nd.y -= offset;
      nd.rad = 0;
      m_drawlist.push_back(nd);
      // Draw the notes and chords rectangles
      strsiter = ps->string_segment_list.begin();
      while (strsiter != ps->string_segment_list.end())
	{
	  pstrsgmt = &(*strsiter);
	  h = 0;
	  string_number = pstrsgmt->string_lst.size();
	  numiter = pstrsgmt->string_lst.begin();
	  while (numiter != pstrsgmt->string_lst.end())
	    {
	      nd.lvl = lvl_background;
	      nd.bcircle = false;
	      nd.string = (*numiter).string;
	      nd.color = string_to_color(&(*numiter));
	      nd.h = height / string_number;
	      nd.y = ps->chord_segment.y + h * (height / string_number);
	      nd.w = pstrsgmt->length;
	      nd.x = pstrsgmt->x0;
	      nd.rad = 0;
	      m_drawlist.push_back(nd);
	      //printf("pushing x=%f y=%f h=%f\n", nd.x, nd.y, nd.h);
	      h++;
	      numiter++;
	    }
	  strsiter++;
	}
      citer++;
    }
  m_segments.clear();
  m_chords.clear();
}

int CgRenderer::reducecolor(int color)
{
  int alpha = color & 0xFF000000;

  color = (color >> 1) & 0x007F7F7F;
  return color | alpha;
}

void CgRenderer::draw_note_drawings(erlvl level, int hnote_id)
{
  std::list<t_note_drawing>::iterator iter;
  t_note_drawing                     *pd;
  int                                 color, masked_outline_color;
  t_fcoord                            center;
  t_fcoord                            fpos, fdim;
  bool                                bantialiased;

  bantialiased = true;
  iter = m_drawlist.begin();
  while (iter != m_drawlist.end())
    {
      pd = &(*iter);
      if (pd->bcircle && pd->lvl == level)
	{
	  color = pd->color;
	  // If pressed but masked by another note then reduce the color
	  if (pd->y < m_playedy[pd->string] && pd->lvl == lvl_top)
	    {
	      color = STRING_GRAY; 
	      //color = reducecolor(color);
	      masked_outline_color = (hnote_id == pd->note_id)? WHITE : NOTE_COLOR; //BORDEAUX;
	    }
	  else
	    {
	      masked_outline_color = (hnote_id == pd->note_id)? WHITE : NOTE_COLOR;
	    }
	  center.x = pd->x;
	  center.y = pd->y;
	  m_gfxprimitives->disc(center, pd->rad, masked_outline_color, bantialiased);
	  m_gfxprimitives->disc(center, pd->rad - pd->rad / 4.5, color, bantialiased);
	}
      else
	{
	  //printf("drawing note box rad == %f\n", pd->rad);
	  if (pd->lvl == level)
	    {
	      // Outline
	      color = pd->color;
	      fpos.x = pd->x;
	      fpos.y = pd->y;
	      fdim.x = pd->w;
	      fdim.y = pd->h;
	      m_gfxprimitives->box(fpos, fdim, color, bantialiased);
	    }
	}
      iter++;
    }
}

void CgRenderer::Draw_the_measure_bars(CScore *pscore, t_coord pos, t_coord dim, double timecode)
{
  std::list<CMesure*>::reverse_iterator it;
  CMesure *pm;
  double   previoustcode;
  double   bartime;
  int      y, x, xt0r, xt1r;
  float    slope;
  float    xsto;

  //
  y = 0;
  xt0r = get_time0_x_limit_from_y(dim, y, false);
  xt1r = get_time0_x_limit_from_y(dim, dim.y, false);
  slope = (float)(xt1r - xt0r) / (float)dim.y;
  previoustcode = timecode + m_viewtime;
  it = pscore->m_mesure_list.rbegin();
  // Assuming the list is sorted then the last measure segment begining
  // before the note is the needed one. Hence reverse order iterator.
  while (it != pscore->m_mesure_list.rend())
    {
      pm = (*it);
      if (pm->m_start < previoustcode)
	{
	  bartime = pm->m_start;
	  while (bartime < previoustcode)
	    {
	      if (bartime > timecode &&
		  bartime < timecode + m_viewtime)
		{
		  x = (bartime - timecode) * (float)(dim.x - xt0r) / m_viewtime;
		  x += xt0r;
		  xsto = x + slope * dim.y;
		  m_gfxprimitives->line(x, 0, xsto, dim.y, VBARCOLOR);
		  // Double the first bar
		  if (bartime == pm->m_start)
		    m_gfxprimitives->line(x + 6, 0, xsto + 6, dim.y, BLACK);
		}
	      bartime += pm->duration();
	    }
	  previoustcode = pm->m_start;
	}
      it++;
    }
}

void CgRenderer::Draw_the_practice_end(t_coord pos, t_coord dim, t_limits *pl)
{
  double timecode;
  double end;
  int    y, x, xt0r, xt1r;
  float  slope;
  float  xsto;

  y = 0;
  xt0r = get_time0_x_limit_from_y(dim, y, false);
  xt1r = get_time0_x_limit_from_y(dim, dim.y, false);
  slope = (float)(xt1r - xt0r) / (float)dim.y;
  timecode = pl->current;
  end = pl->stop;
  if (end > timecode && end < timecode + m_viewtime)
    {
      x = (end - timecode) * (float)(dim.x - xt0r) / m_viewtime;
      x += xt0r;
      xsto = x + slope * dim.y;
      m_gfxprimitives->line(x, 0, xsto, dim.y, BLACK);
    }
}

void CgRenderer::print_current_timecodes(Cgfxarea *pw, int color, double timecode, double viewtime)
{
  char     text[TIMETXTSZ];
  t_coord  pos;
  t_coord  dim;
  t_coord  linepos;
  t_fcoord fpos, fdim;
  double   minutes;
  double   seconds;
  double   mseconds;
  double   tbegin, tplay;
  bool     blended;
  int      outlinecolor;
  int      w, h, texth, textw;
  float    outline;

  //--------------------------------------------
  // Render the timecode
  //--------------------------------------------
  tbegin = timecode + viewtime;
  minutes = floor(tbegin / 60.);
  seconds = tbegin - (minutes * 60.);
  mseconds = seconds;
  mseconds -= seconds;
  mseconds *= 100;
  if (minutes == 0)
    snprintf(text, TIMETXTSZ, "     %- 2.1fs", seconds);
  else
    snprintf(text, TIMETXTSZ, "%- 3.0f:%- 2.1fs", minutes, seconds);
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
  // Play time
  tplay  = timecode;
  minutes = floor(tplay / 60.);
  seconds = tplay - (minutes * 60.);
  mseconds = seconds;
  mseconds -= seconds;
  mseconds *= 100;
  if (minutes == 0)
    snprintf(text, TIMETXTSZ, "     %- 2.1fs", seconds);
  else
    snprintf(text, TIMETXTSZ, "%- 3.0f:%- 2.1fs", minutes, seconds);
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
  fpos.x = fdim.x * 0.32;
  fdim.x = textw;
  fdim.y = texth;
  blended = true;
  //color   = WHITE;
  outlinecolor = BLACK;
  m_gfxprimitives->print(text, m_font, fpos, fdim, color, blended, outline, outlinecolor);
}

void CgRenderer::render(Cgfxarea *pw, CScore *pscore, std::string instrument_name, int instru_identifier, t_limits *pl, int hnote_id)
{
  t_coord pos;
  t_coord dim;

  pw->get_pos(&pos);
  pw->get_dim(&dim);
  clear_all();
  m_string_center.x = dim.x / 2.97;
  m_string_center.y = -2.5 * (float)dim.y;
  m_s0angle = -atan((dim.x * 0.04) / m_string_center.y);
//#define TEST_NOTES_LENGTH
#ifdef TEST_NOTES_LENGTH
  t_coord dim;
  pw->get_dim(&dim);
  t_notefreq notef;
  for (int i = 0; i < 2 * 12; i++)
    {
      notef.string = 1;
      notef.f = m_f2n.note2frequ(12 * 4 + 9 + i);
      note_2_y(dim, &notef);
    }
  return ;
#endif
  m_instrument = pscore->get_instrument(instrument_name, instru_identifier);
  if (m_instrument == NULL)
    return ;
  Draw_the_measure_bars(pscore, pos, dim, pl->current);
  Draw_the_practice_end(pos, dim, pl);
  draw_finger_board(pw, lvl_background);
  check_visible_notes(pw, pscore, pl, hnote_id);
  draw_note_drawings(lvl_background, hnote_id);
  draw_finger_board(pw, lvl_fingerboard);
  draw_strings(pw);
  draw_note_drawings(lvl_fingerboard, hnote_id);
  draw_note_drawings(lvl_top, hnote_id);
  print_current_timecodes(pw, WHITE, pl->current, m_viewtime);
}

//-------------------------------------------------------------------------------
// Drop D version

CgRendererDropD::CgRendererDropD(CGL2Dprimitives *pgfxprimitives, TTF_Font *pfont, double viewtime, CMeshList *pmesh_list):CgRenderer(pgfxprimitives, pfont, viewtime, pmesh_list)
{
  int octave;
  int note;

  //
  // D2
  octave = 2;
  note = 2;
  m_baseAbsNote[5] = m_f2n.notenum(octave, note);
}

CgRendererDropD::~CgRendererDropD()
{
  
}

