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
#include <math.h>
#include <list>
#include <vector>
#include <iterator>

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

//#include <GL/gl.h>
//#include <GL/glu.h>
#include <GL/glew.h>
//#include <arb_multisample.h>

#include "gfxareas.h"
#include "mesh.h"
#include "gl2Dprimitives.h"

using namespace std;

Cpicture::Cpicture(SDL_Surface *ptexture, t_coord size, string name, GLuint OpenGL_texture_id):
  m_name(name),
  m_dim(size),
  m_texture(ptexture),
  m_OpenGL_texture_id(OpenGL_texture_id)
{
  //printf("create texture %d - %s\n", m_OpenGL_texture_id, m_name.c_str());
}

Cpicture::~Cpicture()
{
  //printf("delete texture %d - %s\n", m_OpenGL_texture_id, m_name.c_str());
  glDeleteTextures(1, &m_OpenGL_texture_id);
  SDL_FreeSurface(m_texture);
}

bool Cpicture::is_name(string name)
{
  return m_name == name;
}

//----------------------------------------------------------------------------

CGL2Dprimitives::CGL2Dprimitives(SDL_Window *psdl_window, SDL_GLContext GLContext, int scr_w, int scr_h):
  m_sdl_window(psdl_window),
  m_GLContext(GLContext),
  m_w(scr_w),
  m_h(scr_h),
  m_transfert_texture(NULL),
  m_spectrum_texture(NULL)
{
  //SetLogicalSize(scr_h, scr_w); The work texture must be created last or it will crush the next texture when reallocated
  m_reallocMesh.set_name(string("reallocmesh"));
}

CGL2Dprimitives::~CGL2Dprimitives()
{
  std::list<Cpicture*>::iterator iter;
  
  iter = m_picturelist.begin();
  while (iter != m_picturelist.end())
    {
      delete *iter;
      iter++;
    }
  destroy_work_texture();
  destroy_sp_texture();
}

void CGL2Dprimitives::enable_blending(bool enabled)
{
  if (enabled)
    {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
  else
    {
      glDisable(GL_BLEND);
    }
}

void CGL2Dprimitives::set_clear_color(int color)
{
  unsigned char R, G, B, A;
  float         fR, fG, fB, fA;

  R = color & 0xFF;
  G = (color >> 8) & 0xFF;
  B = (color >> 16) & 0xFF;
  A = (color >> 24) & 0xFF;
  fR = (float)R / 255.;
  fG = (float)G / 255.;
  fB = (float)B / 255.;
  fA = (float)A / 255.;
  glClearColor(fR, fG, fB, fA);
}

int CGL2Dprimitives::init_OpenGL()
{
  GLenum err = glewInit();
  if (GLEW_OK != err)
    {
      // Problem: glewInit failed, something is seriously wrong.
      fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
      exit(EXIT_FAILURE);
    }
  glClear(GL_COLOR_BUFFER_BIT);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  // Change projection to the SDL coordinate system
  glDisable(GL_CULL_FACE);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_FOG);
  enable_blending(false);
  //glSampleCoverage(0.5, GL_FALSE);
  color2openGl(IDENTCOLOR);
  reset_viewport();
  RenderPresent();
  //create_work_texture();
  error_handling(__LINE__);
  return false;
}

