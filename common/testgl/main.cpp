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
//
// Testing the gl2D library functions with the SDL2 and openGL2
//

#include <unistd.h>

#include <iterator>
#include <list>
#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

//#include <GL/gl.h>
//#include <GL/glu.h>
#include <GL/glew.h>


#include "env.h"
#include "gfxareas.h"
#include "mesh.h"
#include "gl2Dprimitives.h"
#include "pictures.h"

#include "sdlcalls.h"

#define RDATAPATH "../../app/data/"

using namespace std;

int main(int argc, char **argv)
{
  int              width;
  int              height;
  TTF_Font        *pfont;
  bool             must_quit = false;
  int              color;
  t_fcoord         center, pos, dim;
  SDL_Window      *sdlwindow;
  SDL_GLContext    GLContext;
  int              window_xpos, window_ypos;
  float            radius, thickness;
  CGL2Dprimitives *pgfxprimitives;
  CpictureList    *ppicturelist;

  window_xpos = window_ypos = 0;
  width = 1024 + 12;
  height = 512 + 256 + 64;
  // Init all the SDL
  init_SDL(&sdlwindow, &GLContext, window_xpos, window_ypos, width, height);
  // Init the GL library
  pgfxprimitives = new CGL2Dprimitives(sdlwindow, GLContext, width, height);
  pgfxprimitives->set_clear_color(0xFF000000);
  pgfxprimitives->init_OpenGL();
  pgfxprimitives->set_dft_size(2 * 512, 512);
  pgfxprimitives->add_empty_texture(400, 200, string("track_bitmap"));
  
  TTF_Init();
  pfont = TTF_OpenFont(RDATAPATH MAINFONT, 14);
  if (pfont == NULL)
    {
      printf("Error: font \"%s\" is missing.\n", RDATAPATH MAINFONT);
      exit(1);
    }
  ppicturelist = new CpictureList(pgfxprimitives);
  ppicturelist->open_interface_imgs(string(RDATAPATH) + string("images.png"));
  ppicturelist->open_practice_drawings(string(RDATAPATH) + string("images.png"));
  ppicturelist->open_instrument_tabs_drawings(string(RDATAPATH) + string("instruments.png"));
  //
  while (!must_quit)
    {
      if (handle_input_events(&must_quit, &width, &height))
	{
	  pgfxprimitives->SetLogicalSize(width, height);
	}
      // Draw shit
      pgfxprimitives->Clear();
      // Box
#define TEST_BOX
#ifdef TEST_BOX
      pos.x = pos.y = -1;
      dim.x = 100;
      dim.y = 60;
      color = 0xFF2FF3E2;
      pgfxprimitives->box(pos, dim, color);
#endif
      // Circle
#define TEST_CIRC
#ifdef TEST_CIRC
      center.x = width / 2;
      center.y = height / 2;
      radius = 50;
      thickness = 20;
      color = 0xFF75310F;
      pgfxprimitives->circle(center, radius, thickness, color, true);
#endif
      // Disc
#define TEST_DISC
#ifdef TEST_DISC
      center.x = width / 3;
      center.y = height / 3;
      radius = 50;
      thickness = 20;
      color = 0xFF75310F;
      pgfxprimitives->disc(center, radius, color, true);
#endif
      // Draw a texture
      pos.x = width * 0.65;
      pos.y = height * 0.25;
      dim.x = 120;
      dim.y = 150;
      pgfxprimitives->draw_texture(std::string("nail_violin"), pos, dim, true);

      // Print
      pos.x = width * 0.15;
      pos.y = height * 0.75;
      dim.x = 140;
      dim.y = 20;
      pgfxprimitives->print((char*)"whatever we know" , pfont, pos, dim, 0xFFEFFE23);

      //
      pgfxprimitives->RenderPresent();
      usleep(7000);
    }
  delete pgfxprimitives;
  delete ppicturelist;
  return 0;
}
