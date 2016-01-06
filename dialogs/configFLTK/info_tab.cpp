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

#include <FL/Fl_Group.H>
#include <FL/Fl_Text_Display.H>
#include <FL/Fl_Text_Buffer.H>

void create_info_tab(int x, int y, int w, int h)
{
  int              border;
  Fl_Text_Display *ptext;
  Fl_Text_Buffer  *pbuff;

  Fl_Group *group1 = new Fl_Group(x, y, w, h, "info");
  {
    border = 10;
    ptext = new Fl_Text_Display(x + border, y + border, w - 3 * border, h - 3 * border);
    pbuff = new Fl_Text_Buffer();
    ptext->buffer(pbuff);
    pbuff->text("\n\n"
		"       Scoreview (tm) beta test version, dec 2015.\n"
		"\n"
		"            Copyright 2015 vreemdelabs.com\n"
		"\n"
		" Code: Patrick Areny\n"
		" Artwork: Clement Caminal\n"
		"\n"
		" Uses the following libraries:\n"
		"\n"
		"  The SDL 2.0\t(Simple Direct Media Layer library)\n"
		"  SDL_ttf\tSam Lantinga\n"
		"  SDL_image\tSam Lantinga and Mattias Engdegård\n"
		"  Portaudio  \twww.portaudio.com\n"
		"  DSPFilters \tVinnie Falco\n"
		"  Tinyxml    \tLee Thomason\n"
		"  libevent   \tNick Mathewson and Niels Provos\n"
		"  libsndfile \tErik de Castro Lopo\n"
		"  OpenGl     \tKhronos group\n"
		"  OpenCl     \tKhronos group\n"
		"  Protocol Buffers\tGoogle\n"
		"  FLTK       \twww.fltk.org\n"
		"\n"
		" Uses the following fonts: "
		"\n\n"
		" ArmWrestler ttf, AJ Paglia\n"
		" VeraMono ttf, Bitstream, Inc.\n"
		"\n");
  }
  group1->end();
}