void CGL2Dprimitives::Clear()
{
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void CGL2Dprimitives::RenderPresent()
{
  SDL_GL_SwapWindow(m_sdl_window);
}

void CGL2Dprimitives::SetLogicalSize(int width, int height)
{
  m_w = width;
  m_h = height;
  destroy_work_texture();
  create_work_texture();
  reset_viewport();
}

bool CGL2Dprimitives::error_handling(int line)
{
  GLenum err;
  string errcode;

  return false;
  err = glGetError();
  if (err)
    {
      switch (err)
      {
      case GL_INVALID_ENUM:
	printf("An unacceptable value is specified for an enumerated argument. The offending command is ignored and has no other side effect than to set the error flag.\n");
	break;
      case GL_INVALID_VALUE:
	printf("A numeric argument is out of range. The offending command is ignored and has no other side effect than to set the error flag.\n");
	break;
      case GL_INVALID_OPERATION:
	printf("The specified operation is not allowed in the current state. The offending command is ignored and has no other side effect than to set the error flag.\n");
	break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:
	printf("The framebuffer object is not complete. The offending command is ignored and has no other side effect than to set the error flag.\n");
	break;
      case GL_OUT_OF_MEMORY:
	printf("There is not enough memory left to execute the command. The state of the GL is undefined, except for the state of the error flags, after this error is recorded.\n");
	break;
      case GL_STACK_UNDERFLOW:
	printf("An attempt has been made to perform an operation that would cause an internal stack to underflow.\n");
	break;
      case GL_STACK_OVERFLOW:
	printf("OpenGL stack overflow\n");
      default:
	printf("OpenGL unknown error\n");
	break;
      };
      printf("OpenGl error: code %d line %d\n", err, line);
      return true;
    }
  return false;
}

// Everything outside the window will not be rendered
void CGL2Dprimitives::set_viewport(t_coord pos, t_coord dim, t_fcoord orthodim)
{
  error_handling(__LINE__);
  glViewport(pos.x, m_h - (pos.y + dim.y), dim.x, dim.y);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, orthodim.x, orthodim.y, 0, -1., 1.);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  if (error_handling(__LINE__))
    {
      printf("Set ViewPort failed.\n");
      exit(EXIT_FAILURE);
    }
}

void CGL2Dprimitives::reset_viewport()
{
  error_handling(__LINE__);
  glViewport(0, 0, m_w, m_h);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, m_w, m_h, 0, -1., 1.); 
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  if (error_handling(__LINE__))
    {
      printf("Set ViewPort failed.\n");
      exit(EXIT_FAILURE);
    }
}

void CGL2Dprimitives::blend_surfaces(SDL_Surface *src, SDL_Rect *srcrect, SDL_Surface *dest, SDL_Rect *dstrect)
{
  if (SDL_SetSurfaceBlendMode(dest, SDL_BLENDMODE_BLEND))
    printf("Error: Surface blend mode failed with code \"%s\".\n", SDL_GetError());  
  if (SDL_SetSurfaceBlendMode(src, SDL_BLENDMODE_BLEND))
    printf("Error: Surface blend mode failed with code \"%s\".\n", SDL_GetError());
  if (SDL_BlitSurface(src, srcrect, dest, dstrect))
    {
      printf("Error: Surface blit failed with code \"%s\".\n", SDL_GetError());
    }
}

Cpicture* CGL2Dprimitives::get_picture(std::string name)
{
  std::list<Cpicture*>::iterator iter;

  iter = m_picturelist.begin();
  while (iter != m_picturelist.end())
    {
      if ((*iter)->is_name(name))
	{
	  return *iter;
	}
      iter++;
    }
  return NULL;
}

bool CGL2Dprimitives::get_texture_size(std::string name, t_coord *pdim)
{
  Cpicture* pp;

  pp = get_picture(name);
  if (pp != NULL)
    {
      *pdim = pp->m_dim;
      return true;
    }
  pdim->x = pdim->y = -1;
  return false;
}

Cpicture* CGL2Dprimitives::create_empty_texture(int w, int h, string tname)
{
  SDL_Surface *img;
  t_coord      dim;
  GLuint       texture_id;
  int          depth;
  Uint32       rmask, gmask, bmask, amask;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif
  depth = 32;
  dim.x = w;
  dim.y = h;
  dim.z = 0;
  img = SDL_CreateRGBSurface(0, dim.x, dim.y, depth, rmask, gmask, bmask, amask);
  texture_id = create_OpenGL_texture(img, tname);
  return (new Cpicture(img, dim, tname, texture_id));
}

void CGL2Dprimitives::add_empty_texture(int w, int h, std::string name)
{
  Cpicture *pp;

  pp = create_empty_texture(w, h, name);
  add_picture(pp);
}

void CGL2Dprimitives::create_work_texture()
{
  string tname("work");

  m_transfert_texture = create_empty_texture(m_w, m_h, tname);
}

void CGL2Dprimitives::destroy_work_texture()
{
  if (m_transfert_texture)
    {
      delete m_transfert_texture;
      m_transfert_texture = NULL;
    }
}

// Loads a surface into the graphic memory.
GLuint CGL2Dprimitives::create_OpenGL_texture(SDL_Surface *psurface, std::string name)
{
  GLuint  textureID;
  t_coord dim;

  glGenTextures(1, &textureID);
  glBindTexture(GL_TEXTURE_2D, textureID);
  dim.x = psurface->clip_rect.w;
  dim.y = psurface->clip_rect.h;
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dim.x, dim.y, 0, GL_RGBA,
	       GL_UNSIGNED_BYTE, psurface->pixels);
  glTexParameteri(GL_TEXTURE_2D , GL_TEXTURE_MIN_FILTER , GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D , GL_TEXTURE_MAG_FILTER , GL_LINEAR);
  error_handling(__LINE__);
  return (textureID);
}

void CGL2Dprimitives::add_picture(Cpicture* pp)
{
  m_picturelist.push_front(pp);
}

void CGL2Dprimitives::texture_quad(t_fcoord pos, t_fcoord dim, GLuint tidentifier, t_fcoord *subdim, t_fcoord *suboffset, bool blending)
{
  t_fcoord tsub, tsubpos;

  if (subdim == NULL)
    {
      tsub.x = tsub.y = 1.;
      tsub.z = 0;
    }
  else
    {
      tsub.x = subdim->x;
      tsub.y = subdim->y;
      tsub.z = subdim->z;
    }
  if (suboffset == NULL)
    {
      tsubpos.x = tsubpos.y = tsubpos.z = 0;
    }
  else
    {
      tsubpos = *suboffset;
    }
  enable_blending(blending);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, tidentifier);
  glBegin(GL_QUADS);
  glTexCoord2f(tsubpos.x, tsubpos.y);
  glVertex2f(pos.x, pos.y);
  glTexCoord2f(tsubpos.x, tsub.y);
  glVertex2f(pos.x, pos.y + dim.y);
  glTexCoord2f(tsub.x, tsub.y);
  glVertex2f(pos.x + dim.x, pos.y + dim.y);
  glTexCoord2f(tsub.x, tsubpos.y);
  glVertex2f(pos.x + dim.x, pos.y);
  glEnd();
  glDisable(GL_TEXTURE_2D);
  error_handling(__LINE__);
}

void CGL2Dprimitives::draw_texture(std::string name, t_fcoord pos, t_fcoord dim, bool filtering, bool blend, t_fcoord *psubdim, t_fcoord *poffset)
{
  Cpicture *pp;

  pp = get_picture(name);
  if (pp != NULL)
    {
      // Translate to 0-1 values
      if (psubdim != NULL)
	{
	  psubdim->x = psubdim->x / (float)pp->m_dim.x;
	  psubdim->y = psubdim->y / (float)pp->m_dim.y;
	}
      if (poffset != NULL)
	{
	  poffset->x = poffset->x / (float)pp->m_dim.x;
	  poffset->y = poffset->y / (float)pp->m_dim.y;
	}
      texture_quad(pos, dim, pp->m_OpenGL_texture_id, psubdim, poffset, blend);
    }
  else
    printf("Error: \"%s\" texture not found.\n", name.c_str());
}

void CGL2Dprimitives::surface_to_texture(Cpicture *dsttext, int width, int height, void *pmap, t_fcoord *subdim)
{
  glBindTexture(GL_TEXTURE_2D, dsttext->m_OpenGL_texture_id);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA,
		  GL_UNSIGNED_BYTE, pmap);
  glTexParameteri(GL_TEXTURE_2D , GL_TEXTURE_MIN_FILTER , GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D , GL_TEXTURE_MAG_FILTER , GL_LINEAR);  
  subdim->x = (float)width / (float)dsttext->m_dim.x;
  subdim->y = (float)height / (float)dsttext->m_dim.y;
  subdim->z = 0;
  error_handling(__LINE__);
}

void CGL2Dprimitives::update_texture_texels(std::string name, int width, int height, void *pmap)
{
  Cpicture *pp;
  t_fcoord  subdim;

  pp = get_picture(name);
  if (pp != NULL)
    {
      surface_to_texture(pp, width, height, pmap, &subdim);
    }
}

void CGL2Dprimitives::surface_to_screen2(t_fcoord pos, t_fcoord dim, int width, int height, void *pmap, bool blending)
{
  t_fcoord subdim;

  surface_to_texture(m_transfert_texture, width, height, pmap, &subdim);
  texture_quad(pos, dim, m_transfert_texture->m_OpenGL_texture_id, &subdim, NULL, blending);
}

void CGL2Dprimitives::surface_to_screen2(t_fcoord pos, t_fcoord dim, SDL_Surface *surface, bool blending)
{
  surface_to_screen2(pos, dim, surface->w, surface->h, surface->pixels, blending);
}

void CGL2Dprimitives::pixdata_to_screen_area(Cgfxarea *pw, int width, int height, void *pmap)
{
  t_fcoord pos, dim;

  pw->get_posf(&pos);
  pw->get_dimf(&dim);
  surface_to_screen2(pos, dim, width, height, pmap);
}

void CGL2Dprimitives::set_dft_size(int w, int h)
{
  m_spectrum_size.x = w;
  m_spectrum_size.y = h;
  create_sp_texture(m_spectrum_size);
}

void CGL2Dprimitives::create_sp_texture(t_coord dim)
{
  string tname("sp");

  m_spectrum_texture = create_empty_texture(dim.x, dim.y, tname);
}

void CGL2Dprimitives::destroy_sp_texture()
{
  if (m_spectrum_texture)
    {
      delete m_spectrum_texture;
      m_spectrum_texture = NULL;
    }
}

// Same as others but rotated
void CGL2Dprimitives::update_spectrum_texture_chunk(int xstart, int xstop, int width, int height, void *pmap)
{
  int       j;

  glBindTexture(GL_TEXTURE_2D, m_spectrum_texture->m_OpenGL_texture_id);
  for (j = 0; j < height; j++)
    {
      // Line by line
      glTexSubImage2D(GL_TEXTURE_2D, 0, xstart, j, xstop - xstart, 1, GL_RGBA,
		      GL_UNSIGNED_BYTE, &(((int*)pmap)[j * width + xstart]));
    }
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  error_handling(__LINE__);
}

void CGL2Dprimitives::draw_spectrum_texture(Cgfxarea *pw, int cut)
{
  t_fcoord  pos, dim;
  double    fcut;
  t_fcoord  tsub;

  pw->get_posf(&pos);
  pw->get_dimf(&dim);
  fcut = (double)cut / (double)m_spectrum_texture->m_dim.x;
  //printf("Cut == %f\n", fcut);
  glMatrixMode(GL_TEXTURE);
  glPushMatrix();
  glTranslated(fcut, 0, 0);
  tsub.x = 1;
  tsub.y = 1;
  tsub.z = 0;
  texture_quad(pos, dim, m_spectrum_texture->m_OpenGL_texture_id, &tsub);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  error_handling(__LINE__);
}

bool CGL2Dprimitives::get_textSize(char *str, TTF_Font *pfont, t_fcoord *pfdim)
{
  int w, h;

  if (TTF_SizeText(pfont, str, &w, &h) == 0)
    {
      pfdim->x = w;
      pfdim->y = h;
      return true;
    }
  return false;
}

// Text
void CGL2Dprimitives::print(char *str, TTF_Font *pfont, t_fcoord pos, int colorin, bool blended, float outline, int colorout)
{
  t_fcoord tdim;

  if (get_textSize(str, pfont, &tdim))
    {
      print(str, pfont, pos, tdim, colorin, blended, outline, colorout);
    }
}

void CGL2Dprimitives::print(char *str, TTF_Font *pfont, t_fcoord pos, t_fcoord dim, int colorin, bool blended, float outline, int colorout)
{
  SDL_Color   c;
  SDL_Color   cbg;
  SDL_Surface *surface;
  t_fcoord    outpos;

  if (outline > 0)
    {
      cbg.r = (colorout >> 16) & 0xFF;
      cbg.g = (colorout >> 8)  & 0xFF;
      cbg.b = colorout & 0xFF;
      //cbg.a = (colorout >> 24) & 0xFF;
      cbg.a = 0xFF; 
      TTF_SetFontOutline(pfont, outline);
      if (blended)
	surface = TTF_RenderText_Blended(pfont, str, cbg);
      else
	surface = TTF_RenderText_Solid(pfont, str, cbg);
      SDL_Surface *img = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ABGR8888, 0);
      SDL_FreeSurface(surface);
      outpos = pos;
      //outpos.x += (dim.x - surface->w) / 2;
      surface_to_screen2(outpos, dim, img->w, img->h, img->pixels, blended);
      SDL_FreeSurface(img);
      TTF_SetFontOutline(pfont, 0);

      pos.x += outline;
      pos.y += 2 * outline;
      dim.x -= 2 * outline;
      dim.y -= 2 * outline;
    }
  c.r = (colorin >> 16) & 0xFF;
  c.g = (colorin >> 8)  & 0xFF;
  c.b = colorin & 0xFF;
  //c.a = (colorin >> 24) & 0xFF;
  c.a = 0xFF;
  if (blended)
    surface = TTF_RenderText_Blended(pfont, str, c);
  else
    surface = TTF_RenderText_Solid(pfont, str, c);
  SDL_Surface *img = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ABGR8888, 0);
  SDL_FreeSurface(surface);  
  //pos.x += (dim.x - surface->w) / 2;
  surface_to_screen2(pos, dim, img->w, img->h, img->pixels, blended);
  SDL_FreeSurface(img);
}

void CGL2Dprimitives::color2openGl(int color)
{
  unsigned char R, G, B, A;

  R = color & 0xFF;
  G = (color >> 8) & 0xFF;
  B = (color >> 16) & 0xFF;
  A = (color >> 24) & 0xFF;
  glColor4ub(R, G, B, A);
  error_handling(__LINE__);
}

void CGL2Dprimitives::hline(float x, float y, float width, int color)
{
  if (width == 0)
    return;
  box(x, y, x + width, y + 1, color, false);
  //line(x, y, x + width, y, color);
}

void CGL2Dprimitives::vline(float x, float y, float height, int color)
{
  if (height == 0)
    return;
  box(x, y, x + 1, y + height, color, false);
  //line(x, y, x , y + 10/*height*/, color);
}

// Bug here, called only twice FIXME replace with a polygon
void CGL2Dprimitives::line(float xsta, float ysta, float xsto, float ysto, int color)
{
  enable_blending(false);
  color2openGl(color);
//  glLineWidth()
  glBegin(GL_LINES);
  glVertex2f(xsta, ysta);
  glVertex2f(xsto, ysto);
  error_handling(__LINE__);
  glEnd();
  color2openGl(IDENTCOLOR);
  error_handling(__LINE__);
}

// Convex polygons only
void CGL2Dprimitives::polygon(t_fcoord *ppointarr, int points, int color, bool bantialiased)
{
  int i;

  enable_blending(true);
  color2openGl(color);
  if (bantialiased)
    glEnable(GL_MULTISAMPLE);
  glBegin(GL_POLYGON);
  for (i = 0; i < points; i++)
    {
      glVertex2f(ppointarr[i].x, ppointarr[i].y);
    }
  glEnd();
  if (bantialiased)
    glDisable(GL_MULTISAMPLE);
  color2openGl(IDENTCOLOR);
  enable_blending(false);
  error_handling(__LINE__);
}

void CGL2Dprimitives::triangle_mesh2D(t_fcoord pos, t_fcoord dim, CMesh *pmesh, int color, bool bantialiased)
{
  t_triangle *ptriangle;
  t_fcoord    p1, p2, p3;
  t_fcoord    meshdim;
  float       xratio, yratio;
  bool        bret;

  meshdim = pmesh->get_size();
  xratio = dim.x / meshdim.x;  // Scaling from object dimensions to drawing box dimensions
  yratio = dim.y / meshdim.y;
  ptriangle = pmesh->get_first_triangle();
  enable_blending(true);
  color2openGl(color);
  // Project inside the box
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glTranslatef(pos.x, pos.y, 0);
  glScaled(xratio, -yratio, 1.);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glEnable(GL_MULTISAMPLE);
  glBegin(GL_TRIANGLES);
  while (ptriangle != NULL)
    {
      bret = pmesh->get_point(ptriangle->v1, &p1);
      bret = bret && pmesh->get_point(ptriangle->v2, &p2);
      bret = bret && pmesh->get_point(ptriangle->v3, &p3);
      if (!bret)
	{
	  printf("Error in 2D primitives: get_point failed\n.");
	  return ;
	}
      glVertex2f(p1.x, p1.y);
      glVertex2f(p2.x, p2.y);
      glVertex2f(p3.x, p3.y);
//       glVertex2f(pos.x + p1.x * xratio, pos.y + p1.y * yratio);
//       glVertex2f(pos.x + p2.x * xratio, pos.y + p2.y * yratio);
//       glVertex2f(pos.x + p3.x * xratio, pos.y + p3.y * yratio);
      ptriangle = pmesh->get_next_triangle();
    }
  glEnd();
  glDisable(GL_MULTISAMPLE);
  // Return to the current projection
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  //
  color2openGl(IDENTCOLOR);
  error_handling(__LINE__);
  enable_blending(false);
}

void CGL2Dprimitives::disc(t_fcoord center, float radius, int color, bool bantialiased)
{
  const int cnpts = 32;
  t_fcoord  points[cnpts];
  int       i;
  float     angle;

  for (i = 0, angle = (2. * M_PI) / (float)cnpts; i < cnpts; i++)
    {      
      points[i].x = center.x + radius * cos(2. * M_PI - angle * (float)i);
      points[i].y = center.y + radius * sin(2. * M_PI - angle * (float)i);
    }
  polygon(points, cnpts, color, bantialiased);
}

void CGL2Dprimitives::rectangle(t_fcoord pos, t_fcoord dim, int color, bool bantialiased)
{
  enable_blending(true);
  vline(pos.x, pos.y, dim.y, color);
  vline(pos.x + dim.x, pos.y, dim.y, color);
  hline(pos.x, pos.y, dim.x, color);
  hline(pos.x, pos.y + dim.y, dim.x, color);
  enable_blending(false);
}

void CGL2Dprimitives::rectangle(float xstart, float ystart, float xstop, float ystop, int color, bool bantialiased)
{
  t_fcoord pos, dim;

  pos.x = xstart;
  pos.y = ystart;
  dim.x = xstop - xstart;
  dim.y = ystop - ystart;
  rectangle(pos, dim, color, bantialiased);
}

void CGL2Dprimitives::box(float xstart, float ystart, float xstop, float ystop, int color, bool bantialiased)
{
  t_fcoord pos, dim;

  pos.x = xstart;
  pos.y = ystart;
  dim.x = xstop - xstart;
  dim.y = ystop - ystart;
  box(pos, dim, color, bantialiased);
}

void CGL2Dprimitives::box(t_fcoord pos, t_fcoord dim, int color, bool bantialiased)
{
  const int cnpts = 4;
  t_fcoord  points[cnpts];

  points[0] = points[1] = points[2] = points[3] = pos;
  points[1].x += dim.x;
  points[2].x += dim.x;
  points[2].y += dim.y;
  points[3].y += dim.y;
  polygon(points, cnpts, color, bantialiased);
}

void CGL2Dprimitives::add_circular_contour(int *pindex, t_fcoord center, float angle_start, float angle_stop, float radius)
{
  const int csegments = 8;
  int       i;
  float     da, angle;
  t_fcoord  point;

  da = (angle_stop - angle_start) / (float)csegments;
  for (i = 1; i < csegments; i++)
    {
      angle = (float)i * da;
      point.x = center.x + radius * cos(angle_start + angle);
      point.y = center.y + radius * sin(angle_start + angle);
      m_reallocMesh.add_vertex(point, (*pindex));
      (*pindex)++;
    }
}

void CGL2Dprimitives::rounded_box(t_fcoord pos, t_fcoord dim, int color, bool bantialiased, float radius)
{
  t_fcoord centerdim, center, pivot;
  t_fcoord point;
  int      i, nbpoints;
  int      indexes[12];

  //box(pos, dim, color, bantialiased);
  //return;
  centerdim.x = dim.x - 2. * radius;
  centerdim.y = dim.y - 2. * radius;  
  center = dim.x / 2.;
  center = dim.y / 2.;
  m_reallocMesh.clear();
  // A list of border points
  i = 1;
  // Center
  m_reallocMesh.add_vertex(center, i++);
  // Top
  point.x = radius;
  point.y = 0;
  m_reallocMesh.add_vertex(point, i++);
  point.x = radius + centerdim.x;
  point.y = 0;
  m_reallocMesh.add_vertex(point, i++);
  pivot.x = radius + centerdim.x;
  pivot.y = -radius;
  add_circular_contour(&i, pivot, M_PI / 2., 0, radius);
  // Right
  point.x = dim.x;
  point.y = -radius;
  m_reallocMesh.add_vertex(point, i++);
  point.x = dim.x;
  point.y = -(radius + centerdim.y);
  m_reallocMesh.add_vertex(point, i++);
  pivot.x = radius + centerdim.x;
  pivot.y = -(radius + centerdim.y);
  add_circular_contour(&i, pivot, 0, -M_PI / 2., radius);
  // Bottom
  point.x = radius + centerdim.x;
  point.y = -(dim.y);
  m_reallocMesh.add_vertex(point, i++);
  point.x = radius;
  pivot.y = -(dim.y);
  m_reallocMesh.add_vertex(point, i++);
  pivot.x = radius;
  pivot.y = -(radius + centerdim.y);
  add_circular_contour(&i, pivot, -M_PI / 2., -M_PI, radius);
  // Left
  point.x = 0;
  point.y = -(radius + centerdim.y);
  m_reallocMesh.add_vertex(point, i++);
  point.x = 0;
  pivot.y = -(radius);
  m_reallocMesh.add_vertex(point, i++);
  pivot.x = radius;
  pivot.y = -(radius);
  add_circular_contour(&i, pivot, M_PI, M_PI / 2., radius);
  // Add the triangles
  nbpoints = i;
  indexes[0] = 1;
  for (i = 2; i < nbpoints; i++)
    {
      indexes[3] = i;
      indexes[6] = (i + 1) < nbpoints? i + 1 : 2;
      m_reallocMesh.add_face(indexes, 9);
    }
  // Draw it
  triangle_mesh2D(pos, dim, &m_reallocMesh, color, bantialiased);
}

void CGL2Dprimitives::circle(t_fcoord center, float radius, float thickness, int color, bool bantialiased)
{
  const int cnpts = 32;
  t_fcoord  point, pivot, pos, dim;
  int       i, v, last, current;
  float     angle;
  int       indexes[12];

  m_reallocMesh.clear();
  angle = (2. * M_PI) / (float)cnpts;
  pivot.x = radius;
  pivot.y = -radius;  
  for (i = 0, v = 1; i < cnpts; i++)
    {
      point.x = pivot.x + radius * cos(2. * M_PI - angle * (float)i);
      point.y = pivot.y + radius * sin(2. * M_PI - angle * (float)i);
      m_reallocMesh.add_vertex(point, v);
      point.x = pivot.x + (radius - thickness) * cos(2. * M_PI - angle * (float)i);
      point.y = pivot.y + (radius - thickness) * sin(2. * M_PI - angle * (float)i);
      m_reallocMesh.add_vertex(point, v + 1);
      v += 2;
    }
  last = v - 2;
  current = 1;
  for (i = 0; i < cnpts; i++)
    {
      indexes[0] = last;
      indexes[3] = current;
      indexes[6] = current + 1;
      indexes[9] = last + 1;
      m_reallocMesh.add_face(indexes, 12);
      last = current;
      current += 2;
    }
  //
  pos.x = center.x - radius;
  pos.y = center.y - radius;
  dim.x = dim.y = 2. * radius;
  triangle_mesh2D(pos, dim, &m_reallocMesh, color, bantialiased);
}

void CGL2Dprimitives::moulagaufre(string texturename, t_fcoord pos, t_fcoord dim, float angle)
{
  Cpicture *pp;

  pp = get_picture(texturename);
  if (pp != NULL)
    {
      //dim.x = pp->m_dim.x;
      //dim.y = pp->m_dim.y;
      //
      glMatrixMode(GL_PROJECTION);
      glPushMatrix();
      glLoadIdentity();

      glOrtho(0, m_w, m_h, 0, -1., 1.);
      glTranslatef(pos.x, pos.y, 0);
      pos.x = pos.y = 0;
      angle = angle * 180. / M_PI;
      glTranslatef(dim.x / 2, dim.x / 2, 0);
      glRotatef(angle, 0, 0, 1);
      glTranslatef(-dim.x / 2, -dim.x / 2, 0);

      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();

      glEnable(GL_MULTISAMPLE);
      texture_quad(pos, dim, pp->m_OpenGL_texture_id, NULL, NULL, true);
      glDisable(GL_MULTISAMPLE);

      // Return to the current projection
      glMatrixMode(GL_PROJECTION);
      glPopMatrix();
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
    }
}

void CGL2Dprimitives::moulagaufre_box(t_fcoord pos, t_fcoord dim, float angle, int color, float radius)
{
  bool      bantialiased = true;

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  
  glOrtho(0, m_w, m_h, 0, -1., 1.);
  glTranslatef(pos.x, pos.y, 0);
  pos.x = pos.y = 0;
  angle = angle * 180. / M_PI;
  glTranslatef(dim.x / 2, dim.x / 2, 0);
  glRotatef(angle, 0, 0, 1);
  glTranslatef(-dim.x / 2, -dim.x / 2, 0);
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  
  glEnable(GL_MULTISAMPLE);
  rounded_box(pos, dim, color, bantialiased, radius);
  glDisable(GL_MULTISAMPLE);
  
  // Return to the current projection
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
}

void CGL2Dprimitives::draw_key(string texturename, t_fcoord pos, t_fcoord dim, bool blending)
{
  Cpicture *pp;

  pp = get_picture(texturename);
  if (pp != NULL)
    {
      glEnable(GL_MULTISAMPLE);
      enable_blending(blending);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, pp->m_OpenGL_texture_id);
      glBegin(GL_QUADS);
      glTexCoord2f(1, 0);
      glVertex2f(pos.x, pos.y);
      glTexCoord2f(0, 0);
      glVertex2f(pos.x, pos.y + dim.y);
      glTexCoord2f(0, 1);
      glVertex2f(pos.x + dim.x, pos.y + dim.y);
      glTexCoord2f(1, 1);
      glVertex2f(pos.x + dim.x, pos.y);
      glEnd();
      glDisable(GL_TEXTURE_2D);
      error_handling(__LINE__);
      glDisable(GL_MULTISAMPLE);
    }
}

